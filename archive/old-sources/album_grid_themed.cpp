// Theme-Aware Album Art Grid Component for foobar2000
// Enhanced with dark mode scrollbar support

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dwmapi.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

// Dark mode support
namespace DarkMode {
    bool IsEnabled() {
        // Check Windows dark mode setting
        HKEY hKey;
        DWORD value = 0;
        DWORD size = sizeof(value);
        
        if (RegOpenKeyExW(HKEY_CURRENT_USER, 
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
            RegCloseKey(hKey);
            return value == 0; // 0 means dark mode
        }
        return false;
    }
    
    // Custom scrollbar colors for dark mode
    struct ScrollbarColors {
        COLORREF track;
        COLORREF thumb;
        COLORREF thumbHover;
        COLORREF thumbActive;
        COLORREF arrow;
    };
    
    ScrollbarColors GetColors() {
        if (IsEnabled()) {
            return {
                RGB(45, 45, 45),    // track - dark gray
                RGB(100, 100, 100), // thumb - medium gray
                RGB(130, 130, 130), // thumb hover - lighter gray
                RGB(160, 160, 160), // thumb active - even lighter
                RGB(200, 200, 200)  // arrow - light gray
            };
        } else {
            return {
                RGB(240, 240, 240), // track - light gray
                RGB(205, 205, 205), // thumb - medium gray
                RGB(166, 166, 166), // thumb hover - darker gray
                RGB(96, 96, 96),    // thumb active - dark gray
                RGB(96, 96, 96)     // arrow - dark gray
            };
        }
    }
}

// Album info structure
struct AlbumInfo {
    std::wstring artist;
    std::wstring album;
    COLORREF color;
};

// Custom scrollbar window class
class ThemedScrollbar {
private:
    HWND m_hwnd;
    HWND m_parent;
    int m_position;
    int m_max;
    int m_page;
    int m_thumbPos;
    int m_thumbHeight;
    bool m_isDragging;
    bool m_isHovering;
    int m_dragOffset;
    
public:
    ThemedScrollbar() : m_hwnd(NULL), m_parent(NULL), m_position(0), 
                        m_max(100), m_page(10), m_thumbPos(0), 
                        m_thumbHeight(30), m_isDragging(false), 
                        m_isHovering(false), m_dragOffset(0) {}
    
    bool Create(HWND parent, int x, int y, int width, int height) {
        m_parent = parent;
        
        WNDCLASSW wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ScrollbarWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszClassName = L"ThemedScrollbar";
        
        RegisterClassW(&wc);
        
        m_hwnd = CreateWindowExW(
            0,
            L"ThemedScrollbar",
            NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            parent, NULL,
            GetModuleHandle(NULL),
            this
        );
        
        if (m_hwnd) {
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
            return true;
        }
        return false;
    }
    
    static LRESULT CALLBACK ScrollbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        ThemedScrollbar* pThis;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (ThemedScrollbar*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            return 0;
        }
        
        pThis = (ThemedScrollbar*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!pThis) return DefWindowProcW(hwnd, msg, wParam, lParam);
        
        switch (msg) {
        case WM_PAINT:
            pThis->OnPaint();
            return 0;
            
        case WM_LBUTTONDOWN:
            pThis->OnLButtonDown(HIWORD(lParam));
            return 0;
            
        case WM_LBUTTONUP:
            pThis->OnLButtonUp();
            return 0;
            
        case WM_MOUSEMOVE:
            pThis->OnMouseMove(HIWORD(lParam));
            return 0;
            
        case WM_MOUSELEAVE:
            pThis->m_isHovering = false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    void OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        auto colors = DarkMode::GetColors();
        
        // Draw track
        HBRUSH hbrTrack = CreateSolidBrush(colors.track);
        FillRect(hdc, &rc, hbrTrack);
        DeleteObject(hbrTrack);
        
        // Calculate thumb position and size
        UpdateThumbMetrics();
        
        // Draw thumb
        RECT rcThumb = rc;
        rcThumb.top = m_thumbPos;
        rcThumb.bottom = m_thumbPos + m_thumbHeight;
        rcThumb.left += 2;
        rcThumb.right -= 2;
        
        COLORREF thumbColor = colors.thumb;
        if (m_isDragging) thumbColor = colors.thumbActive;
        else if (m_isHovering) thumbColor = colors.thumbHover;
        
        HBRUSH hbrThumb = CreateSolidBrush(thumbColor);
        FillRect(hdc, &rcThumb, hbrThumb);
        DeleteObject(hbrThumb);
        
        // Draw top and bottom arrows
        DrawArrow(hdc, rc, true, colors.arrow);
        DrawArrow(hdc, rc, false, colors.arrow);
        
        EndPaint(m_hwnd, &ps);
    }
    
