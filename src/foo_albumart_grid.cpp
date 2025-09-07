#include "../include/foo_albumart_grid.h"
#include "../include/album_art_cache.h"
#include "../include/grid_renderer.h"
#include <resource.h>
#include <sstream>

// Plugin GUID - Generate unique GUID for your plugin
const GUID AlbumArtGridElement::g_guid = 
{ 0x12345678, 0x1234, 0x5678, { 0x90, 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78 } };

// UI Element registration
static ui_element_factory<AlbumArtGridElement> g_ui_element_factory;

// Plugin configuration
void PluginConfig::save() {
    cfg_int_t<int> cfg_grouping_mode(guid_cfg_grouping_mode, (int)grouping_mode);
    cfg_int_t<int> cfg_grid_size(guid_cfg_grid_size, (int)grid_size);
    cfg_bool cfg_show_text_overlay(guid_cfg_show_text_overlay, show_text_overlay);
    cfg_bool cfg_dark_theme(guid_cfg_dark_theme, dark_theme);
    cfg_bool cfg_smooth_scrolling(guid_cfg_smooth_scrolling, smooth_scrolling);
    cfg_int_t<int> cfg_cache_size_mb(guid_cfg_cache_size_mb, cache_size_mb);
}

void PluginConfig::load() {
    cfg_int_t<int> cfg_grouping_mode(guid_cfg_grouping_mode, (int)GroupingMode::BY_ALBUM);
    cfg_int_t<int> cfg_grid_size(guid_cfg_grid_size, (int)GridSize::MEDIUM);
    cfg_bool cfg_show_text_overlay(guid_cfg_show_text_overlay, true);
    cfg_bool cfg_dark_theme(guid_cfg_dark_theme, true);
    cfg_bool cfg_smooth_scrolling(guid_cfg_smooth_scrolling, true);
    cfg_int_t<int> cfg_cache_size_mb(guid_cfg_cache_size_mb, 100);
    
    grouping_mode = (GroupingMode)cfg_grouping_mode.get_value();
    grid_size = (GridSize)cfg_grid_size.get_value();
    show_text_overlay = cfg_show_text_overlay;
    dark_theme = cfg_dark_theme;
    smooth_scrolling = cfg_smooth_scrolling;
    cache_size_mb = cfg_cache_size_mb;
}

// Configuration GUIDs
static const GUID guid_cfg_grouping_mode = 
{ 0x87654321, 0x4321, 0x8765, { 0x43, 0x21, 0x87, 0x65, 0x43, 0x21, 0x87, 0x65 } };
static const GUID guid_cfg_grid_size = 
{ 0x11111111, 0x2222, 0x3333, { 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77 } };
static const GUID guid_cfg_show_text_overlay = 
{ 0x22222222, 0x3333, 0x4444, { 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88 } };
static const GUID guid_cfg_dark_theme = 
{ 0x33333333, 0x4444, 0x5555, { 0x66, 0x66, 0x77, 0x77, 0x88, 0x88, 0x99, 0x99 } };
static const GUID guid_cfg_smooth_scrolling = 
{ 0x44444444, 0x5555, 0x6666, { 0x77, 0x77, 0x88, 0x88, 0x99, 0x99, 0xaa, 0xaa } };
static const GUID guid_cfg_cache_size_mb = 
{ 0x55555555, 0x6666, 0x7777, { 0x88, 0x88, 0x99, 0x99, 0xaa, 0xaa, 0xbb, 0xbb } };

// AlbumArtGridPanel implementation
LRESULT AlbumArtGridPanel::OnCreate(LPCREATESTRUCT lpcs) {
    // Load configuration
    m_config.load();
    
    // Initialize members
    m_scroll_pos = 0;
    m_items_per_row = 0;
    m_visible_rows = 0;
    m_total_rows = 0;
    m_item_width = 0;
    m_item_height = 0;
    m_hover_item = -1;
    m_selected_item = -1;
    m_has_focus = false;
    m_loader_stop = false;
    
    // Initialize Direct2D
    InitializeDirect2D();
    
    // Create cache and renderer
    m_cache = std::make_unique<AlbumArtCache>(m_config.cache_size_mb);
    m_renderer = std::make_unique<GridRenderer>();
    
    if (m_render_target) {
        m_renderer->Initialize(m_render_target, m_dwrite_factory);
        m_renderer->UpdateTheme(m_config.dark_theme);
    }
    
    // Register for playback callbacks
    play_callback_manager::get()->register_callback(this, 
        flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_pause, false);
    
    // Start library refresh
    SetTimer(TIMER_REFRESH, 100);
    
    // Start loader thread
    m_loader_thread = std::thread(&AlbumArtGridPanel::LoadAlbumArt, this);
    
    return 0;
}

