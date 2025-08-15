// Album Art Grid Component for foobar2000
// Based on foobar2000 SDK 2025-03-07

#include <helpers/foobar2000+atl.h>
#include <helpers/album_art_helpers.h>
#include <libPPUI/win32_op.h>
#include <helpers/BumpableElem.h>
#include <vector>
#include <memory>

namespace {
    // Component GUID - unique identifier for our UI element
    static const GUID guid_album_art_grid = 
        { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } };

    struct AlbumInfo {
        pfc::string8 artist;
        pfc::string8 album;
        pfc::string8 path;
        metadb_handle_ptr handle;
    };

    class CAlbumArtGridWindow : public ui_element_instance, public CWindowImpl<CAlbumArtGridWindow> {
    public:
        // ATL window class declaration
        DECLARE_WND_CLASS_EX(TEXT("{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}"), CS_VREDRAW | CS_HREDRAW, (-1));

        CAlbumArtGridWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback);
        
        // ui_element_instance methods
        void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
        HWND get_wnd() { return *this; }
        void set_configuration(ui_element_config::ptr config) { m_config = config; }
        ui_element_config::ptr get_configuration() { return m_config; }
        
        // Static element info
        static GUID g_get_guid() { return guid_album_art_grid; }
        static GUID g_get_subclass() { return ui_element_subclass_media_library_viewers; }
        static void g_get_name(pfc::string_base & out) { out = "Album Art Grid"; }
        static const char * g_get_description() { return "Displays album covers in a grid layout"; }
        static ui_element_config::ptr g_get_default_configuration() { 
            return ui_element_config::g_create_empty(g_get_guid()); 
        }

        void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);

        // Message map
        BEGIN_MSG_MAP_EX(CAlbumArtGridWindow)
            MSG_WM_CREATE(OnCreate)
            MSG_WM_DESTROY(OnDestroy)
            MSG_WM_SIZE(OnSize)
            MSG_WM_ERASEBKGND(OnEraseBkgnd)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_LBUTTONDOWN(OnLButtonDown)
            MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
            MSG_WM_VSCROLL(OnVScroll)
            MSG_WM_MOUSEWHEEL(OnMouseWheel)
            MSG_WM_CONTEXTMENU(OnContextMenu)
        END_MSG_MAP()

    private:
        // Message handlers
        int OnCreate(LPCREATESTRUCT lpCreateStruct);
        void OnDestroy();
        void OnSize(UINT nType, CSize size);
        BOOL OnEraseBkgnd(CDCHandle dc);
        void OnPaint(CDCHandle dc);
        void OnLButtonDown(UINT nFlags, CPoint point);
        void OnLButtonDblClk(UINT nFlags, CPoint point);
        void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
        BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
        void OnContextMenu(CWindow wnd, CPoint point);

        // Helper methods
        void LoadAlbums();
        void DrawAlbumItem(CDCHandle dc, int x, int y, size_t index);
        int HitTest(CPoint point);
        void UpdateScrollBar();
        void PlayAlbum(size_t index);

        // Member variables
        ui_element_config::ptr m_config;
        const ui_element_instance_callback_ptr m_callback;
        
        std::vector<AlbumInfo> m_albums;
        int m_itemSize;
        int m_itemsPerRow;
        int m_scrollPos;
        int m_selectedIndex;
        
        static const int ITEM_SIZE = 150;
        static const int ITEM_PADDING = 10;
        static const int TEXT_HEIGHT = 35;
    };

    // Constructor
    CAlbumArtGridWindow::CAlbumArtGridWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) 
        : m_callback(p_callback), m_config(config), m_itemSize(ITEM_SIZE), m_scrollPos(0), m_selectedIndex(-1), m_itemsPerRow(1) {
    }

    // Window creation
    int CAlbumArtGridWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) {
        LoadAlbums();
        UpdateScrollBar();
        return 0;
    }

    void CAlbumArtGridWindow::OnDestroy() {
        m_albums.clear();
    }

    // Load albums from library
    void CAlbumArtGridWindow::LoadAlbums() {
        m_albums.clear();
        
        try {
            // Get library manager
            auto api = library_manager::get();
            metadb_handle_list items;
            api->get_all_items(items);
            
            // Group by album
            pfc::map_t<pfc::string8, AlbumInfo> albumMap;
            
            for (size_t i = 0; i < items.get_count(); ++i) {
                metadb_handle_ptr item = items[i];
                
                const file_info* info = NULL;
                if (item->get_info_ref(info)) {
                    AlbumInfo album;
                    
                    const char* artist = info->meta_get("artist", 0);
                    const char* albumName = info->meta_get("album", 0);
                    
                    if (artist) album.artist = artist;
                    if (albumName) album.album = albumName;
                    
                    if (!album.album.is_empty()) {
                        pfc::string8 key;
                        key << album.artist << " - " << album.album;
                        
                        if (!albumMap.have_item(key)) {
                            album.handle = item;
                            album.path = item->get_path();
                            albumMap.set(key, album);
                        }
                    }
                }
            }
            
            // Convert map to vector
            for (auto iter = albumMap.first(); iter.is_valid(); ++iter) {
                m_albums.push_back(iter->m_value);
            }
            
        } catch (exception_service_not_found &) {
            // Library not available - add sample data
            AlbumInfo sample;
            sample.artist = "Sample Artist";
            sample.album = "Sample Album";
            m_albums.push_back(sample);
        }
        
        // If no albums found, add samples
        if (m_albums.empty()) {
            const char* samples[][2] = {
                {"Pink Floyd", "The Dark Side of the Moon"},
                {"The Beatles", "Abbey Road"},
                {"Led Zeppelin", "Led Zeppelin IV"},
                {"Nirvana", "Nevermind"},
                {"Radiohead", "OK Computer"}
            };
            
            for (auto& s : samples) {
                AlbumInfo album;
                album.artist = s[0];
                album.album = s[1];
                m_albums.push_back(album);
            }
        }
    }

    // Size handling
    void CAlbumArtGridWindow::OnSize(UINT nType, CSize size) {
        m_itemsPerRow = max(1, (size.cx - ITEM_PADDING) / (m_itemSize + ITEM_PADDING));
        UpdateScrollBar();
        Invalidate();
    }

    // Background painting
    BOOL CAlbumArtGridWindow::OnEraseBkgnd(CDCHandle dc) {
        CRect rc;
        GetClientRect(&rc);
        CBrush brush;
        brush.CreateSolidBrush(m_callback->query_std_color(ui_color_background));
        dc.FillRect(&rc, brush);
        return TRUE;
    }

    // Main painting
    void CAlbumArtGridWindow::OnPaint(CDCHandle) {
        CPaintDC dc(*this);
        
        CRect rcClient;
        GetClientRect(&rcClient);
        
        // Set text properties
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(m_callback->query_std_color(ui_color_text));
        SelectObjectScope fontScope(dc, (HGDIOBJ)m_callback->query_font_ex(ui_font_default));
        
        // Draw visible items
        int startY = -m_scrollPos;
        
        for (size_t i = 0; i < m_albums.size(); ++i) {
            int col = i % m_itemsPerRow;
            int row = i / m_itemsPerRow;
            
            int x = col * (m_itemSize + ITEM_PADDING) + ITEM_PADDING;
            int y = startY + row * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING) + ITEM_PADDING;
            
            // Skip if outside visible area
            if (y + m_itemSize + TEXT_HEIGHT < 0) continue;
            if (y > rcClient.bottom) break;
            
            DrawAlbumItem(dc, x, y, i);
        }
    }

    // Draw individual album
    void CAlbumArtGridWindow::DrawAlbumItem(CDCHandle dc, int x, int y, size_t index) {
        if (index >= m_albums.size()) return;
        
        const AlbumInfo& album = m_albums[index];
        
        // Draw selection
        if (index == m_selectedIndex) {
            CRect rcSel(x - 2, y - 2, x + m_itemSize + 2, y + m_itemSize + TEXT_HEIGHT + 2);
            CBrush selBrush;
            selBrush.CreateSolidBrush(m_callback->query_std_color(ui_color_selection));
            dc.FrameRect(&rcSel, selBrush);
        }
        
        // Draw album art placeholder
        CRect rcArt(x, y, x + m_itemSize, y + m_itemSize);
        CBrush artBrush;
        artBrush.CreateSolidBrush(RGB(60, 60, 60));
        dc.FillRect(&rcArt, artBrush);
        
        // Draw "No Cover" text in placeholder
        dc.SetTextColor(RGB(128, 128, 128));
        dc.DrawText(_T("No Cover"), -1, &rcArt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // Draw album info
        CRect rcText(x, y + m_itemSize + 2, x + m_itemSize, y + m_itemSize + TEXT_HEIGHT);
        
        dc.SetTextColor(m_callback->query_std_color(ui_color_text));
        
        // Album name
        pfc::stringcvt::string_os_from_utf8 albumText(album.album);
        dc.DrawText(albumText, -1, &rcText, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        
        // Artist name
        rcText.top += 16;
        dc.SetTextColor(m_callback->query_std_color(ui_color_text_secondary));
        pfc::stringcvt::string_os_from_utf8 artistText(album.artist);
        dc.DrawText(artistText, -1, &rcText, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    // Mouse handling
    void CAlbumArtGridWindow::OnLButtonDown(UINT nFlags, CPoint point) {
        int index = HitTest(point);
        if (index >= 0 && index < (int)m_albums.size()) {
            m_selectedIndex = index;
            Invalidate();
        }
    }

    void CAlbumArtGridWindow::OnLButtonDblClk(UINT nFlags, CPoint point) {
        int index = HitTest(point);
        if (index >= 0 && index < (int)m_albums.size()) {
            PlayAlbum(index);
        }
    }

    int CAlbumArtGridWindow::HitTest(CPoint point) {
        int col = point.x / (m_itemSize + ITEM_PADDING);
        int row = (point.y + m_scrollPos) / (m_itemSize + TEXT_HEIGHT + ITEM_PADDING);
        int index = row * m_itemsPerRow + col;
        
        if (col >= m_itemsPerRow) return -1;
        if (index >= (int)m_albums.size()) return -1;
        
        return index;
    }

    // Scrolling
    void CAlbumArtGridWindow::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar) {
        int oldPos = m_scrollPos;
        
        switch (nSBCode) {
        case SB_LINEUP: m_scrollPos -= 20; break;
        case SB_LINEDOWN: m_scrollPos += 20; break;
        case SB_PAGEUP: m_scrollPos -= 100; break;
        case SB_PAGEDOWN: m_scrollPos += 100; break;
        case SB_THUMBTRACK: m_scrollPos = nPos; break;
        }
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        GetScrollInfo(SB_VERT, &si);
        
        m_scrollPos = max(0, min(m_scrollPos, si.nMax - (int)si.nPage));
        
        if (m_scrollPos != oldPos) {
            SetScrollPos(SB_VERT, m_scrollPos, TRUE);
            Invalidate();
        }
    }

    BOOL CAlbumArtGridWindow::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
        m_scrollPos -= (zDelta / WHEEL_DELTA) * 60;
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        GetScrollInfo(SB_VERT, &si);
        
        m_scrollPos = max(0, min(m_scrollPos, si.nMax - (int)si.nPage));
        SetScrollPos(SB_VERT, m_scrollPos, TRUE);
        Invalidate();
        
        return TRUE;
    }

    void CAlbumArtGridWindow::UpdateScrollBar() {
        CRect rcClient;
        GetClientRect(&rcClient);
        
        int rows = (m_albums.size() + m_itemsPerRow - 1) / m_itemsPerRow;
        int totalHeight = rows * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING) + ITEM_PADDING;
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = totalHeight;
        si.nPage = rcClient.bottom;
        
        SetScrollInfo(SB_VERT, &si, TRUE);
    }

    // Context menu
    void CAlbumArtGridWindow::OnContextMenu(CWindow wnd, CPoint point) {
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_albums.size()) {
            // Would show context menu here
        }
    }

    // Play album
    void CAlbumArtGridWindow::PlayAlbum(size_t index) {
        if (index < m_albums.size() && m_albums[index].handle.is_valid()) {
            static_api_ptr_t<playlist_manager> pm;
            static_api_ptr_t<play_control> pc;
            
            metadb_handle_list items;
            items.add_item(m_albums[index].handle);
            
            size_t playlist = pm->get_active_playlist();
            pm->playlist_clear(playlist);
            pm->playlist_add_items(playlist, items, bit_array_false());
            pc->play_start();
        }
    }

    // Notifications
    void CAlbumArtGridWindow::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) {
        if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed) {
            Invalidate();
        }
    }

    // UI element implementation
    class ui_element_album_art_grid : public ui_element_impl_withpopup<CAlbumArtGridWindow> {};
    
    // Service factory
    static service_factory_single_t<ui_element_album_art_grid> g_ui_element_album_art_grid_factory;
}