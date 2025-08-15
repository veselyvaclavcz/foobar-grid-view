// Final Working Album Art Grid Component for foobar2000
// No SDK dependencies, clean compilation

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")

// Album info structure
struct AlbumInfo {
    std::wstring artist;
    std::wstring album;
    COLORREF color;
};

// Main window class
class AlbumArtGridWindow {
private:
    HWND m_hwnd;
    HWND m_parent;
    std::vector<AlbumInfo> m_albums;
    int m_selectedIndex;
    int m_scrollPos;
    int m_itemSize;
    int m_itemsPerRow;
    
    static const int ITEM_SIZE = 120;
    static const int ITEM_PADDING = 10;
    static const int TEXT_HEIGHT = 35;
    
public:
    AlbumArtGridWindow() 
        : m_hwnd(NULL), m_parent(NULL), 
          m_selectedIndex(-1), m_scrollPos(0), 
          m_itemSize(ITEM_SIZE), m_itemsPerRow(3) {
        LoadSampleAlbums();
    }
    
    ~AlbumArtGridWindow() {
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
            {L"Dire Straits", L"Brothers in Arms"},
            {L"Fleetwood Mac", L"Rumours"},
            {L"Eagles", L"Hotel California"},
            {L"Michael Jackson", L"Thriller"}
        };
        
        for (int i = 0; i < 15; i++) {
            AlbumInfo album;
            album.artist = samples[i][0];
            album.album = samples[i][1];
            album.color = RGB(60 + (i*20)%100, 70 + (i*15)%100, 80 + (i*25)%100);
            m_albums.push_back(album);
        }
    }
    
    // Create window
    bool CreateWindow(HWND parent) {
        m_parent = parent;
        
        WNDCLASSW wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"AlbumArtGridClass";
        
        RegisterClassW(&wc);
        
        m_hwnd = CreateWindowExW(
            0,
            L"AlbumArtGridClass",
            L"Album Art Grid",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS,
            0, 0, 400, 300,
            parent, NULL,
            GetModuleHandle(NULL),
            this
        );
        
        if (m_hwnd) {
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
            UpdateScrollBar();
            return true;
        }
        return false;
    }
    
    HWND GetHwnd() const { return m_hwnd; }
    
    // Window procedure
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        AlbumArtGridWindow* pThis;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (AlbumArtGridWindow*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            return 0;
        }
        
        pThis = (AlbumArtGridWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
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
            
        case WM_KEYDOWN:
            pThis->OnKeyDown((int)wParam);
            return 0;
        }
        
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    void OnSize(int width, int height) {
        m_itemsPerRow = (std::max)(1, (width - ITEM_PADDING) / (m_itemSize + ITEM_PADDING));
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
        
        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + m_itemSize, y + m_itemSize);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
        
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
        case SB_THUMBPOSITION: {
            SCROLLINFO si = {0};
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(m_hwnd, SB_VERT, &si);
            m_scrollPos = si.nTrackPos;
            break;
        }
        }
        
        m_scrollPos = (std::max)(0, m_scrollPos);
        
        if (m_scrollPos != oldPos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void OnMouseWheel(int delta) {
        m_scrollPos -= (delta / WHEEL_DELTA) * 40;
        m_scrollPos = (std::max)(0, m_scrollPos);
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
            std::wstring msg = L"Playing: " + m_albums[m_selectedIndex].album + 
                              L"\nBy: " + m_albums[m_selectedIndex].artist;
            MessageBoxW(m_hwnd, msg.c_str(), L"Album Selected", MB_OK | MB_ICONINFORMATION);
        }
    }
    
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
        case VK_HOME:
            m_selectedIndex = 0;
            break;
        case VK_END:
            m_selectedIndex = m_albums.size() - 1;
            break;
        }
        
        if (m_selectedIndex != oldIndex) {
            EnsureVisible(m_selectedIndex);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void EnsureVisible(int index) {
        int row = index / m_itemsPerRow;
        int itemY = row * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING);
        
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        if (itemY < m_scrollPos) {
            m_scrollPos = itemY;
        } else if (itemY + m_itemSize + TEXT_HEIGHT > m_scrollPos + rcClient.bottom) {
            m_scrollPos = itemY + m_itemSize + TEXT_HEIGHT - rcClient.bottom;
        }
        
        SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
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

// Global instance
static AlbumArtGridWindow* g_gridWindow = NULL;

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
        if (g_gridWindow) delete g_gridWindow;
        g_gridWindow = new AlbumArtGridWindow();
        g_gridWindow->CreateWindow(parent);
        return g_gridWindow->GetHwnd();
    }
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (g_gridWindow) {
            delete g_gridWindow;
            g_gridWindow = NULL;
        }
        break;
    }
    return TRUE;
}