void AlbumArtGridPanel::OnDestroy() {
    // Stop loader thread
    {
        std::lock_guard<std::mutex> lock(m_items_mutex);
        m_loader_stop = true;
    }
    m_loader_cv.notify_all();
    
    if (m_loader_thread.joinable()) {
        m_loader_thread.join();
    }
    
    // Unregister callbacks
    play_callback_manager::get()->unregister_callback(this);
    
    // Cleanup resources
    CleanupDirect2D();
    m_renderer.reset();
    m_cache.reset();
}

void AlbumArtGridPanel::OnNcDestroy() {
    // This is the final window message - HWND will be invalid after this
    // The host should now release its reference to our ui_element_instance
}

// CRITICAL: UI Element callback interface - prevents shutdown crash
void AlbumArtGridPanel::on_host_shutdown() {
    // The host (foo_ui_std) is telling us it's shutting down
    // We must destroy our window NOW to signal the host to release its reference
    if (IsWindow()) {
        DestroyWindow();
    }
}

void AlbumArtGridPanel::OnSize(UINT nType, CSize size) {
    if (m_render_target) {
        D2D1_SIZE_U new_size = D2D1::SizeU(size.cx, size.cy);
        m_render_target->Resize(new_size);
        UpdateLayout();
        InvalidateRect(nullptr);
    }
}

void AlbumArtGridPanel::OnPaint(CDCHandle dc) {
    if (!m_renderer || !m_render_target) {
        PAINTSTRUCT ps;
        BeginPaint(&ps);
        EndPaint(&ps);
        return;
    }
    
    PAINTSTRUCT ps;
    BeginPaint(&ps);
    
    m_renderer->BeginRender();
    
    // Calculate layout
    RECT client_rect;
    GetClientRect(&client_rect);
    GridLayout layout;
    m_renderer->CalculateLayout(client_rect, m_config.grid_size, 
                               (int)m_filtered_indices.size(), layout);
    
    // Store layout info
    m_items_per_row = layout.items_per_row;
    m_visible_rows = layout.visible_rows;
    m_total_rows = layout.total_rows;
    m_item_width = layout.item_width;
    m_item_height = layout.item_height;
    
    // Render background
    m_renderer->RenderBackground(layout);
    
    // Render items
    std::lock_guard<std::mutex> lock(m_items_mutex);
    
    if (m_filtered_indices.empty()) {
        m_renderer->RenderNoResults(layout, "No items to display");
    } else {
        // Calculate visible range
        int first_visible_row = m_scroll_pos / m_item_height;
        int last_visible_row = first_visible_row + m_visible_rows + 1;
        
        for (int row = first_visible_row; row <= last_visible_row && row < m_total_rows; ++row) {
            for (int col = 0; col < m_items_per_row; ++col) {
                int item_index = row * m_items_per_row + col;
                if (item_index >= (int)m_filtered_indices.size()) break;
                
                int actual_index = m_filtered_indices[item_index];
                if (actual_index >= 0 && actual_index < (int)m_items.size()) {
                    const auto& item = m_items[actual_index];
                    
                    bool selected = (item_index == m_selected_item);
                    bool hovered = (item_index == m_hover_item);
                    bool playing = false; // TODO: Check if current track is in this item
                    
                    m_renderer->RenderItem(layout, *item, item_index, m_scroll_pos, 
                                         selected, hovered, playing);
                }
            }
        }
    }
    
    // Render scrollbar
    if (m_total_rows * m_item_height > layout.client_rect.bottom - layout.client_rect.top) {
        m_renderer->RenderScrollbar(layout, m_scroll_pos, m_total_rows * m_item_height);
    }
    
    m_renderer->EndRender();
    
    EndPaint(&ps);
}

BOOL AlbumArtGridPanel::OnEraseBkgnd(CDCHandle dc) {
    return TRUE; // We handle background in OnPaint
}

void AlbumArtGridPanel::OnLButtonDown(UINT nFlags, CPoint point) {
    SetFocus();
    
    int item_index = GetItemAtPoint(point);
    if (item_index >= 0) {
        m_selected_item = item_index;
        InvalidateRect(nullptr);
    }
}

void AlbumArtGridPanel::OnLButtonDblClk(UINT nFlags, CPoint point) {
    int item_index = GetItemAtPoint(point);
    if (item_index >= 0) {
        PlayItem(item_index);
    }
}

void AlbumArtGridPanel::OnRButtonDown(UINT nFlags, CPoint point) {
    int item_index = GetItemAtPoint(point);
    if (item_index >= 0) {
        m_selected_item = item_index;
        InvalidateRect(nullptr);
        ShowContextMenu(point, item_index);
    }
}

void AlbumArtGridPanel::OnMouseMove(UINT nFlags, CPoint point) {
    int item_index = GetItemAtPoint(point);
    if (item_index != m_hover_item) {
        m_hover_item = item_index;
        InvalidateRect(nullptr);
    }
}

