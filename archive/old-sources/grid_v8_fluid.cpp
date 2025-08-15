// Album Art Grid Component v8.3.0 - Fluid Dynamic Resizing
// Automatically adjusts grid to perfectly fit container width

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "SDK-2025-03-07/foobar2000/helpers/helpers.h"
#include "SDK-2025-03-07/foobar2000/SDK/ui_element.h"
#include "SDK-2025-03-07/foobar2000/SDK/coreDarkMode.h"
#include "SDK-2025-03-07/foobar2000/helpers/DarkMode.h"
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

// GUIDs
static const GUID guid_element = { 0xc26ce458, 0x41b7, 0x4e6e, { 0xa3, 0xf3, 0x12, 0x2f, 0x3c, 0xde, 0x8e, 0x64 } };
static const GUID guid_branch = { 0x1d4b5c87, 0xe32a, 0x4b8d, { 0x9f, 0x65, 0x2a, 0x3c, 0x5d, 0x7b, 0x9e, 0x32 } };

static const int PADDING = 4;
static const int MIN_PADDING = 2;
static const int MAX_PADDING = 10;
static const int LINE_HEIGHT = 14;
static const int MIN_ITEM_SIZE = 50;
static const int MAX_ITEM_SIZE = 300;
static const int DEFAULT_ITEM_SIZE = 120;

// Fluid resizing modes
enum class ResizeMode {
    FIXED = 0,        // Fixed size (traditional mode)
    AUTO_FIT = 1,     // Auto-fit to container width
    AUTO_FILL = 2,    // Fill container maintaining aspect ratio
    FLUID = 3         // Completely fluid resizing
};

// Configuration structure
struct grid_config {
    int item_size = DEFAULT_ITEM_SIZE;
    int min_item_size = 80;
    int max_item_size = 200;
    ResizeMode resize_mode = ResizeMode::AUTO_FIT;
    bool maintain_aspect_ratio = true;
    int target_columns = 0;  // 0 = automatic
    int padding = PADDING;
    bool show_text = true;
    bool show_track_count = false;
    int group_mode = 0;
    int sort_mode = 0;
    int text_lines = 2;
    
    void load(ui_element_config::ptr cfg) {
        if (cfg.is_valid()) {
            try {
                abort_callback_dummy abort;
                file::ptr reader;
                filesystem::g_open_tempmem(reader, abort);
                cfg->get_data_raw(reader.get_ptr(), abort);
                
                reader->read_lendian_t(item_size, abort);
                reader->read_lendian_t(show_text, abort);
                reader->read_lendian_t(show_track_count, abort);
                reader->read_lendian_t(group_mode, abort);
                reader->read_lendian_t(sort_mode, abort);
                reader->read_lendian_t(text_lines, abort);
                
                // New fluid resizing settings
                int resize_mode_int = (int)resize_mode;
                reader->read_lendian_t(resize_mode_int, abort);
                resize_mode = (ResizeMode)resize_mode_int;
                reader->read_lendian_t(min_item_size, abort);
                reader->read_lendian_t(max_item_size, abort);
                reader->read_lendian_t(maintain_aspect_ratio, abort);
                reader->read_lendian_t(target_columns, abort);
                reader->read_lendian_t(padding, abort);
            } catch (...) {
                // Use defaults on error
                resize_mode = ResizeMode::AUTO_FIT;
                min_item_size = 80;
                max_item_size = 200;
            }
        }
    }
    
    ui_element_config::ptr save(const GUID& guid) {
        ui_element_config_builder builder;
        abort_callback_dummy abort;
        file::ptr writer;
        filesystem::g_open_tempmem(writer, abort);
        
        writer->write_lendian_t(item_size, abort);
        writer->write_lendian_t(show_text, abort);
        writer->write_lendian_t(show_track_count, abort);
        writer->write_lendian_t(group_mode, abort);
        writer->write_lendian_t(sort_mode, abort);
        writer->write_lendian_t(text_lines, abort);
        
        // Save new fluid resizing settings
        writer->write_lendian_t((int)resize_mode, abort);
        writer->write_lendian_t(min_item_size, abort);
        writer->write_lendian_t(max_item_size, abort);
        writer->write_lendian_t(maintain_aspect_ratio, abort);
        writer->write_lendian_t(target_columns, abort);
        writer->write_lendian_t(padding, abort);
        
        return builder.finish(guid, writer->get_ptr());
    }
};

// Album item structure
struct album_item {
    metadb_handle_list tracks;
    pfc::string8 folder_name;
    pfc::string8 album_name;
    pfc::string8 artist_name;
    pfc::string8 display_name;
    pfc::string8 sort_key;
    t_filetimestamp mod_time;
    album_art_data_ptr art_data;
    Gdiplus::Bitmap* thumbnail = nullptr;
    bool art_loaded = false;
    
