#pragma once

#include <foobar2000.h>
#include <helpers/foobar2000+atl.h>
#include <atlbase.h>
#include <atlwin.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

// Forward declarations
class AlbumArtCache;
class GridRenderer;
class ConfigDialog;

// Plugin version information
#define PLUGIN_NAME "Album Art Grid View"
#define PLUGIN_VERSION "1.0.0"
#define COMPONENT_VERSION "1,0,0,0"

// Grid item structure
struct GridItem {
    pfc::string8 album;
    pfc::string8 artist;
    pfc::string8 genre;
    pfc::string8 year;
    pfc::string8 folder_path;
    metadb_handle_list tracks;
    HBITMAP album_art;
    bool art_loading;
    bool art_loaded;
    
    GridItem() : album_art(nullptr), art_loading(false), art_loaded(false) {}
    ~GridItem() {
        if (album_art) {
            DeleteObject(album_art);
            album_art = nullptr;
        }
    }
};

// Grouping modes
enum class GroupingMode {
    BY_ALBUM,
    BY_ARTIST,
    BY_GENRE, 
    BY_YEAR,
    BY_FOLDER
};

// Grid size modes
enum class GridSize {
    SMALL = 128,
    MEDIUM = 192,
    LARGE = 256
};

// Plugin configuration
struct PluginConfig {
    GroupingMode grouping_mode = GroupingMode::BY_ALBUM;
    GridSize grid_size = GridSize::MEDIUM;
    bool show_text_overlay = true;
    bool dark_theme = true;
    bool smooth_scrolling = true;
    int cache_size_mb = 100;
    
    void save();
    void load();
};

// Main UI Element class
class AlbumArtGridPanel : 
    public ui_element_instance,
    public ui_element_instance_callback,
    public CWindowImpl<AlbumArtGridPanel>,
    public play_callback_impl_base
{
public:
    // UI Element interface
    HWND get_wnd() override { return m_hWnd; }
    void set_configuration(stream_reader* p_reader, t_size p_size, abort_callback& p_abort) override;
    void get_configuration(stream_writer* p_writer, abort_callback& p_abort) const override;
    void initialize_window(HWND parent) override;
    
    // Window message map
    BEGIN_MSG_MAP_EX(AlbumArtGridPanel)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_NCDESTROY(OnNcDestroy)
        MSG_WM_SIZE(OnSize)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_RBUTTONDOWN(OnRButtonDown)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_SETFOCUS(OnSetFocus)
        MSG_WM_KILLFOCUS(OnKillFocus)
        MSG_WM_TIMER(OnTimer)
    END_MSG_MAP()

    // Message handlers
    LRESULT OnCreate(LPCREATESTRUCT lpcs);
    void OnDestroy();
    void OnNcDestroy();
    void OnSize(UINT nType, CSize size);
    void OnPaint(CDCHandle dc);
    BOOL OnEraseBkgnd(CDCHandle dc);
    void OnLButtonDown(UINT nFlags, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    void OnRButtonDown(UINT nFlags, CPoint point);
    void OnMouseMove(UINT nFlags, CPoint point);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnSetFocus(HWND hWndOther);
    void OnKillFocus(HWND hWndNew);
    void OnTimer(UINT_PTR nIDEvent);

    // Play callback interface
    void on_playback_new_track(metadb_handle_ptr p_track) override;
    void on_playback_stop(play_control::t_stop_reason p_reason) override;
    void on_playback_pause(bool p_state) override;

    // UI Element callback interface - CRITICAL FOR SHUTDOWN
    void on_host_shutdown() override;

    // Public methods
    void RefreshLibrary();
    void SetGroupingMode(GroupingMode mode);
    void SetGridSize(GridSize size);
    void ShowConfig();
    void SetFilter(const char* filter_text);

private:
    // Private methods
    void InitializeDirect2D();
    void CleanupDirect2D();
    void CreateGridItems();
    void UpdateLayout();
    void InvalidateGrid();
    int GetItemAtPoint(CPoint pt);
    void EnsureItemVisible(int item_index);
    void UpdateScrollInfo();
    void ScrollToItem(int item_index);
    void PlayItem(int item_index);
    void AddItemToPlaylist(int item_index);
    void ShowContextMenu(CPoint pt, int item_index);
    void LoadAlbumArt();
    void ApplyFilter();

    // Private members
    std::unique_ptr<AlbumArtCache> m_cache;
    std::unique_ptr<GridRenderer> m_renderer;
    std::vector<std::unique_ptr<GridItem>> m_items;
    std::vector<int> m_filtered_indices;
    
    PluginConfig m_config;
    pfc::string8 m_filter_text;
    
    // Direct2D resources
    ID2D1Factory* m_d2d_factory;
    ID2D1HwndRenderTarget* m_render_target;
    IDWriteFactory* m_dwrite_factory;
    IDWriteTextFormat* m_text_format;
    
    // UI state
    int m_scroll_pos;
    int m_items_per_row;
    int m_visible_rows;
    int m_total_rows;
    int m_item_width;
    int m_item_height;
    int m_hover_item;
    int m_selected_item;
    bool m_has_focus;
    
    // Threading
    std::thread m_loader_thread;
    std::mutex m_items_mutex;
    std::condition_variable m_loader_cv;
    bool m_loader_stop;
    
    // Constants
    static const int TIMER_REFRESH = 1;
    static const int TIMER_INVALIDATE = 2;
    static const int SCROLL_LINE_SIZE = 48;
    static const int ITEM_SPACING = 8;
    static const int TEXT_HEIGHT = 40;
};

// UI Element factory
class AlbumArtGridElement : public ui_element_impl_withpopup<AlbumArtGridPanel> {
public:
    static const GUID g_guid;
    static const char* g_get_name() { return PLUGIN_NAME; }
    static const char* g_get_description() { return "Album art grid view for foobar2000"; }
    static GUID g_get_guid() { return g_guid; }
    static GUID g_get_subclass() { return ui_element_subclass_utility; }
    static ui_element_config::ptr g_get_default_configuration() {
        return ui_element_config::g_create_empty(g_guid);
    }
};

// Configuration dialog class
class ConfigDialog : public CDialogImpl<ConfigDialog> {
public:
    enum { IDD = IDD_CONFIG };
    
    BEGIN_MSG_MAP_EX(ConfigDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER_EX(IDOK, OnOK)
        COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
        COMMAND_HANDLER_EX(IDC_GROUPING_COMBO, CBN_SELCHANGE, OnGroupingChange)
        COMMAND_HANDLER_EX(IDC_SIZE_COMBO, CBN_SELCHANGE, OnSizeChange)
        COMMAND_HANDLER_EX(IDC_THEME_CHECK, BN_CLICKED, OnThemeChange)
    END_MSG_MAP()

    ConfigDialog(PluginConfig& config) : m_config(config) {}

    BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
    void OnOK(UINT uNotifyCode, int nID, HWND hWndCtl);
    void OnCancel(UINT uNotifyCode, int nID, HWND hWndCtl);
    void OnGroupingChange(UINT uNotifyCode, int nID, HWND hWndCtl);
    void OnSizeChange(UINT uNotifyCode, int nID, HWND hWndCtl);
    void OnThemeChange(UINT uNotifyCode, int nID, HWND hWndCtl);

private:
    PluginConfig& m_config;
};

// Resource IDs
#define IDD_CONFIG 1000
#define IDC_GROUPING_COMBO 1001
#define IDC_SIZE_COMBO 1002
#define IDC_THEME_CHECK 1003
#define IDC_OVERLAY_CHECK 1004
#define IDC_CACHE_EDIT 1005