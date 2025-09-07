#include "../include/album_art_cache.h"
#include <algorithm>
#include <sstream>

using namespace Gdiplus;

// Initialize GDI+ token
static ULONG_PTR g_gdiplusToken = 0;
static int g_gdiplusRefCount = 0;
static std::mutex g_gdiplusMutex;

static void InitializeGdiPlus() {
    std::lock_guard<std::mutex> lock(g_gdiplusMutex);
    if (g_gdiplusRefCount == 0) {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    }
    g_gdiplusRefCount++;
}

static void ShutdownGdiPlus() {
    std::lock_guard<std::mutex> lock(g_gdiplusMutex);
    g_gdiplusRefCount--;
    if (g_gdiplusRefCount == 0) {
        GdiplusShutdown(g_gdiplusToken);
    }
}

AlbumArtCache::AlbumArtCache(size_t max_cache_size_mb)
    : m_max_cache_size(max_cache_size_mb * 1024 * 1024)
    , m_current_cache_size(0)
    , m_cache_hits(0)
    , m_cache_misses(0)
    , m_stop_loader(false)
{
    InitializeGdiPlus();
    m_loader_thread = std::thread(&AlbumArtCache::LoaderThread, this);
}

AlbumArtCache::~AlbumArtCache() {
    // Stop loader thread
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_stop_loader = true;
    }
    m_queue_cv.notify_all();
    
    if (m_loader_thread.joinable()) {
        m_loader_thread.join();
    }
    
    ClearCache();
    ShutdownGdiPlus();
}

HBITMAP AlbumArtCache::GetAlbumArt(const std::string& key, int target_size) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Update access time
        it->second->last_access_time = GetTickCount();
        m_cache_hits++;
        
        // Return a copy of the bitmap
        if (it->second->bitmap) {
            return (HBITMAP)CopyImage(it->second->bitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        }
    }
    
    m_cache_misses++;
    return nullptr;
}

void AlbumArtCache::RequestAlbumArt(const std::string& key, metadb_handle_ptr track, 
                                   int target_size, HWND notify_hwnd, UINT notify_msg) {
    // Check if already in cache
    {
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // Already cached, notify immediately
            PostMessage(notify_hwnd, notify_msg, 0, 0);
            return;
        }
    }
    
    // Add to load queue
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        
        // Check queue size to prevent memory issues
        if (m_load_queue.size() >= MAX_LOAD_QUEUE_SIZE) {
            return;
        }
        
        m_load_queue.emplace(key, track, target_size, notify_hwnd, notify_msg);
    }
    m_queue_cv.notify_one();
}

void AlbumArtCache::ClearCache() {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    m_cache.clear();
    m_current_cache_size = 0;
}

void AlbumArtCache::SetMaxCacheSize(size_t max_size_mb) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    m_max_cache_size = max_size_mb * 1024 * 1024;
    EvictOldEntries();
}

size_t AlbumArtCache::GetCacheSize() const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    return m_current_cache_size;
}

size_t AlbumArtCache::GetCacheEntries() const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    return m_cache.size();
}

double AlbumArtCache::GetHitRate() const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    size_t total_requests = m_cache_hits + m_cache_misses;
    return total_requests > 0 ? (double)m_cache_hits / total_requests : 0.0;
}

void AlbumArtCache::LoaderThread() {
    while (true) {
        LoadRequest request("", nullptr, 0, nullptr, 0);
        
        // Get next request
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_cv.wait(lock, [this]() { return !m_load_queue.empty() || m_stop_loader; });
            
            if (m_stop_loader) break;
            
            if (!m_load_queue.empty()) {
                request = m_load_queue.front();
                m_load_queue.pop();
            } else {
                continue;
            }
        }
        
        // Process the request
        ProcessLoadRequest(request);
    }
}

