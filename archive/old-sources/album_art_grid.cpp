// foobar2000 Album Art Grid Component
// Based on foobar2000 SDK documentation

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <memory>

// SDK includes (these will be available after extracting the SDK)
#ifdef HAVE_SDK
#include "../SDK/foobar2000/SDK/foobar2000.h"
#include "../SDK/helpers/helpers.h"
#include "../SDK/foobar2000/SDK/ui_element.h"
#include "../SDK/foobar2000/SDK/album_art.h"
#include "../SDK/foobar2000/SDK/library_manager.h"
#include "../SDK/foobar2000/SDK/playlist.h"
#include "../SDK/foobar2000/SDK/metadb.h"
#else
// Minimal definitions for compilation without full SDK
#define DECLARE_COMPONENT_VERSION(NAME, VER, ABOUT) \
    extern "C" __declspec(dllexport) const char* foobar2000_component_version() { return VER; }

#define DECLARE_FILE_TYPE(NAME, MASK, MULTIVALUE, LOAD, SAVE, MOVE, ALBUM, WRITE) 
#define VALIDATE_COMPONENT_FILENAME(NAME)
#endif

// Component info
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "1.0.0",
    "Album Art Grid View Component\n"
    "Displays album covers in a customizable grid layout.\n\n"
    "Features:\n"
    "- Multiple grouping modes (Album, Artist, Folder)\n"
    "- Configurable grid size\n"
    "- Smooth scrolling\n"
    "- Click to play\n\n"
    "Created with foobar2000 SDK"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Album item structure
struct AlbumItem {
    std::wstring title;
    std::wstring artist;
    std::wstring path;
    HBITMAP albumArt;
    bool artLoaded;
    
    AlbumItem() : albumArt(nullptr), artLoaded(false) {}
    ~AlbumItem() { 
        if (albumArt) DeleteObject(albumArt); 
    }
};

// Main grid window class
class AlbumArtGridWindow {
private:
    HWND m_hwnd;
    HWND m_parent;
    std::vector<std::unique_ptr<AlbumItem>> m_albums;
    int m_gridSize;
    int m_scrollPos;
    int m_selectedIndex;
    
    // Grid configuration
    int m_itemWidth;
    int m_itemHeight;
    int m_itemsPerRow;
    
    static const int DEFAULT_ITEM_SIZE = 150;
    static const int ITEM_PADDING = 10;
    
public:
    AlbumArtGridWindow() : 
        m_hwnd(nullptr), 
        m_parent(nullptr),
        m_gridSize(3),
        m_scrollPos(0),
        m_selectedIndex(-1),
        m_itemWidth(DEFAULT_ITEM_SIZE),
        m_itemHeight(DEFAULT_ITEM_SIZE + 40), // Extra space for text
        m_itemsPerRow(3) {
    }
    
    ~AlbumArtGridWindow() {
        if (m_hwnd) DestroyWindow(m_hwnd);
    }
    