BOOL AlbumArtGridPanel::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
    if (m_total_rows == 0) return TRUE;
    
    int scroll_delta = -zDelta / WHEEL_DELTA * SCROLL_LINE_SIZE;
    int new_scroll_pos = m_scroll_pos + scroll_delta;
    
    int max_scroll = max(0, m_total_rows * m_item_height - (m_visible_rows * m_item_height));
    new_scroll_pos = max(0, min(new_scroll_pos, max_scroll));
    
    if (new_scroll_pos != m_scroll_pos) {
        m_scroll_pos = new_scroll_pos;
        UpdateScrollInfo();
        InvalidateRect(nullptr);
    }
    
    return TRUE;
}

void AlbumArtGridPanel::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
    if (m_filtered_indices.empty()) return;
    
    int new_selected = m_selected_item;
    
    switch (nChar) {
    case VK_LEFT:
        if (m_selected_item > 0) {
            new_selected = m_selected_item - 1;
        }
        break;
        
    case VK_RIGHT:
        if (m_selected_item < (int)m_filtered_indices.size() - 1) {
            new_selected = m_selected_item + 1;
        }
        break;
        
    case VK_UP:
        if (m_selected_item >= m_items_per_row) {
            new_selected = m_selected_item - m_items_per_row;
        }
        break;
        
    case VK_DOWN:
        if (m_selected_item + m_items_per_row < (int)m_filtered_indices.size()) {
            new_selected = m_selected_item + m_items_per_row;
        }
        break;
        
    case VK_RETURN:
        if (m_selected_item >= 0) {
            PlayItem(m_selected_item);
        }
        break;
        
    case VK_SPACE:
        if (m_selected_item >= 0) {
            AddItemToPlaylist(m_selected_item);
        }
        break;
        
    case VK_HOME:
        new_selected = 0;
        break;
        
    case VK_END:
        new_selected = (int)m_filtered_indices.size() - 1;
        break;
    }
    
    if (new_selected != m_selected_item && new_selected >= 0) {
        m_selected_item = new_selected;
        EnsureItemVisible(m_selected_item);
        InvalidateRect(nullptr);
    }
}

void AlbumArtGridPanel::OnSetFocus(HWND hWndOther) {
    m_has_focus = true;
    InvalidateRect(nullptr);
}

void AlbumArtGridPanel::OnKillFocus(HWND hWndNew) {
    m_has_focus = false;
    InvalidateRect(nullptr);
}

void AlbumArtGridPanel::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == TIMER_REFRESH) {
        KillTimer(TIMER_REFRESH);
        RefreshLibrary();
    } else if (nIDEvent == TIMER_INVALIDATE) {
        KillTimer(TIMER_INVALIDATE);
        InvalidateRect(nullptr);
    }
}

void AlbumArtGridPanel::set_configuration(stream_reader* p_reader, t_size p_size, abort_callback& p_abort) {
    if (p_size > 0) {
        // Read configuration from stream
        // For now, use default configuration
    }
}

void AlbumArtGridPanel::get_configuration(stream_writer* p_writer, abort_callback& p_abort) const {
    // Write configuration to stream
    // For now, write empty configuration
}

void AlbumArtGridPanel::initialize_window(HWND parent) {
    Create(parent, nullptr, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
}

void AlbumArtGridPanel::on_playback_new_track(metadb_handle_ptr p_track) {
    // Update playing indicator
    InvalidateRect(nullptr);
}

void AlbumArtGridPanel::on_playback_stop(play_control::t_stop_reason p_reason) {
    // Update playing indicator
    InvalidateRect(nullptr);
}

void AlbumArtGridPanel::on_playback_pause(bool p_state) {
    // Update playing indicator
    InvalidateRect(nullptr);
}

void AlbumArtGridPanel::RefreshLibrary() {
    CreateGridItems();
    ApplyFilter();
    UpdateLayout();
    InvalidateRect(nullptr);
}

void AlbumArtGridPanel::SetGroupingMode(GroupingMode mode) {
    if (m_config.grouping_mode != mode) {
        m_config.grouping_mode = mode;
        m_config.save();
        RefreshLibrary();
    }
}

void AlbumArtGridPanel::SetGridSize(GridSize size) {
    if (m_config.grid_size != size) {
        m_config.grid_size = size;
        m_config.save();
        UpdateLayout();
        InvalidateRect(nullptr);
    }
}

void AlbumArtGridPanel::ShowConfig() {
    ConfigDialog dlg(m_config);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        m_config.save();
        m_renderer->UpdateTheme(m_config.dark_theme);
        m_cache->SetMaxCacheSize(m_config.cache_size_mb);
        RefreshLibrary();
    }
}

void AlbumArtGridPanel::SetFilter(const char* filter_text) {
    m_filter_text = filter_text;
    ApplyFilter();
    UpdateLayout();
    InvalidateRect(nullptr);
}

// Private methods implementation would continue here...
// This is a substantial amount of code, so I'll continue with the remaining methods in the next part