void AlbumArtCache::ProcessLoadRequest(const LoadRequest& request) {
    // Check if already cached (might have been loaded by another request)
    {
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        if (m_cache.find(request.key) != m_cache.end()) {
            if (request.notify_hwnd) {
                PostMessage(request.notify_hwnd, request.notify_msg, 0, 0);
            }
            return;
        }
    }
    
    // Load album art
    HBITMAP bitmap = LoadAlbumArtFromTrack(request.track, request.target_size);
    if (!bitmap) {
        bitmap = AlbumArtUtils::CreateDefaultAlbumArt(request.target_size);
    }
    
    if (bitmap) {
        // Add to cache
        {
            std::lock_guard<std::mutex> lock(m_cache_mutex);
            
            auto entry = std::make_unique<CacheEntry>();
            entry->bitmap = bitmap;
            entry->last_access_time = GetTickCount();
            entry->size_bytes = CalculateBitmapSize(bitmap);
            
            m_current_cache_size += entry->size_bytes;
            m_cache[request.key] = std::move(entry);
            
            // Evict old entries if necessary
            EvictOldEntries();
        }
        
        // Notify completion
        if (request.notify_hwnd) {
            PostMessage(request.notify_hwnd, request.notify_msg, 0, 0);
        }
    }
}

HBITMAP AlbumArtCache::LoadAlbumArtFromTrack(metadb_handle_ptr track, int target_size) {
    if (!track.is_valid()) return nullptr;
    
    try {
        // Get album art from track
        static_api_ptr_t<album_art_manager_v2> aam;
        album_art_data_ptr art_data;
        
        if (aam->open(track, pfc::list_single_ref_t<GUID>(album_art_ids::cover_front), 
                     abort_callback_dummy()) && aam->query(album_art_ids::cover_front, art_data)) {
            
            // Create IStream from art data
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, art_data->get_size());
            if (!hGlobal) return nullptr;
            
            void* pData = GlobalLock(hGlobal);
            if (!pData) {
                GlobalFree(hGlobal);
                return nullptr;
            }
            
            memcpy(pData, art_data->get_ptr(), art_data->get_size());
            GlobalUnlock(hGlobal);
            
            IStream* pStream = nullptr;
            HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
            if (FAILED(hr)) {
                GlobalFree(hGlobal);
                return nullptr;
            }
            
            // Load bitmap from stream
            Bitmap* gdip_bitmap = AlbumArtUtils::GdiplusBitmapFromStream(pStream);
            pStream->Release();
            
            if (gdip_bitmap && gdip_bitmap->GetLastStatus() == Ok) {
                // Resize if necessary
                HBITMAP result_bitmap;
                
                if ((int)gdip_bitmap->GetWidth() != target_size || 
                    (int)gdip_bitmap->GetHeight() != target_size) {
                    
                    // Create resized bitmap
                    Bitmap resized_bitmap(target_size, target_size, PixelFormat32bppARGB);
                    Graphics graphics(&resized_bitmap);
                    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                    graphics.DrawImage(gdip_bitmap, 0, 0, target_size, target_size);
                    
                    result_bitmap = AlbumArtUtils::BitmapFromGdiplusBitmap(&resized_bitmap);
                } else {
                    result_bitmap = AlbumArtUtils::BitmapFromGdiplusBitmap(gdip_bitmap);
                }
                
                delete gdip_bitmap;
                return result_bitmap;
            }
            
            if (gdip_bitmap) delete gdip_bitmap;
        }
    } catch (...) {
        // Handle exceptions silently
    }
    
    return nullptr;
}

HBITMAP AlbumArtCache::ResizeBitmap(HBITMAP source, int target_size) {
    if (!source) return nullptr;
    
    HDC hdc = GetDC(nullptr);
    HDC src_dc = CreateCompatibleDC(hdc);
    HDC dst_dc = CreateCompatibleDC(hdc);
    
    HBITMAP dst_bitmap = CreateCompatibleBitmap(hdc, target_size, target_size);
    
    HBITMAP old_src = (HBITMAP)SelectObject(src_dc, source);
    HBITMAP old_dst = (HBITMAP)SelectObject(dst_dc, dst_bitmap);
    
    SetStretchBltMode(dst_dc, HALFTONE);
    StretchBlt(dst_dc, 0, 0, target_size, target_size,
               src_dc, 0, 0, target_size, target_size, SRCCOPY);
    
    SelectObject(src_dc, old_src);
    SelectObject(dst_dc, old_dst);
    DeleteDC(src_dc);
    DeleteDC(dst_dc);
    ReleaseDC(nullptr, hdc);
    
    return dst_bitmap;
}