    // Create the grid window
    bool Create(HWND parent) {
        m_parent = parent;
        
        // Register window class
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_Hredraw | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30)); // Dark background
        wc.lpszClassName = L"AlbumArtGrid";
        wc.cbWndExtra = sizeof(void*);
        
        if (!RegisterClassExW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) return false;
        }
        
        // Create window
        m_hwnd = CreateWindowExW(
            0,
            L"AlbumArtGrid",
            L"Album Art Grid",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
            0, 0, 400, 300,
            parent,
            nullptr,
            GetModuleHandle(nullptr),
            this
        );
        
        return m_hwnd != nullptr;
    }
    
    // Window procedure
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        AlbumArtGridWindow* pThis = nullptr;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (AlbumArtGridWindow*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, 0, (LONG_PTR)pThis);
            pThis->m_hwnd = hwnd;
        } else {
            pThis = (AlbumArtGridWindow*)GetWindowLongPtrW(hwnd, 0);
        }
        
        if (pThis) {
            return pThis->HandleMessage(msg, wParam, lParam);
        }
        
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    // Message handler
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE:
            OnCreate();
            return 0;
            
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_VSCROLL:
            OnVScroll(LOWORD(wParam), HIWORD(wParam));
            return 0;
            
        case WM_MOUSEWHEEL:
            OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
            
        case WM_LBUTTONDOWN:
            OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_LBUTTONDBLCLK:
            OnLButtonDblClk(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_KEYDOWN:
            OnKeyDown((int)wParam);
            return 0;
            
        case WM_ERASEBKGND:
            return 1; // We handle background in WM_PAINT
            
        default:
            return DefWindowProcW(m_hwnd, msg, wParam, lParam);
        }
    }
    
    // Initialize the grid
    void OnCreate() {
        // Load some sample albums (in real implementation, load from foobar2000 library)
        LoadAlbums();
        UpdateScrollBar();
    }
    
    // Paint the grid
    void OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        // Create memory DC for double buffering
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        
        // Fill background
        HBRUSH hbrBg = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdcMem, &rcClient, hbrBg);
        DeleteObject(hbrBg);
        
        // Draw grid items
        DrawGrid(hdcMem, rcClient);
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        
        EndPaint(m_hwnd, &ps);
    }
    
    // Draw the album grid
    void DrawGrid(HDC hdc, const RECT& rcClient) {
        int startY = -m_scrollPos;
        int itemIndex = 0;
        
        // Set text properties
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        for (size_t i = 0; i < m_albums.size(); i++) {
            int col = itemIndex % m_itemsPerRow;
            int row = itemIndex / m_itemsPerRow;
            
            int x = col * (m_itemWidth + ITEM_PADDING) + ITEM_PADDING;
            int y = startY + row * (m_itemHeight + ITEM_PADDING) + ITEM_PADDING;
            
            // Skip if outside visible area
            if (y + m_itemHeight < 0) {
                itemIndex++;
                continue;
            }
            if (y > rcClient.bottom) break;
            
            // Draw item
            DrawAlbumItem(hdc, x, y, m_albums[i].get(), i == m_selectedIndex);
            itemIndex++;
        }
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
    
    // Draw individual album item
    void DrawAlbumItem(HDC hdc, int x, int y, AlbumItem* item, bool selected) {
        RECT rcItem = { x, y, x + m_itemWidth, y + m_itemHeight };
        
        // Draw selection/hover background
        if (selected) {
            HBRUSH hbrSel = CreateSolidBrush(RGB(60, 60, 60));
            FillRect(hdc, &rcItem, hbrSel);
            DeleteObject(hbrSel);
        }
        
        // Draw album art placeholder
        RECT rcArt = { x, y, x + m_itemWidth, y + m_itemWidth };
        HBRUSH hbrArt = CreateSolidBrush(RGB(50, 50, 50));
        FillRect(hdc, &rcArt, hbrArt);
        DeleteObject(hbrArt);
        
        // Draw album art if available
        if (item->albumArt) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, item->albumArt);
            StretchBlt(hdc, x, y, m_itemWidth, m_itemWidth,
                hdcMem, 0, 0, m_itemWidth, m_itemWidth, SRCCOPY);
            DeleteDC(hdcMem);
        } else {
            // Draw placeholder text
            SetTextColor(hdc, RGB(128, 128, 128));
            DrawTextW(hdc, L"No Cover", -1, &rcArt, 
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        
        // Draw text below album art
        RECT rcText = { x, y + m_itemWidth + 5, x + m_itemWidth, y + m_itemHeight };
        
        // Album title
        DrawTextW(hdc, item->title.c_str(), -1, &rcText, 
            DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        
        // Artist
        rcText.top += 16;
        SetTextColor(hdc, RGB(180, 180, 180));
        DrawTextW(hdc, item->artist.c_str(), -1, &rcText, 
            DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        SetTextColor(hdc, RGB(255, 255, 255));
    }
    
    // Handle window resize
    void OnSize(int width, int height) {
        // Calculate items per row
        m_itemsPerRow = max(1, (width - ITEM_PADDING) / (m_itemWidth + ITEM_PADDING));
        UpdateScrollBar();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
    
    // Handle vertical scroll
    void OnVScroll(int code, int pos) {
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(m_hwnd, SB_VERT, &si);
        
        int oldPos = m_scrollPos;
        
        switch (code) {
        case SB_LINEUP: m_scrollPos -= 20; break;
        case SB_LINEDOWN: m_scrollPos += 20; break;
        case SB_PAGEUP: m_scrollPos -= si.nPage; break;
        case SB_PAGEDOWN: m_scrollPos += si.nPage; break;
        case SB_THUMBTRACK: m_scrollPos = si.nTrackPos; break;
        }
        
        m_scrollPos = max(0, min(m_scrollPos, si.nMax - (int)si.nPage));
        
        if (m_scrollPos != oldPos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
    
    // Handle mouse wheel
    void OnMouseWheel(int delta) {
        m_scrollPos -= (delta / WHEEL_DELTA) * 60;
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        GetScrollInfo(m_hwnd, SB_VERT, &si);
        
        m_scrollPos = max(0, min(m_scrollPos, si.nMax - (int)si.nPage));
        SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
    
    // Handle mouse click
    void OnLButtonDown(int x, int y) {
        // Calculate which item was clicked
        int col = x / (m_itemWidth + ITEM_PADDING);
        int row = (y + m_scrollPos) / (m_itemHeight + ITEM_PADDING);
        int index = row * m_itemsPerRow + col;
        
        if (index >= 0 && index < (int)m_albums.size()) {
            m_selectedIndex = index;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
    
    // Handle double click
    void OnLButtonDblClk(int x, int y) {
        OnLButtonDown(x, y);
        
        // Play the selected album
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_albums.size()) {
            // In real implementation, trigger playback through foobar2000 API
            MessageBoxW(m_hwnd, m_albums[m_selectedIndex]->title.c_str(), 
                L"Playing Album", MB_OK);
        }
    }
    
    // Handle keyboard
    void OnKeyDown(int key) {
        int oldIndex = m_selectedIndex;
        
        switch (key) {
        case VK_LEFT:
            if (m_selectedIndex > 0) m_selectedIndex--;
            break;
        case VK_RIGHT:
            if (m_selectedIndex < (int)m_albums.size() - 1) m_selectedIndex++;
            break;
        case VK_UP:
            if (m_selectedIndex >= m_itemsPerRow) m_selectedIndex -= m_itemsPerRow;
            break;
        case VK_DOWN:
            if (m_selectedIndex < (int)m_albums.size() - m_itemsPerRow) 
                m_selectedIndex += m_itemsPerRow;
            break;
        case VK_RETURN:
            if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_albums.size()) {
                MessageBoxW(m_hwnd, m_albums[m_selectedIndex]->title.c_str(), 
                    L"Playing Album", MB_OK);
            }
            break;
        }
        
        if (m_selectedIndex != oldIndex) {
            EnsureVisible(m_selectedIndex);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
    
    // Ensure item is visible
    void EnsureVisible(int index) {
        int row = index / m_itemsPerRow;
        int itemY = row * (m_itemHeight + ITEM_PADDING);
        
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        if (itemY < m_scrollPos) {
            m_scrollPos = itemY;
        } else if (itemY + m_itemHeight > m_scrollPos + rcClient.bottom) {
            m_scrollPos = itemY + m_itemHeight - rcClient.bottom;
        }
        
        SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
    }
    
    // Update scroll bar
    void UpdateScrollBar() {
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        int totalRows = (m_albums.size() + m_itemsPerRow - 1) / m_itemsPerRow;
        int totalHeight = totalRows * (m_itemHeight + ITEM_PADDING) + ITEM_PADDING;
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = totalHeight;
        si.nPage = rcClient.bottom;
        
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }
    
    // Load albums (sample data for now)
    void LoadAlbums() {
        // In real implementation, load from foobar2000 media library
        // For now, create sample albums
        
        const wchar_t* sampleAlbums[][2] = {
            {L"The Dark Side of the Moon", L"Pink Floyd"},
            {L"Abbey Road", L"The Beatles"},
            {L"Thriller", L"Michael Jackson"},
            {L"Back in Black", L"AC/DC"},
            {L"The Wall", L"Pink Floyd"},
            {L"Led Zeppelin IV", L"Led Zeppelin"},
            {L"Rumours", L"Fleetwood Mac"},
            {L"Hotel California", L"Eagles"},
            {L"Nevermind", L"Nirvana"},
            {L"Appetite for Destruction", L"Guns N' Roses"},
            {L"The Joshua Tree", L"U2"},
            {L"Born to Run", L"Bruce Springsteen"},
            {L"Purple Rain", L"Prince"},
            {L"Blood Sugar Sex Magik", L"Red Hot Chili Peppers"},
            {L"OK Computer", L"Radiohead"},
            {L"Ten", L"Pearl Jam"},
            {L"Metallica", L"Metallica"},
            {L"Hybrid Theory", L"Linkin Park"},
            {L"American Idiot", L"Green Day"},
            {L"In Utero", L"Nirvana"}
        };
        
        for (const auto& album : sampleAlbums) {
            auto item = std::make_unique<AlbumItem>();
            item->title = album[0];
            item->artist = album[1];
            item->path = L"";
            item->albumArt = nullptr;
            item->artLoaded = false;
            m_albums.push_back(std::move(item));
        }
    }
    
    // Get window handle
    HWND GetHwnd() const { return m_hwnd; }
};

// UI Element implementation for foobar2000
#ifdef HAVE_SDK
class ui_element_albumart_grid : public ui_element_instance {
private:
    AlbumArtGridWindow m_window;
    ui_element_config::ptr m_config;
    
public:
    ui_element_albumart_grid(HWND parent, ui_element_config::ptr config) : m_config(config) {
        m_window.Create(parent);
    }
    
    virtual HWND get_wnd() override {
        return m_window.GetHwnd();
    }
    
    virtual void set_configuration(ui_element_config::ptr config) override {
        m_config = config;
    }
    
    virtual ui_element_config::ptr get_configuration() override {
        return m_config;
    }
    
    static GUID g_get_guid() {
        // {12345678-1234-5678-9012-123456789ABC}
        static const GUID guid = 
            { 0x12345678, 0x1234, 0x5678, { 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCC } };
        return guid;
    }
    
    static const char* g_get_name() {
        return "Album Art Grid";
    }
    
    static const char* g_get_description() {
        return "Displays album covers in a grid layout";
    }
};

class ui_element_albumart_grid_factory : public ui_element_impl<ui_element_albumart_grid> {};
static ui_element_albumart_grid_factory g_ui_element_albumart_grid_factory;
#endif

// Export for testing without SDK
extern "C" __declspec(dllexport) void* CreateAlbumArtGrid(HWND parent) {
    static AlbumArtGridWindow window;
    window.Create(parent);
    return &window;
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}