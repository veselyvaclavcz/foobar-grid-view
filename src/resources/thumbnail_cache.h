#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../foundation/lifecycle_manager.h"
#include <Windows.h>
#include <gdiplus.h>
#include <unordered_map>
#include <memory>
#include <list>
#include <mutex>
#include <atomic>

namespace albumart_grid {

// Thumbnail cache with LRU eviction
class ThumbnailCache : public ILifecycleAware {
public:
    struct CacheStats {
        size_t total_memory_bytes;
        size_t used_memory_bytes;
        size_t item_count;
        size_t hit_count;
        size_t miss_count;
        float hit_rate;
    };
    
    ThumbnailCache();
    ~ThumbnailCache();
    
    // ILifecycleAware implementation
    void on_initialize() override;
    void on_shutdown() override;
    bool is_valid() const override;
    
    // Configure cache limits (0 = auto-detect)
    void configure(size_t max_memory_mb = 0);
    
    // Get cached thumbnail (returns nullptr if not cached)
    Gdiplus::Bitmap* get(const std::string& key);
    
    // Add thumbnail to cache (takes ownership)
    void put(const std::string& key, std::unique_ptr<Gdiplus::Bitmap> bitmap);
    
    // Prefetch range of items
    void prefetch(const std::vector<std::string>& keys);
    
    // Clear all cached items
    void clear();
    
    // Get cache statistics
    CacheStats get_stats() const;
    
private:
    struct CacheEntry {
        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        size_t memory_size;
        std::list<std::string>::iterator lru_iterator;
    };
    
    // Auto-detect optimal cache size based on RAM
    size_t detect_optimal_cache_size() const;
    
    // Evict least recently used items
    void evict_lru(size_t bytes_needed);
    
    // Calculate bitmap memory usage
    size_t calculate_bitmap_size(Gdiplus::Bitmap* bmp) const;
    
    // Update LRU order
    void touch_entry(const std::string& key);
    
private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, CacheEntry> m_cache;
    std::list<std::string> m_lru_list; // Front = most recent, Back = least recent
    
    size_t m_max_memory;
    std::atomic<size_t> m_used_memory{0};
    std::atomic<size_t> m_hit_count{0};
    std::atomic<size_t> m_miss_count{0};
    
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shutting_down{false};
};

} // namespace albumart_grid