void AlbumArtCache::EvictOldEntries() {
    while (m_current_cache_size > m_max_cache_size && !m_cache.empty()) {
        // Find oldest entry
        auto oldest = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->second->last_access_time < oldest->second->last_access_time) {
                oldest = it;
            }
        }
        
        // Remove oldest entry
        m_current_cache_size -= oldest->second->size_bytes;
        m_cache.erase(oldest);
    }
}

std::string AlbumArtCache::GenerateKey(metadb_handle_ptr track) {
    if (!track.is_valid()) return "";
    
    file_info_impl info;
    if (!track->get_info(info)) return "";
    
    const char* artist = info.meta_get("ARTIST", 0);
    const char* album = info.meta_get("ALBUM", 0);
    
    return AlbumArtUtils::GetAlbumKey(
        artist ? artist : "Unknown Artist",
        album ? album : "Unknown Album"
    );
}

size_t AlbumArtCache::CalculateBitmapSize(HBITMAP bitmap) {
    if (!bitmap) return 0;
    
    BITMAP bm;
    if (GetObject(bitmap, sizeof(BITMAP), &bm) == 0) return 0;
    
    return bm.bmWidth * bm.bmHeight * (bm.bmBitsPixel / 8);
}

// Helper functions implementation
namespace AlbumArtUtils {
    HBITMAP CreateDefaultAlbumArt(int size, bool dark_theme) {
        HDC hdc = GetDC(nullptr);
        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, size, size);
        HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, bitmap);
        
        // Fill background
        RECT rect = { 0, 0, size, size };
        COLORREF bg_color = dark_theme ? RGB(32, 32, 32) : RGB(240, 240, 240);
        COLORREF border_color = dark_theme ? RGB(64, 64, 64) : RGB(200, 200, 200);
        
        HBRUSH bg_brush = CreateSolidBrush(bg_color);
        FillRect(mem_dc, &rect, bg_brush);
        DeleteObject(bg_brush);
        
        // Draw border
        HPEN border_pen = CreatePen(PS_SOLID, 1, border_color);
        HPEN old_pen = (HPEN)SelectObject(mem_dc, border_pen);
        Rectangle(mem_dc, 0, 0, size, size);
        SelectObject(mem_dc, old_pen);
        DeleteObject(border_pen);
        
        // Draw music note icon (simplified)
        HPEN icon_pen = CreatePen(PS_SOLID, 2, dark_theme ? RGB(128, 128, 128) : RGB(128, 128, 128));
        SelectObject(mem_dc, icon_pen);
        
        int center_x = size / 2;
        int center_y = size / 2;
        int note_size = size / 4;
        
        // Draw simple music note
        MoveToEx(mem_dc, center_x - note_size/2, center_y + note_size/2, nullptr);
        LineTo(mem_dc, center_x - note_size/2, center_y - note_size/2);
        LineTo(mem_dc, center_x + note_size/2, center_y - note_size/3);
        
        Ellipse(mem_dc, center_x - note_size/2 - 4, center_y + note_size/2 - 4,
                center_x - note_size/2 + 4, center_y + note_size/2 + 4);
        
        SelectObject(mem_dc, old_pen);
        DeleteObject(icon_pen);
        
        SelectObject(mem_dc, old_bitmap);
        DeleteDC(mem_dc);
        ReleaseDC(nullptr, hdc);
        
        return bitmap;
    }
    
    HBITMAP BitmapFromGdiplusBitmap(Bitmap* gdip_bitmap) {
        if (!gdip_bitmap || gdip_bitmap->GetLastStatus() != Ok) return nullptr;
        
        HBITMAP hbitmap;
        Status status = gdip_bitmap->GetHBITMAP(Color(0, 0, 0, 0), &hbitmap);
        return (status == Ok) ? hbitmap : nullptr;
    }
    
    Bitmap* GdiplusBitmapFromStream(IStream* stream) {
        if (!stream) return nullptr;
        return new Bitmap(stream, false);
    }
    
    std::string GetAlbumKey(const char* artist, const char* album) {
        std::ostringstream oss;
        oss << (artist ? artist : "Unknown Artist") << "|" << (album ? album : "Unknown Album");
        return oss.str();
    }
}