    void DrawArrow(HDC hdc, RECT& rc, bool isUp, COLORREF color) {
        RECT rcArrow = rc;
        int arrowHeight = 17;
        
        if (isUp) {
            rcArrow.bottom = rcArrow.top + arrowHeight;
        } else {
            rcArrow.top = rcArrow.bottom - arrowHeight;
        }
        
        // Draw arrow background
        HBRUSH hbrArrow = CreateSolidBrush(DarkMode::GetColors().track);
        FillRect(hdc, &rcArrow, hbrArrow);
        DeleteObject(hbrArrow);
        
        // Draw arrow triangle
        POINT pts[3];
        int cx = (rcArrow.left + rcArrow.right) / 2;
        int cy = (rcArrow.top + rcArrow.bottom) / 2;
        
        if (isUp) {
            pts[0] = {cx, cy - 3};
            pts[1] = {cx - 4, cy + 3};
            pts[2] = {cx + 4, cy + 3};
        } else {
            pts[0] = {cx, cy + 3};
            pts[1] = {cx - 4, cy - 3};
            pts[2] = {cx + 4, cy - 3};
        }
        
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HBRUSH hBrush = CreateSolidBrush(color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
        
        Polygon(hdc, pts, 3);
        
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);
    }
    
    void UpdateThumbMetrics() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int trackHeight = rc.bottom - rc.top - 34; // Account for arrows
        
        if (m_max > 0) {
            m_thumbHeight = (std::max)(30, (int)(trackHeight * m_page / (float)m_max));
            m_thumbPos = 17 + (int)((trackHeight - m_thumbHeight) * m_position / (float)(m_max - m_page));
        }
    }
    