    ~album_item() {
        if (thumbnail) {
            delete thumbnail;
            thumbnail = nullptr;
        }
    }
};

class album_grid_instance : public ui_element_instance, public library_callback_dynamic {
private:
    HWND m_hwnd = nullptr;
    ui_element_instance_callback::ptr m_callback;
    std::vector<std::shared_ptr<album_item>> m_items;
    std::set<int> m_selected_indices;
    int m_last_selected = -1;
    int m_hover_index = -1;
    bool m_tracking = false;
    int m_scroll_pos = 0;
    grid_config m_config;
    bool m_library_initialized = false;
    fb2k::CCoreDarkModeHooks m_dark;
    
    // Fluid resizing calculation
    int m_calculated_item_size = DEFAULT_ITEM_SIZE;
    int m_actual_columns = 0;
    int m_actual_padding = PADDING;
    
    // Calculate optimal item size for fluid resizing
    void calculate_fluid_layout() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int available_width = rc.right;
        
        if (available_width <= 0) {
            m_calculated_item_size = m_config.item_size;
            m_actual_columns = 1;
            m_actual_padding = m_config.padding;
            return;
        }
        
        switch (m_config.resize_mode) {
            case ResizeMode::FIXED:
                // Traditional fixed size mode
                m_calculated_item_size = m_config.item_size;
                m_actual_padding = m_config.padding;
                m_actual_columns = max(1, (available_width - m_actual_padding) / 
                                         (m_calculated_item_size + m_actual_padding));
                break;
                
            case ResizeMode::AUTO_FIT: {
                // Auto-fit: Find optimal item size to minimize wasted space
                int best_size = m_config.item_size;
                int best_waste = available_width;
                int best_cols = 1;
                
                // Try different column counts
                for (int cols = 1; cols <= 20; cols++) {
                    // Calculate item size for this column count
                    int total_padding = m_config.padding * (cols + 1);
                    int item_space = available_width - total_padding;
                    int item_size = item_space / cols;
                    
                    // Check if item size is within limits
                    if (item_size < m_config.min_item_size) break;
                    if (item_size > m_config.max_item_size) continue;
                    
                    // Calculate wasted space
                    int used_width = cols * item_size + total_padding;
                    int waste = available_width - used_width;
                    
                    if (waste < best_waste && waste >= 0) {
                        best_waste = waste;
                        best_size = item_size;
                        best_cols = cols;
                    }
                }
                
                m_calculated_item_size = best_size;
                m_actual_columns = best_cols;
                m_actual_padding = m_config.padding;
                break;
            }
            
            case ResizeMode::AUTO_FILL: {
                // Auto-fill: Stretch items to completely fill width
                int target_cols = m_config.target_columns;
                if (target_cols <= 0) {
                    // Auto-determine column count based on preferred size
                    target_cols = max(1, available_width / (m_config.item_size + m_config.padding));
                }
                
                // Calculate exact item size to fill width
                int total_padding = m_config.padding * (target_cols + 1);
                int item_space = available_width - total_padding;
                m_calculated_item_size = max(m_config.min_item_size, 
                                            min(m_config.max_item_size, item_space / target_cols));
                m_actual_columns = target_cols;
                m_actual_padding = m_config.padding;
                break;
            }
            
            case ResizeMode::FLUID: {
                // Completely fluid: Dynamically adjust both size and padding
                int best_size = m_config.item_size;
                int best_padding = m_config.padding;
                int best_cols = 1;
                int best_score = INT_MAX;
                
                // Try different combinations
                for (int cols = 2; cols <= 15; cols++) {
                    for (int pad = MIN_PADDING; pad <= MAX_PADDING; pad++) {
                        int total_padding = pad * (cols + 1);
                        int item_space = available_width - total_padding;
                        int item_size = item_space / cols;
                        
                        if (item_size < m_config.min_item_size || 
                            item_size > m_config.max_item_size) continue;
                        
                        // Calculate score (prefer sizes close to target, minimal waste)
                        int used_width = cols * item_size + total_padding;
                        int waste = available_width - used_width;
                        int size_diff = abs(item_size - m_config.item_size);
                        int score = waste * 10 + size_diff;
                        
                        if (score < best_score && waste >= 0) {
                            best_score = score;
                            best_size = item_size;
                            best_padding = pad;
                            best_cols = cols;
                        }
                    }
                }
                
                m_calculated_item_size = best_size;
                m_actual_columns = best_cols;
                m_actual_padding = best_padding;
                break;
            }
        }
    }
    
