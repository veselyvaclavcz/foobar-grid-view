#pragma once

#include <foobar2000.h>
#include <atlbase.h>
#include <gdiplus.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

using namespace Gdiplus;

struct CacheEntry {
    HBITMAP bitmap;
    DWORD last_access_time;
    size_t size_bytes;
    
    CacheEntry() : bitmap(nullptr), last_access_time(0), size_bytes(0) {}
    ~CacheEntry() {
        if (bitmap) {
            DeleteObject(bitmap);
        }
    }
};

struct LoadRequest {
    std::string key;
    metadb_handle_ptr track;
    int target_size;
    HWND notify_hwnd;
    UINT notify_msg;
    
    LoadRequest(const std::string& k, metadb_handle_ptr t, int size, HWND hwnd, UINT msg)
        : key(k), track(t), target_size(size), notify_hwnd(hwnd), notify_msg(msg) {}
};

class AlbumArtCache {
public:
    AlbumArtCache(size_t max_cache_size_mb = 100);
    ~AlbumArtCache();
    
    // Cache operations
    HBITMAP GetAlbumArt(const std::string& key, int target_size);
    void RequestAlbumArt(const std::string& key, metadb_handle_ptr track, int target_size, HWND notify_hwnd, UINT notify_msg);
    void ClearCache();
    void SetMaxCacheSize(size_t max_size_mb);
    
    // Statistics
    size_t GetCacheSize() const;
    size_t GetCacheEntries() const;
    double GetHitRate() const;

private:
    // Private methods
    void LoaderThread();
    void ProcessLoadRequest(const LoadRequest& request);
    HBITMAP LoadAlbumArtFromTrack(metadb_handle_ptr track, int target_size);
    HBITMAP ResizeBitmap(HBITMAP source, int target_size);
    void EvictOldEntries();
    std::string GenerateKey(metadb_handle_ptr track);
    size_t CalculateBitmapSize(HBITMAP bitmap);
    
    // Private members
    mutable std::mutex m_cache_mutex;
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> m_cache;
    size_t m_max_cache_size;
    size_t m_current_cache_size;
    
    // Statistics
    mutable size_t m_cache_hits;
    mutable size_t m_cache_misses;
    
    // Loading thread
    std::thread m_loader_thread;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    std::queue<LoadRequest> m_load_queue;
    bool m_stop_loader;
    
    // GDI+ token
    ULONG_PTR m_gdiplus_token;
    
    // Constants
    static const int DEFAULT_CACHE_SIZE_MB = 100;
    static const int MAX_LOAD_QUEUE_SIZE = 100;
    static const UINT WM_ALBUMART_LOADED = WM_USER + 1;
};

// Helper functions
namespace AlbumArtUtils {
    HBITMAP CreateDefaultAlbumArt(int size, bool dark_theme = true);
    HBITMAP BitmapFromGdiplusBitmap(Bitmap* gdip_bitmap);
    Bitmap* GdiplusBitmapFromStream(IStream* stream);
    std::string GetAlbumKey(const char* artist, const char* album);
}