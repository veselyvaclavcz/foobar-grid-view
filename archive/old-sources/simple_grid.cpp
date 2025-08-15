// Simplified Album Art Grid for foobar2000
// This version compiles without the full SDK

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <memory>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")

// Export component version
extern "C" __declspec(dllexport) const char* foobar2000_component_version() {
    return "1.0.0";
}

// Export interface version
extern "C" __declspec(dllexport) int foobar2000_get_interface_version() {
    return 80;
}

// Album structure
struct Album {
    std::wstring title;
    std::wstring artist;
    COLORREF color; // Random color for demo
};

// Grid window class
class GridWindow {
private:
    HWND m_hwnd;
    std::vector<Album> m_albums;
    int m_selectedIndex;
    int m_scrollPos;
    int m_itemSize;
    int m_itemsPerRow;
    
public:
    GridWindow() : m_hwnd(nullptr), m_selectedIndex(0), m_scrollPos(0), m_itemSize(120), m_itemsPerRow(3) {
        LoadSampleAlbums();
    }
    
    void LoadSampleAlbums() {
        const wchar_t* titles[] = {
            L"Dark Side", L"Abbey Road", L"Thriller", L"The Wall", L"Nevermind",
            L"OK Computer", L"Rumours", L"Ten", L"Metallica", L"Hybrid Theory",
            L"In Utero", L"Purple Rain", L"Born to Run", L"Joshua Tree", L"Blood Sugar"
        };
        
        const wchar_t* artists[] = {
            L"Pink Floyd", L"Beatles", L"M. Jackson", L"Pink Floyd", L"Nirvana",
            L"Radiohead", L"Fleetwood Mac", L"Pearl Jam", L"Metallica", L"Linkin Park",
            L"Nirvana", L"Prince", L"Springsteen", L"U2", L"RHCP"
        };
        
        for (int i = 0; i < 15; i++) {
            Album album;
            album.title = titles[i];
            album.artist = artists[i];
            album.color = RGB(50 + (i * 10) % 150, 60 + (i * 15) % 150, 70 + (i * 20) % 150);
            m_albums.push_back(album);
        }
    }
    
    bool Create(HWND parent) {
        WNDCLASSW wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"AlbumGrid";
        
        RegisterClassW(&wc);
        
        m_hwnd = CreateWindowW(
            L"AlbumGrid", L"Album Grid",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            0, 0, 400, 300,
            parent, nullptr, GetModuleHandle(nullptr), this
        );
        
        return m_hwnd != nullptr;
    }
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        GridWindow* pThis;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (GridWindow*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwnd = hwnd;
            return 0;
        }
        
        pThis = (GridWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!pThis) return DefWindowProcW(hwnd, msg, wParam, lParam);
        
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            pThis->Paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_SIZE:
            pThis->OnSize();
            return 0;
            
        case WM_VSCROLL:
            pThis->OnScroll(LOWORD(wParam));
            return 0;
            
        case WM_LBUTTONDOWN:
            pThis->OnClick(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_MOUSEWHEEL:
            pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
        }
        
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    void Paint(HDC hdc) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Calculate layout
        m_itemsPerRow = max(1, rc.right / (m_itemSize + 10));
        
        // Set text properties
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        // Draw albums
        int y = -m_scrollPos;
        for (size_t i = 0; i < m_albums.size(); i++) {
            int col = i % m_itemsPerRow;
            int row = i / m_itemsPerRow;
            
            int x = col * (m_itemSize + 10) + 10;
            int itemY = y + row * (m_itemSize + 40) + 10;
            
            if (itemY + m_itemSize < 0) continue;
            if (itemY > rc.bottom) break;
            
            DrawAlbum(hdc, x, itemY, m_albums[i], i == m_selectedIndex);
        }
    }
    
    void DrawAlbum(HDC hdc, int x, int y, const Album& album, bool selected) {
        // Draw album art placeholder
        RECT rcAlbum = { x, y, x + m_itemSize, y + m_itemSize };
        HBRUSH hbr = CreateSolidBrush(album.color);
        FillRect(hdc, &rcAlbum, hbr);
        DeleteObject(hbr);
        
        // Draw selection
        if (selected) {
            HPEN hpen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
            HPEN oldPen = (HPEN)SelectObject(hdc, hpen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, x-2, y-2, x + m_itemSize + 2, y + m_itemSize + 2);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(hpen);
        }
        
        // Draw text
        RECT rcText = { x, y + m_itemSize + 2, x + m_itemSize, y + m_itemSize + 35 };
        DrawTextW(hdc, album.title.c_str(), -1, &rcText, DT_CENTER | DT_WORDBREAK);
        
        SetTextColor(hdc, RGB(180, 180, 180));
        rcText.top += 16;
        DrawTextW(hdc, album.artist.c_str(), -1, &rcText, DT_CENTER | DT_SINGLELINE);
        SetTextColor(hdc, RGB(255, 255, 255));
    }
    
    void OnSize() {
        UpdateScrollBar();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
    
    void OnScroll(int code) {
        int oldPos = m_scrollPos;
        
        switch (code) {
        case SB_LINEUP: m_scrollPos -= 20; break;
        case SB_LINEDOWN: m_scrollPos += 20; break;
        case SB_PAGEUP: m_scrollPos -= 100; break;
        case SB_PAGEDOWN: m_scrollPos += 100; break;
        }
        
        m_scrollPos = max(0, m_scrollPos);
        
        if (m_scrollPos != oldPos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }
    
    void OnMouseWheel(int delta) {
        m_scrollPos -= (delta / WHEEL_DELTA) * 40;
        m_scrollPos = max(0, m_scrollPos);
        SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
    
    void OnClick(int x, int y) {
        int col = x / (m_itemSize + 10);
        int row = (y + m_scrollPos) / (m_itemSize + 40);
        int index = row * m_itemsPerRow + col;
        
        if (index >= 0 && index < (int)m_albums.size()) {
            m_selectedIndex = index;
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }
    
    void UpdateScrollBar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int rows = (m_albums.size() + m_itemsPerRow - 1) / m_itemsPerRow;
        int totalHeight = rows * (m_itemSize + 40);
        
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = totalHeight;
        si.nPage = rc.bottom;
        
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }
    
    HWND GetHwnd() const { return m_hwnd; }
};

// Global instance
static GridWindow* g_gridWindow = nullptr;

// Export function to create the grid
extern "C" __declspec(dllexport) HWND CreateAlbumGrid(HWND parent) {
    if (g_gridWindow) delete g_gridWindow;
    g_gridWindow = new GridWindow();
    g_gridWindow->Create(parent);
    return g_gridWindow->GetHwnd();
}

// Main interface function
extern "C" __declspec(dllexport) void foobar2000_get_interface(void** ppv, const char* name) {
    // Return null for now - component loads but doesn't integrate with UI yet
    if (ppv) *ppv = nullptr;
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        if (g_gridWindow) {
            delete g_gridWindow;
            g_gridWindow = nullptr;
        }
    }
    return TRUE;
}