// Minimal Album Art Grid Component for foobar2000
// This version works without ATL dependencies

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <memory>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

// Minimal foobar2000 SDK definitions needed
namespace foobar2000 {
    typedef size_t t_size;
    typedef unsigned char t_uint8;
    typedef unsigned int t_uint32;
    
    class NOVTABLE service_base {
    public:
        virtual ~service_base() {}
    };
    
    class NOVTABLE ui_element_instance : public service_base {
    public:
        virtual void initialize_window(HWND parent) = 0;
        virtual HWND get_wnd() = 0;
        virtual void set_configuration(void* config) = 0;
        virtual void* get_configuration() = 0;
        virtual void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) {}
    };
    
    struct ui_element_instance_callback {
        virtual COLORREF query_std_color(int which) { return RGB(255,255,255); }
        virtual HFONT query_font_ex(int which) { return (HFONT)GetStockObject(DEFAULT_GUI_FONT); }
    };
    
    typedef ui_element_instance_callback* ui_element_instance_callback_ptr;
}

using namespace foobar2000;

// Album info structure
struct AlbumInfo {
    std::wstring artist;
    std::wstring album;
    COLORREF color;
};

// Main window class
class CAlbumArtGridWindow : public ui_element_instance {
private:
    HWND m_hwnd;
    HWND m_parent;
    ui_element_instance_callback_ptr m_callback;
    std::vector<AlbumInfo> m_albums;
    int m_selectedIndex;
    int m_scrollPos;
    int m_itemSize;
    int m_itemsPerRow;
    
    static const int ITEM_SIZE = 120;
    static const int ITEM_PADDING = 10;
    static const int TEXT_HEIGHT = 35;
    
public:
    CAlbumArtGridWindow(void* config, ui_element_instance_callback_ptr callback) 
        : m_hwnd(NULL), m_parent(NULL), m_callback(callback), 
          m_selectedIndex(-1), m_scrollPos(0), m_itemSize(ITEM_SIZE), m_itemsPerRow(3) {
        LoadSampleAlbums();
    }
    
    virtual ~CAlbumArtGridWindow() {
        if (m_hwnd) DestroyWindow(m_hwnd);
    }
    
    // Load sample albums
    void LoadSampleAlbums() {
        const wchar_t* samples[][2] = {
            {L"Pink Floyd", L"Dark Side of the Moon"},
            {L"The Beatles", L"Abbey Road"},
            {L"Led Zeppelin", L"Led Zeppelin IV"},
            {L"Queen", L"A Night at the Opera"},
            {L"Nirvana", L"Nevermind"},
            {L"Radiohead", L"OK Computer"},
            {L"Metallica", L"Master of Puppets"},
            {L"AC/DC", L"Back in Black"},
            {L"The Who", L"Who's Next"},
            {L"Pink Floyd", L"The Wall"},
            {L"U2", L"The Joshua Tree"},
            {L"Dire Straits", L"Brothers in Arms"}
        };
        
        for (int i = 0; i < 12; i++) {
            AlbumInfo album;
            album.artist = samples[i][0];
            album.album = samples[i][1];
            album.color = RGB(60 + (i*20)%100, 70 + (i*15)%100, 80 + (i*25)%100);
            m_albums.push_back(album);
        }
    }
    
    // UI element interface
    virtual void initialize_window(HWND parent) override {
        m_parent = parent;
        
        WNDCLASSW wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"AlbumArtGridClass";
        
        RegisterClassW(&wc);
        
        m_hwnd = CreateWindowW(
            L"AlbumArtGridClass",
            L"Album Art Grid",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS,
            0, 0, 100, 100,
            parent, NULL,
            GetModuleHandle(NULL),
            this
        );
        
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        UpdateScrollBar();
    }
    
    virtual HWND get_wnd() override { return m_hwnd; }
    virtual void set_configuration(void* config) override {}
    virtual void* get_configuration() override { return NULL; }
    