public:
    album_grid_instance(HWND parent, ui_element_config::ptr config, 
                        ui_element_instance_callback::ptr callback) 
        : m_callback(callback) {
        
        m_config.load(config);
        
        // Create window
        m_hwnd = CreateWindowEx(
            0,
            L"STATIC",
            L"Album Grid",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            parent,
            nullptr,
            core_api::get_my_instance(),
            nullptr
        );
        
        if (m_hwnd) {
            SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
            SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)&window_proc);
            
            // Initialize dark mode
            m_dark.AddDialogWithControls(m_hwnd);
            if (m_dark) {
                SetWindowTheme(m_hwnd, L"DarkMode_Explorer", nullptr);
            }
            
            // Initialize GDI+
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            ULONG_PTR gdiplusToken;
            Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
            
            // Register for library changes
            library_manager::get()->register_callback_dynamic(this);
            
            // Initial load
            refresh_items();
        }
    }
    
    ~album_grid_instance() {
        if (m_library_initialized) {
            library_manager::get()->unregister_callback(this);
        }
        m_items.clear();
    }
    
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        album_grid_instance* instance = (album_grid_instance*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (instance) {
            switch (msg) {
                case WM_PAINT: return instance->on_paint();
                case WM_SIZE: return instance->on_size();
                case WM_VSCROLL: return instance->on_vscroll(LOWORD(wp));
                case WM_MOUSEWHEEL: return instance->on_mousewheel(GET_WHEEL_DELTA_WPARAM(wp), GET_KEYSTATE_WPARAM(wp));
                case WM_LBUTTONDOWN: return instance->on_lbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), wp);
                case WM_LBUTTONDBLCLK: return instance->on_lbuttondblclk(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_RBUTTONDOWN: return instance->on_rbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_KEYDOWN: return instance->on_keydown(wp);
                case WM_MOUSEMOVE: return instance->on_mousemove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSELEAVE: return instance->on_mouseleave();
            }
        }
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    
    // Refresh library items
    void refresh_items() {
        m_items.clear();
        m_selected_indices.clear();
        
        static_api_ptr_t<library_manager> lm;
        metadb_handle_list all_items;
        lm->get_all_items(all_items);
        
        if (all_items.get_count() == 0) {
            InvalidateRect(m_hwnd, NULL, FALSE);
            return;
        }
        
        // Group items based on mode
        std::map<pfc::string8, std::shared_ptr<album_item>> grouped;
        
        for (t_size i = 0; i < all_items.get_count(); i++) {
            metadb_handle_ptr track = all_items[i];
            
            pfc::string8 key;
            pfc::string8 display_name;
            pfc::string8 sort_key;
            
            const char* path = track->get_path();
            
            if (m_config.group_mode == 0) {
                // Group by folder
                pfc::string8 folder = pfc::string_directory(path);
                key = folder;
                
                // Extract folder name for display
                const char* folder_name = strrchr(folder, '\\');
                if (!folder_name) folder_name = strrchr(folder, '/');
                display_name = folder_name ? (folder_name + 1) : folder;
                sort_key = display_name;
            } else if (m_config.group_mode == 1) {
                // Group by album
                file_info_impl info;
                if (track->get_info(info)) {
                    const char* album = info.meta_get("album", 0);
                    const char* artist = info.meta_get("artist", 0);
                    if (album) {
                        key = album;
                        if (artist) {
                            key += "|";
                            key += artist;
                            display_name << album << "\n" << artist;
                        } else {
                            display_name = album;
                        }
                        sort_key = album;
                    }
                }
            } else {
                // Group by artist
                file_info_impl info;
                if (track->get_info(info)) {
                    const char* artist = info.meta_get("artist", 0);
                    if (artist) {
                        key = artist;
                        display_name = artist;
                        sort_key = artist;
                    }
                }
            }
            
            if (key.is_empty()) continue;
            
            auto it = grouped.find(key);
            if (it == grouped.end()) {
                auto item = std::make_shared<album_item>();
                item->folder_name = pfc::string_directory(path);
                item->display_name = display_name;
                item->sort_key = sort_key;
                
                // Get modification time
                t_filetimestamp ts = filesystem::g_get_filetimestamp(path);
                item->mod_time = ts;
                
                grouped[key] = item;
                it = grouped.find(key);
            }
            
            it->second->tracks.add_item(track);
        }
        
        // Convert map to vector for display
        for (auto& pair : grouped) {
            m_items.push_back(pair.second);
        }
        
        // Sort items
        if (m_config.sort_mode == 0) {
            // Sort by name
            std::sort(m_items.begin(), m_items.end(), 
                [](const std::shared_ptr<album_item>& a, const std::shared_ptr<album_item>& b) {
                    return pfc::stricmp_ascii(a->sort_key.c_str(), b->sort_key.c_str()) < 0;
                });
        } else if (m_config.sort_mode == 1) {
            // Sort by date
            std::sort(m_items.begin(), m_items.end(), 
                [](const std::shared_ptr<album_item>& a, const std::shared_ptr<album_item>& b) {
                    return a->mod_time > b->mod_time;
                });
        } else {
            // Sort by track count
            std::sort(m_items.begin(), m_items.end(), 
                [](const std::shared_ptr<album_item>& a, const std::shared_ptr<album_item>& b) {
                    return a->tracks.get_count() > b->tracks.get_count();
                });
        }
        
        // Load album art in background
        for (auto& item : m_items) {
            load_album_art(item);
        }
        
        calculate_fluid_layout();
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void load_album_art(std::shared_ptr<album_item> item) {
        if (item->art_loaded || item->tracks.get_count() == 0) return;
        
        static_api_ptr_t<album_art_manager_v2> aam;
        abort_callback_dummy abort;
        
        try {
            item->art_data = aam->open(item->tracks[0], abort);
            item->art_loaded = true;
            
            // Create thumbnail
            if (item->art_data.is_valid() && !item->thumbnail) {
                create_thumbnail(item);
            }
        } catch (...) {
            item->art_loaded = true;
        }
    }
    
    void create_thumbnail(std::shared_ptr<album_item> item) {
        if (!item->art_data.is_valid()) return;
        
        try {
            // Get actual calculated size for thumbnail
            int thumb_size = m_calculated_item_size;
            
            IStream* stream = SHCreateMemStream(
                (const BYTE*)item->art_data->get_ptr(), 
                item->art_data->get_size()
            );
            
            if (stream) {
                Gdiplus::Image img(stream);
                if (img.GetLastStatus() == Gdiplus::Ok) {
                    // Create high-quality thumbnail
                    item->thumbnail = new Gdiplus::Bitmap(thumb_size, thumb_size);
                    Gdiplus::Graphics g(item->thumbnail);
                    g.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);
                    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
                    g.DrawImage(&img, 0, 0, thumb_size, thumb_size);
                }
                stream->Release();
            }
        } catch (...) {
            // Handle error
        }
    }
    
    LRESULT on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        // Get colors
        t_ui_color color_back = m_callback->query_std_color(ui_color_background);
        t_ui_color color_text = m_callback->query_std_color(ui_color_text);
        t_ui_color color_sel = m_callback->query_std_color(ui_color_selection);
        t_ui_color color_hover = RGB(
            GetRValue(color_sel) * 0.3 + GetRValue(color_back) * 0.7,
            GetGValue(color_sel) * 0.3 + GetGValue(color_back) * 0.7,
            GetBValue(color_sel) * 0.3 + GetBValue(color_back) * 0.7
        );
        
        // Fill background
        HBRUSH br_back = CreateSolidBrush(color_back);
        FillRect(hdc, &ps.rcPaint, br_back);
        DeleteObject(br_back);
        
        // Calculate layout with fluid resizing
        calculate_fluid_layout();
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_calculated_item_size + text_height;
        int cols = m_actual_columns;
        
        // Calculate actual positions for perfect centering
        int total_items_width = cols * m_calculated_item_size + (cols - 1) * m_actual_padding;
        int start_x = (rc.right - total_items_width) / 2;
        
        // Draw items
        int visible_start = m_scroll_pos / (item_total_height + m_actual_padding);
        int visible_end = (m_scroll_pos + rc.bottom) / (item_total_height + m_actual_padding) + 1;
        
        for (int i = visible_start * cols; i < min((int)m_items.size(), (visible_end + 1) * cols); i++) {
            if (i < 0 || i >= (int)m_items.size()) continue;
            
            int col = i % cols;
            int row = i / cols;
            
            int x = start_x + col * (m_calculated_item_size + m_actual_padding);
            int y = row * (item_total_height + m_actual_padding) + m_actual_padding - m_scroll_pos;
            
            // Skip if not visible
            if (y + item_total_height < 0 || y > rc.bottom) continue;
            
            bool selected = m_selected_indices.find(i) != m_selected_indices.end();
            bool hover = (i == m_hover_index);
            
            draw_item(hdc, m_items[i], x, y, selected, hover, color_back, color_text, color_sel, color_hover);
        }
        
        // Draw debug info in fluid mode
        if (m_config.resize_mode != ResizeMode::FIXED) {
            HFONT font = m_callback->query_font_ex(ui_font_default);
            SelectObject(hdc, font);
            SetTextColor(hdc, color_text);
            SetBkMode(hdc, TRANSPARENT);
            
            wchar_t debug[256];
            swprintf_s(debug, L"Mode: %s | Cols: %d | Size: %dpx | Padding: %dpx",
                m_config.resize_mode == ResizeMode::AUTO_FIT ? L"Auto-Fit" :
                m_config.resize_mode == ResizeMode::AUTO_FILL ? L"Auto-Fill" :
                m_config.resize_mode == ResizeMode::FLUID ? L"Fluid" : L"Fixed",
                m_actual_columns, m_calculated_item_size, m_actual_padding);
            
            RECT debug_rc = {5, 5, rc.right - 5, 25};
            DrawText(hdc, debug, -1, &debug_rc, DT_LEFT | DT_TOP);
        }
        
        EndPaint(m_hwnd, &ps);
        return 0;
    }
    
    void draw_item(HDC hdc, std::shared_ptr<album_item> item, int x, int y, 
                   bool selected, bool hover, COLORREF color_back, COLORREF color_text,
                   COLORREF color_sel, COLORREF color_hover) {
        
        int size = m_calculated_item_size;
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        
        // Draw selection/hover background
        if (selected) {
            HBRUSH br = CreateSolidBrush(color_sel);
            RECT sel_rc = {x - 2, y - 2, x + size + 2, y + size + text_height + 2};
            FillRect(hdc, &sel_rc, br);
            DeleteObject(br);
        } else if (hover) {
            HBRUSH br = CreateSolidBrush(color_hover);
            RECT hover_rc = {x - 1, y - 1, x + size + 1, y + size + text_height + 1};
            FillRect(hdc, &hover_rc, br);
            DeleteObject(br);
        }
        
        // Draw album art or placeholder
        if (item->thumbnail) {
            // Recreate thumbnail if size changed significantly
            if (abs(item->thumbnail->GetWidth() - size) > 10) {
                delete item->thumbnail;
                item->thumbnail = nullptr;
                create_thumbnail(item);
            }
            
            if (item->thumbnail) {
                Gdiplus::Graphics g(hdc);
                g.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);
                g.DrawImage(item->thumbnail, x, y, size, size);
            }
        } else {
            // Draw placeholder
            HBRUSH br = CreateSolidBrush(RGB(60, 60, 60));
            RECT art_rc = {x, y, x + size, y + size};
            FillRect(hdc, &art_rc, br);
            DeleteObject(br);
            
            // Draw text placeholder
            HFONT font = m_callback->query_font_ex(ui_font_default);
            SelectObject(hdc, font);
            SetTextColor(hdc, RGB(150, 150, 150));
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, L"No Art", -1, &art_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        
        // Draw text
        if (m_config.show_text) {
            HFONT font = m_callback->query_font_ex(ui_font_default);
            RECT text_rc = {x, y + size + 2, x + size, y + size + text_height};
            SetTextColor(hdc, color_text);
            SelectObject(hdc, font);
            
            pfc::stringcvt::string_os_from_utf8 display_text(item->display_name.c_str());
            
            if (m_config.show_track_count && item->tracks.get_count() > 0) {
                pfc::string8 text_with_count;
                text_with_count << item->display_name << " (" << item->tracks.get_count() << ")";
                display_text = pfc::stringcvt::string_os_from_utf8(text_with_count.c_str());
            }
            
            UINT format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX;
            if (m_config.text_lines == 1) {
                format = DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX;
            }
            
            DrawText(hdc, display_text, -1, &text_rc, format);
        }
    }
    
    int hit_test(int x, int y) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_fluid_layout();
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_calculated_item_size + text_height;
        int cols = m_actual_columns;
        
        // Calculate actual positions for perfect centering
        int total_items_width = cols * m_calculated_item_size + (cols - 1) * m_actual_padding;
        int start_x = (rc.right - total_items_width) / 2;
        
        // Find which column was clicked
        int rel_x = x - start_x;
        if (rel_x < 0) return -1;
        
        int col = rel_x / (m_calculated_item_size + m_actual_padding);
        if (col >= cols) return -1;
        
        // Check if click is within item bounds (not in padding)
        int item_x = col * (m_calculated_item_size + m_actual_padding);
        if (rel_x < item_x || rel_x >= item_x + m_calculated_item_size) return -1;
        
        // Find which row was clicked
        int row = (y + m_scroll_pos - m_actual_padding) / (item_total_height + m_actual_padding);
        if (row < 0) return -1;
        
        int index = row * cols + col;
        if (index >= 0 && index < (int)m_items.size()) {
            return index;
        }
        
        return -1;
    }
    
    LRESULT on_lbuttondown(int x, int y, WPARAM keys) {
        int index = hit_test(x, y);
        if (index >= 0) {
            if (keys & MK_CONTROL) {
                // Ctrl+Click: Toggle selection
                if (m_selected_indices.find(index) != m_selected_indices.end()) {
                    m_selected_indices.erase(index);
                } else {
                    m_selected_indices.insert(index);
                }
                m_last_selected = index;
            } else if (keys & MK_SHIFT && m_last_selected >= 0) {
                // Shift+Click: Range selection
                m_selected_indices.clear();
                int start = min(m_last_selected, index);
                int end = max(m_last_selected, index);
                for (int i = start; i <= end; i++) {
                    m_selected_indices.insert(i);
                }
            } else {
                // Normal click: Single selection
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                m_last_selected = index;
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // Click on empty space: Clear selection
            m_selected_indices.clear();
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        SetFocus(m_hwnd);
        return 0;
    }
    
    LRESULT on_lbuttondblclk(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_items.size()) {
            // Play the item
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            if (playlist != pfc::infinite_size) {
                pm->playlist_clear(playlist);
                
                // Add all selected items or just the double-clicked one
                if (!m_selected_indices.empty() && 
                    m_selected_indices.find(index) != m_selected_indices.end()) {
                    // Play all selected items
                    for (int sel_index : m_selected_indices) {
                        if (sel_index < (int)m_items.size()) {
                            pm->playlist_add_items(playlist, m_items[sel_index]->tracks, bit_array_false());
                        }
                    }
                } else {
                    // Play only the double-clicked item
                    pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                }
                
                pm->set_playing_playlist(playlist);
                static_api_ptr_t<playback_control> pc;
                pc->play_start();
            }
        }
        return 0;
    }
    
    LRESULT on_rbuttondown(int x, int y) {
        int index = hit_test(x, y);
        
        // Create context menu
        HMENU menu = CreatePopupMenu();
        HMENU groupMenu = CreatePopupMenu();
        HMENU sortMenu = CreatePopupMenu();
        HMENU sizeMenu = CreatePopupMenu();
        HMENU resizeMenu = CreatePopupMenu();
        HMENU textMenu = CreatePopupMenu();
        
        // Playback options
        if (index >= 0) {
            AppendMenu(menu, MF_STRING, 1, L"Play");
            AppendMenu(menu, MF_STRING, 2, L"Add to Current Playlist");
            AppendMenu(menu, MF_STRING, 3, L"Add to New Playlist");
            AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        }
        
        // Resize mode menu
        AppendMenu(resizeMenu, MF_STRING | (m_config.resize_mode == ResizeMode::FIXED ? MF_CHECKED : 0), 
                   40, L"Fixed Size");
        AppendMenu(resizeMenu, MF_STRING | (m_config.resize_mode == ResizeMode::AUTO_FIT ? MF_CHECKED : 0), 
                   41, L"Auto-Fit (Minimize Gaps)");
        AppendMenu(resizeMenu, MF_STRING | (m_config.resize_mode == ResizeMode::AUTO_FILL ? MF_CHECKED : 0), 
                   42, L"Auto-Fill (Stretch to Fit)");
        AppendMenu(resizeMenu, MF_STRING | (m_config.resize_mode == ResizeMode::FLUID ? MF_CHECKED : 0), 
                   43, L"Fluid (Dynamic Everything)");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)resizeMenu, L"Resize Mode");
        
        // Group menu
        AppendMenu(groupMenu, MF_STRING | (m_config.group_mode == 0 ? MF_CHECKED : 0), 10, L"By Folder");
        AppendMenu(groupMenu, MF_STRING | (m_config.group_mode == 1 ? MF_CHECKED : 0), 11, L"By Album");
        AppendMenu(groupMenu, MF_STRING | (m_config.group_mode == 2 ? MF_CHECKED : 0), 12, L"By Artist");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)groupMenu, L"Group");
        
        // Sort menu
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == 0 ? MF_CHECKED : 0), 13, L"By Name");
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == 1 ? MF_CHECKED : 0), 14, L"By Date");
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == 2 ? MF_CHECKED : 0), 15, L"By Track Count");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)sortMenu, L"Sort");
        
        // Size menu (only for fixed mode)
        if (m_config.resize_mode == ResizeMode::FIXED) {
            AppendMenu(sizeMenu, MF_STRING, 16, L"Small (80px)");
            AppendMenu(sizeMenu, MF_STRING, 17, L"Medium (100px)");
            AppendMenu(sizeMenu, MF_STRING, 18, L"Normal (120px)");
            AppendMenu(sizeMenu, MF_STRING, 19, L"Large (150px)");
            AppendMenu(sizeMenu, MF_STRING, 20, L"Extra Large (200px)");
            AppendMenu(menu, MF_POPUP, (UINT_PTR)sizeMenu, L"Size");
        }
        
        // Text lines menu
        AppendMenu(textMenu, MF_STRING | (m_config.text_lines == 1 ? MF_CHECKED : 0), 21, L"Single Line");
        AppendMenu(textMenu, MF_STRING | (m_config.text_lines == 2 ? MF_CHECKED : 0), 22, L"Two Lines");
        AppendMenu(textMenu, MF_STRING | (m_config.text_lines == 3 ? MF_CHECKED : 0), 23, L"Three Lines");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)textMenu, L"Text Lines");
        
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING | (m_config.show_text ? MF_CHECKED : 0), 30, L"Show Labels");
        AppendMenu(menu, MF_STRING | (m_config.show_track_count ? MF_CHECKED : 0), 31, L"Show Track Count");
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING, 32, L"Select All");
        AppendMenu(menu, MF_STRING, 33, L"Refresh");
        
        // Show menu
        POINT pt = {x, y};
        ClientToScreen(m_hwnd, &pt);
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, nullptr);
        
        // Handle menu commands
        if (cmd >= 1 && cmd <= 3 && index >= 0) {
            static_api_ptr_t<playlist_manager> pm;
            if (cmd == 1) {
                // Play
                t_size playlist = pm->get_active_playlist();
                if (playlist != pfc::infinite_size) {
                    pm->playlist_clear(playlist);
                    pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                    pm->set_playing_playlist(playlist);
                    static_api_ptr_t<playback_control> pc;
                    pc->play_start();
                }
            } else if (cmd == 2) {
                // Add to current playlist
                t_size playlist = pm->get_active_playlist();
                if (playlist != pfc::infinite_size) {
                    pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                }
            } else if (cmd == 3) {
                // Add to new playlist
                t_size playlist = pm->create_playlist("New Playlist", pfc::infinite_size, pfc::infinite_size);
                pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
            }
        } else if (cmd >= 10 && cmd <= 12) {
            m_config.group_mode = cmd - 10;
            refresh_items();
        } else if (cmd >= 13 && cmd <= 15) {
            m_config.sort_mode = cmd - 13;
            refresh_items();
        } else if (cmd >= 16 && cmd <= 20) {
            int sizes[] = {80, 100, 120, 150, 200};
            m_config.item_size = sizes[cmd - 16];
            calculate_fluid_layout();
            m_callback->on_min_max_info_change();
            update_scrollbar();
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else if (cmd >= 21 && cmd <= 23) {
            m_config.text_lines = cmd - 20;
            update_scrollbar();
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else if (cmd == 30) {
            m_config.show_text = !m_config.show_text;
            update_scrollbar();
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else if (cmd == 31) {
            m_config.show_track_count = !m_config.show_track_count;
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else if (cmd == 32) {
            // Select all
            m_selected_indices.clear();
            for (size_t i = 0; i < m_items.size(); i++) {
                m_selected_indices.insert(i);
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else if (cmd == 33) {
            refresh_items();
        } else if (cmd >= 40 && cmd <= 43) {
            // Resize mode changes
            m_config.resize_mode = (ResizeMode)(cmd - 40);
            calculate_fluid_layout();
            
            // Recreate thumbnails for new size
            for (auto& item : m_items) {
                if (item->thumbnail) {
                    delete item->thumbnail;
                    item->thumbnail = nullptr;
                    create_thumbnail(item);
                }
            }
            
            update_scrollbar();
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        DestroyMenu(menu);
        return 0;
    }
    
    LRESULT on_keydown(WPARAM key) {
        if (key == VK_F5) {
            refresh_items();
        } else if (key == 'A' && GetKeyState(VK_CONTROL) & 0x8000) {
            // Ctrl+A: Select all
            m_selected_indices.clear();
            for (size_t i = 0; i < m_items.size(); i++) {
                m_selected_indices.insert(i);
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    LRESULT on_mousemove(int x, int y) {
        if (!m_tracking) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            TrackMouseEvent(&tme);
            m_tracking = true;
        }
        
        int index = hit_test(x, y);
        if (index != m_hover_index) {
            m_hover_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT on_mouseleave() {
        m_tracking = false;
        if (m_hover_index != -1) {
            m_hover_index = -1;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    LRESULT on_size() {
        calculate_fluid_layout();
        
        // Recreate thumbnails if size changed significantly
        static int last_size = 0;
        if (abs(last_size - m_calculated_item_size) > 10) {
            last_size = m_calculated_item_size;
            for (auto& item : m_items) {
                if (item->thumbnail) {
                    delete item->thumbnail;
                    item->thumbnail = nullptr;
                    create_thumbnail(item);
                }
            }
        }
        
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }
    
    LRESULT on_vscroll(int code) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_fluid_layout();
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_calculated_item_size + text_height;
        int cols = m_actual_columns;
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + m_actual_padding) + m_actual_padding;
        
        int old_pos = m_scroll_pos;
        int line_height = 30;
        int page_height = rc.bottom - rc.bottom % line_height;
        
        switch (code) {
            case SB_LINEUP: m_scroll_pos -= line_height; break;
            case SB_LINEDOWN: m_scroll_pos += line_height; break;
            case SB_PAGEUP: m_scroll_pos -= page_height; break;
            case SB_PAGEDOWN: m_scroll_pos += page_height; break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION: {
                SCROLLINFO si = {};
                si.cbSize = sizeof(si);
                si.fMask = SIF_TRACKPOS;
                GetScrollInfo(m_hwnd, SB_VERT, &si);
                m_scroll_pos = si.nTrackPos;
                break;
            }
        }
        
        m_scroll_pos = max(0, min(m_scroll_pos, max(0, total_height - rc.bottom)));
        
        if (m_scroll_pos != old_pos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT on_mousewheel(int delta, int keys) {
        // Ctrl+Wheel to resize (only in fixed mode)
        if (keys & MK_CONTROL && m_config.resize_mode == ResizeMode::FIXED) {
            int new_size = m_config.item_size;
            if (delta > 0) {
                new_size = min(250, new_size + 10);
            } else {
                new_size = max(60, new_size - 10);
            }
            
            if (new_size != m_config.item_size) {
                m_config.item_size = new_size;
                calculate_fluid_layout();
                m_callback->on_min_max_info_change();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        } else {
            // Smooth scrolling
            int lines = 3;
            for (int i = 0; i < lines; i++) {
                on_vscroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN);
            }
        }
        return 0;
    }
    
    void update_scrollbar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_fluid_layout();
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_calculated_item_size + text_height;
        int cols = m_actual_columns;
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + m_actual_padding) + m_actual_padding;
        
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = total_height - 1;
        si.nPage = rc.bottom;
        si.nPos = m_scroll_pos;
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }
    
    // ui_element_instance methods
    HWND get_wnd() override { return m_hwnd; }
    
    void set_configuration(ui_element_config::ptr config) override {
        m_config.load(config);
        calculate_fluid_layout();
        refresh_items();
    }
    
    ui_element_config::ptr get_configuration() override { 
        return m_config.save(g_get_guid());
    }
    
    GUID get_guid() override {
        return guid_element;
    }
    
    GUID get_subclass() override {
        return ui_element_subclass_utility;
    }
    
    ui_element_min_max_info get_min_max_info() override {
        ui_element_min_max_info info;
        info.m_min_width = 100;
        info.m_min_height = 100;
        info.m_max_width = ui_element_min_max_info::infinite_size;
        info.m_max_height = ui_element_min_max_info::infinite_size;
        return info;
    }
    
    // library_callback_dynamic methods
    void on_items_added(metadb_handle_list_cref p_data) override {
        refresh_items();
    }
    
    void on_items_removed(metadb_handle_list_cref p_data) override {
        refresh_items();
    }
    
    void on_items_modified(metadb_handle_list_cref p_data) override {
        refresh_items();
    }
};

// UI element factory
class album_grid_factory : public ui_element_impl<album_grid_instance> {
public:
    GUID get_guid() override {
        return guid_element;
    }
    
    void get_name(pfc::string_base& out) override {
        out = "Album Art Grid";
    }
    
    ui_element_config::ptr get_default_configuration() override {
        grid_config cfg;
        return cfg.save(get_guid());
    }
    
    const char* get_description() override {
        return "Album Art Grid v8.3.0 - Fluid Dynamic Resizing\n\n"
               "Displays your music library as a grid of album artwork with fluid resizing.\n\n"
               "Resize Modes:\n"
               "• Fixed - Traditional fixed size\n"
               "• Auto-Fit - Minimize gaps between items\n"
               "• Auto-Fill - Stretch to fill container\n"
               "• Fluid - Dynamic size and padding\n\n"
               "Features:\n"
               "• Automatic container fitting\n"
               "• Multi-select support (Ctrl/Shift+Click)\n"
               "• Full Unicode text support\n"
               "• Dark mode scrollbar support\n"
               "• Group by Folder/Album/Artist\n"
               "• Sort by Name/Date/Track Count\n"
               "• Ctrl+Wheel to resize (Fixed mode)\n"
               "• F5 to refresh";
    }
};

static ui_element_factory<album_grid_factory> g_album_grid_factory;

DECLARE_COMPONENT_VERSION("Album Art Grid", "8.3.0", 
    "Album Art Grid Component with Fluid Dynamic Resizing\n"
    "Automatically adjusts to perfectly fit container width\n\n"
    "Created with assistance from Anthropic's Claude\n"
    "https://github.com/yourusername/foobar-grid-view");

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");