    void OnLButtonDown(int y) {
        SetCapture(m_hwnd);
        
        UpdateThumbMetrics();
        
        if (y >= m_thumbPos && y <= m_thumbPos + m_thumbHeight) {
            m_isDragging = true;
            m_dragOffset = y - m_thumbPos;
        } else {
            // Page up/down
            int newPos = m_position;
            if (y < m_thumbPos) {
                newPos = (std::max)(0, m_position - m_page);
            } else {
                newPos = (std::min)(m_max - m_page, m_position + m_page);
            }
            SetPosition(newPos);
        }
        
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void OnLButtonUp() {
        ReleaseCapture();
        m_isDragging = false;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void OnMouseMove(int y) {
        if (m_isDragging) {
            RECT rc;
            GetClientRect(m_hwnd, &rc);
            
            int trackHeight = rc.bottom - rc.top - 34;
            int effectiveTrackHeight = trackHeight - m_thumbHeight;
            
            int newThumbPos = y - m_dragOffset - 17;
            newThumbPos = (std::max)(0, (std::min)(effectiveTrackHeight, newThumbPos));
            
            int newPos = (int)(newThumbPos * (m_max - m_page) / (float)effectiveTrackHeight);
            SetPosition(newPos);
        } else {
            // Check hover state
            UpdateThumbMetrics();
            bool wasHovering = m_isHovering;
            m_isHovering = (y >= m_thumbPos && y <= m_thumbPos + m_thumbHeight);
            
            if (wasHovering != m_isHovering) {
                InvalidateRect(m_hwnd, NULL, FALSE);
                
                if (m_isHovering) {
                    TRACKMOUSEEVENT tme = {0};
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = m_hwnd;
                    TrackMouseEvent(&tme);
                }
            }
        }
    }
    
    void SetScrollInfo(int max, int page, int pos) {
        m_max = max;
        m_page = page;
        m_position = (std::min)(pos, max - page);
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void SetPosition(int pos) {
        int oldPos = m_position;
        m_position = (std::max)(0, (std::min)(m_max - m_page, pos));
        
        if (oldPos != m_position) {
            InvalidateRect(m_hwnd, NULL, FALSE);
            
            // Notify parent
            SendMessageW(m_parent, WM_VSCROLL, 
                MAKEWPARAM(SB_THUMBPOSITION, m_position), (LPARAM)m_hwnd);
        }
    }
    
    int GetPosition() const { return m_position; }
    
    void UpdatePosition(int x, int y, int width, int height) {
        if (m_hwnd) {
            SetWindowPos(m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
        }
    }
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
    ThemedScrollbar m_scrollbar;
    bool m_isDarkMode;
    
    static const int ITEM_SIZE = 120;
    static const int ITEM_PADDING = 10;
    static const int TEXT_HEIGHT = 35;
    static const int SCROLLBAR_WIDTH = 17;
    
public:
    AlbumArtGridWindow() 
        : m_hwnd(NULL), m_parent(NULL), 
          m_selectedIndex(-1), m_scrollPos(0), 
          m_itemSize(ITEM_SIZE), m_itemsPerRow(3),
          m_isDarkMode(false) {
        LoadSampleAlbums();
        m_isDarkMode = DarkMode::IsEnabled();
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
            {L"Michael Jackson", L"Thriller"},
            {L"Bob Dylan", L"Highway 61 Revisited"},
            {L"The Rolling Stones", L"Exile on Main St."},
            {L"David Bowie", L"The Rise and Fall of Ziggy Stardust"},
            {L"Prince", L"Purple Rain"},
            {L"The Clash", L"London Calling"}
        };
        
        for (int i = 0; i < 20; i++) {
            AlbumInfo album;
            album.artist = samples[i][0];
            album.album = samples[i][1];
            album.color = RGB(60 + (i*20)%100, 70 + (i*15)%100, 80 + (i*25)%100);
            m_albums.push_back(album);
        }
    }
    
    // Create window
    bool CreateGridWindow(HWND parentWnd) {
        m_parent = parentWnd;
        
        WNDCLASSW wc = {0};
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszClassName = L"AlbumArtGridClass";
        
        RegisterClassW(&wc);
        
        m_hwnd = CreateWindowExW(
            0,
            L"AlbumArtGridClass",
            L"Album Art Grid",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 400, 300,
            parentWnd, NULL,
            GetModuleHandle(NULL),
            this
        );
        
        if (m_hwnd) {
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
            
            // Create custom scrollbar
            RECT rc;
            GetClientRect(m_hwnd, &rc);
            m_scrollbar.Create(m_hwnd, rc.right - SCROLLBAR_WIDTH, 0, 
                              SCROLLBAR_WIDTH, rc.bottom);
            
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
            pThis->OnVScroll(wParam, lParam);
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
            
        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE:
            pThis->OnThemeChanged();
            return 0;
        }
        
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    void OnSize(int width, int height) {
        m_itemsPerRow = (std::max)(1, (width - SCROLLBAR_WIDTH - ITEM_PADDING) / (m_itemSize + ITEM_PADDING));
        
        // Update scrollbar position
        m_scrollbar.UpdatePosition(width - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, height);
        
        UpdateScrollBar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void OnPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        rcClient.right -= SCROLLBAR_WIDTH; // Account for scrollbar
        
        // Create memory DC for double buffering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        
        // Fill background based on theme
        COLORREF bgColor = m_isDarkMode ? RGB(30, 30, 30) : RGB(245, 245, 245);
        HBRUSH hbrBg = CreateSolidBrush(bgColor);
        FillRect(hdcMem, &rcClient, hbrBg);
        DeleteObject(hbrBg);
        
        // Set text properties
        SetBkMode(hdcMem, TRANSPARENT);
        COLORREF textColor = m_isDarkMode ? RGB(255, 255, 255) : RGB(0, 0, 0);
        SetTextColor(hdcMem, textColor);
        
        HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
        
        // Draw albums
        int startY = -m_scrollPos;
        for (size_t i = 0; i < m_albums.size(); i++) {
            int row = (int)(i / m_itemsPerRow);
            int col = (int)(i % m_itemsPerRow);
            
            int x = ITEM_PADDING + col * (m_itemSize + ITEM_PADDING);
            int y = startY + ITEM_PADDING + row * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING);
            
            if (y + m_itemSize + TEXT_HEIGHT < 0) continue;
            if (y > rcClient.bottom) break;
            
            // Draw album cover placeholder
            RECT rcAlbum = {x, y, x + m_itemSize, y + m_itemSize};
            
            // Draw selection highlight
            if ((int)i == m_selectedIndex) {
                RECT rcHighlight = rcAlbum;
                InflateRect(&rcHighlight, 3, 3);
                COLORREF highlightColor = m_isDarkMode ? RGB(100, 150, 255) : RGB(0, 120, 215);
                HBRUSH hbrHighlight = CreateSolidBrush(highlightColor);
                FillRect(hdcMem, &rcHighlight, hbrHighlight);
                DeleteObject(hbrHighlight);
            }
            
            // Draw album art placeholder
            HBRUSH hbrAlbum = CreateSolidBrush(m_albums[i].color);
            FillRect(hdcMem, &rcAlbum, hbrAlbum);
            DeleteObject(hbrAlbum);
            
            // Draw border
            COLORREF borderColor = m_isDarkMode ? RGB(60, 60, 60) : RGB(200, 200, 200);
            HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
            HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            Rectangle(hdcMem, rcAlbum.left, rcAlbum.top, rcAlbum.right, rcAlbum.bottom);
            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hPen);
            
            // Draw text
            RECT rcText = {x, y + m_itemSize + 2, x + m_itemSize, y + m_itemSize + TEXT_HEIGHT};
            DrawTextW(hdcMem, m_albums[i].artist.c_str(), -1, &rcText, 
                     DT_CENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);
            
            rcText.top += 15;
            COLORREF subTextColor = m_isDarkMode ? RGB(180, 180, 180) : RGB(100, 100, 100);
            SetTextColor(hdcMem, subTextColor);
            DrawTextW(hdcMem, m_albums[i].album.c_str(), -1, &rcText, 
                     DT_CENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);
            SetTextColor(hdcMem, textColor);
        }
        
        SelectObject(hdcMem, hOldFont);
        DeleteObject(hFont);
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
        
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        
        EndPaint(m_hwnd, &ps);
    }
    
    void OnVScroll(WPARAM wParam, LPARAM lParam) {
        if ((HWND)lParam == m_scrollbar.m_hwnd) {
            // Custom scrollbar message
            m_scrollPos = HIWORD(wParam);
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // Standard scrollbar message (if any)
            int code = LOWORD(wParam);
            int oldPos = m_scrollPos;
            
            RECT rcClient;
            GetClientRect(m_hwnd, &rcClient);
            int pageSize = rcClient.bottom;
            
            switch (code) {
            case SB_LINEUP: m_scrollPos -= 20; break;
            case SB_LINEDOWN: m_scrollPos += 20; break;
            case SB_PAGEUP: m_scrollPos -= pageSize; break;
            case SB_PAGEDOWN: m_scrollPos += pageSize; break;
            }
            
            UpdateScrollBar();
            
            if (oldPos != m_scrollPos) {
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
    }
    
    void OnMouseWheel(short delta) {
        int oldPos = m_scrollPos;
        m_scrollPos -= (delta / WHEEL_DELTA) * 40;
        
        UpdateScrollBar();
        
        if (oldPos != m_scrollPos) {
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void OnLButtonDown(int x, int y) {
        // Don't handle clicks on scrollbar area
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        if (x >= rc.right - SCROLLBAR_WIDTH) return;
        
        SetFocus(m_hwnd);
        
        int adjustedY = y + m_scrollPos;
        int row = (adjustedY - ITEM_PADDING) / (m_itemSize + TEXT_HEIGHT + ITEM_PADDING);
        int col = (x - ITEM_PADDING) / (m_itemSize + ITEM_PADDING);
        
        if (row >= 0 && col >= 0 && col < m_itemsPerRow) {
            int index = row * m_itemsPerRow + col;
            if (index >= 0 && index < (int)m_albums.size()) {
                m_selectedIndex = index;
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
    }
    
    void OnLButtonDblClk(int x, int y) {
        OnLButtonDown(x, y);
        
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_albums.size()) {
            std::wstring msg = L"Playing: " + m_albums[m_selectedIndex].artist + 
                              L" - " + m_albums[m_selectedIndex].album;
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
            m_selectedIndex = (int)m_albums.size() - 1;
            break;
        }
        
        if (oldIndex != m_selectedIndex) {
            EnsureVisible(m_selectedIndex);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void OnThemeChanged() {
        bool wasD
arkMode = m_isDarkMode;
        m_isDarkMode = DarkMode::IsEnabled();
        
        if (wasDarkMode != m_isDarkMode) {
            InvalidateRect(m_hwnd, NULL, FALSE);
            // Scrollbar will repaint automatically
        }
    }
    
    void EnsureVisible(int index) {
        if (index < 0 || index >= (int)m_albums.size()) return;
        
        int row = index / m_itemsPerRow;
        int itemY = ITEM_PADDING + row * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING);
        
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        if (itemY < m_scrollPos) {
            m_scrollPos = itemY;
        } else if (itemY + m_itemSize + TEXT_HEIGHT > m_scrollPos + rcClient.bottom) {
            m_scrollPos = itemY + m_itemSize + TEXT_HEIGHT - rcClient.bottom;
        }
        
        UpdateScrollBar();
    }
    
    void UpdateScrollBar() {
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        
        int rows = (int)((m_albums.size() + m_itemsPerRow - 1) / m_itemsPerRow);
        int totalHeight = rows * (m_itemSize + TEXT_HEIGHT + ITEM_PADDING) + ITEM_PADDING;
        
        int maxScroll = (std::max)(0, totalHeight - rcClient.bottom);
        m_scrollPos = (std::max)(0, (std::min)(maxScroll, m_scrollPos));
        
        // Update custom scrollbar
        m_scrollbar.SetScrollInfo(totalHeight, rcClient.bottom, m_scrollPos);
    }
};

// Component entry point
extern "C" __declspec(dllexport) void* CreateAlbumArtGrid(HWND parent) {
    AlbumArtGridWindow* window = new AlbumArtGridWindow();
    if (window->CreateGridWindow(parent)) {
        return window;
    }
    delete window;
    return nullptr;
}

extern "C" __declspec(dllexport) void DestroyAlbumArtGrid(void* instance) {
    if (instance) {
        delete (AlbumArtGridWindow*)instance;
    }
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}