    // Window procedure
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        CAlbumArtGridWindow* pThis = (CAlbumArtGridWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (CAlbumArtGridWindow*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        
        if (!pThis) return DefWindowProcW(hwnd, msg, wParam, lParam);
        
        switch (msg) {
        case WM_SIZE:
            pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_PAINT:
            pThis->OnPaint();
            return 0;
            
        case WM_ERASEBKGND:
            return 1;
            
        case WM_VSCROLL:
            pThis->OnVScroll(LOWORD(wParam));
            return 0;
            
        case WM_MOUSEWHEEL:
            pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
            
        case WM_LBUTTONDOWN:
            pThis->OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_LBUTTONDBLCLK:
            pThis->OnLButtonDblClk(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    void OnSize(int width, int height) {
        m_itemsPerRow = max(1, (width - ITEM_PADDING) / (m_itemSize + ITEM_PADDING));
        UpdateScrollBar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        // Create memory DC for double buffering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        
        // Fill background
        HBRUSH hbrBg = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdcMem, &rcClient, hbrBg);
        DeleteObject(hbrBg);
        
        // Set text properties
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
        
        // Draw albums
        int startY = -m_scrollPos;
        for (size_t i = 0; i < m_albums.size(); i++) {
            int col = i % m_itemsPerRow;
            int row = i / m_itemsPerRow;
            
            int x = col * (m_itemSize + ITEM_PADDING) + ITEM_PADDING;
            int y = startY + row * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING) + ITEM_PADDING;
            
            if (y + m_itemSize + TEXT_HEIGHT < 0) continue;
            if (y > rcClient.bottom) break;
            
            DrawAlbum(hdcMem, x, y, i);
        }
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(hdcMem, hOldFont);
        DeleteObject(hFont);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        
        EndPaint(m_hwnd, &ps);
    }
    
    void DrawAlbum(HDC hdc, int x, int y, size_t index) {
        const AlbumInfo& album = m_albums[index];
        
        // Draw selection
        if ((int)index == m_selectedIndex) {
            RECT rcSel = { x-2, y-2, x + m_itemSize + 2, y + m_itemSize + TEXT_HEIGHT + 2 };
            HBRUSH hbrSel = CreateSolidBrush(RGB(100, 100, 100));
            FillRect(hdc, &rcSel, hbrSel);
            DeleteObject(hbrSel);
        }
        
        // Draw album art placeholder
        RECT rcArt = { x, y, x + m_itemSize, y + m_itemSize };
        HBRUSH hbrArt = CreateSolidBrush(album.color);
        FillRect(hdc, &rcArt, hbrArt);
        DeleteObject(hbrArt);
        
        // Draw text
        RECT rcText = { x, y + m_itemSize + 2, x + m_itemSize, y + m_itemSize + TEXT_HEIGHT };
        
        // Album name
        DrawTextW(hdc, album.album.c_str(), -1, &rcText, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        
        // Artist
        SetTextColor(hdc, RGB(180, 180, 180));
        rcText.top += 16;
        DrawTextW(hdc, album.artist.c_str(), -1, &rcText, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        SetTextColor(hdc, RGB(255, 255, 255));
    }
    
    void OnVScroll(int code) {
        int oldPos = m_scrollPos;
        
        switch (code) {
        case SB_LINEUP: m_scrollPos -= 20; break;
        case SB_LINEDOWN: m_scrollPos += 20; break;
        case SB_PAGEUP: m_scrollPos -= 100; break;
        case SB_PAGEDOWN: m_scrollPos += 100; break;
        case SB_THUMBTRACK:
            SCROLLINFO si = {0};
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(m_hwnd, SB_VERT, &si);
            m_scrollPos = si.nTrackPos;
            break;
        }
        
        m_scrollPos = max(0, m_scrollPos);
        
        if (m_scrollPos != oldPos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void OnMouseWheel(int delta) {
        m_scrollPos -= (delta / WHEEL_DELTA) * 40;
        m_scrollPos = max(0, m_scrollPos);
        SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void OnLButtonDown(int x, int y) {
        int col = x / (m_itemSize + ITEM_PADDING);
        int row = (y + m_scrollPos) / (m_itemSize + TEXT_HEIGHT + ITEM_PADDING);
        int index = row * m_itemsPerRow + col;
        
        if (index >= 0 && index < (int)m_albums.size() && col < m_itemsPerRow) {
            m_selectedIndex = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void OnLButtonDblClk(int x, int y) {
        OnLButtonDown(x, y);
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_albums.size()) {
            MessageBoxW(m_hwnd, m_albums[m_selectedIndex].album.c_str(), L"Playing", MB_OK);
        }
    }
    
    void UpdateScrollBar() {
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        int rows = (m_albums.size() + m_itemsPerRow - 1) / m_itemsPerRow;
        int totalHeight = rows * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING) + ITEM_PADDING;
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = totalHeight;
        si.nPage = rcClient.bottom;
        
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }
};

// Component GUID
static const GUID guid_album_art_grid = 
    { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x91 } };

// Global instance
static CAlbumArtGridWindow* g_instance = NULL;

// Export functions for foobar2000
extern "C" {
    __declspec(dllexport) void foobar2000_get_interface(void** ppv, const char* name) {
        if (ppv) *ppv = NULL;
    }
    
    __declspec(dllexport) const char* foobar2000_component_version() {
        return "1.0.0";
    }
    
    __declspec(dllexport) int foobar2000_get_interface_version() {
        return 80;
    }
    
    // Create window for testing
    __declspec(dllexport) HWND CreateAlbumArtGrid(HWND parent) {
        if (g_instance) delete g_instance;
        g_instance = new CAlbumArtGridWindow(NULL, NULL);
        g_instance->initialize_window(parent);
        return g_instance->get_wnd();
    }
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        if (g_instance) {
            delete g_instance;
            g_instance = NULL;
        }
    }
    return TRUE;
}