# Module: ThumbnailCache

## Overview
**Layer**: Resources  
**Purpose**: Memory-efficient caching of album art thumbnails with LRU eviction  
**Status**: Not Started  
**Version**: 0.0.1  
**Last Updated**: 2025-01-09  

## Responsibilities
- Cache album art thumbnails in memory
- LRU (Least Recently Used) eviction policy
- Memory limit enforcement
- Auto-detection of available RAM
- Prefetching adjacent items
- Cache statistics and monitoring

## Dependencies
### Internal Dependencies
- LifecycleManager: For lifecycle management
- AlbumArtLoader: Source of album art

### External Dependencies
- GDI+: Bitmap storage
- Windows API: Memory information

## Interface

### Public Methods
```cpp
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
    
    // Configure cache limits
    void configure(size_t max_memory_mb = 0);  // 0 = auto-detect
    
    // Get cached thumbnail
    // Returns: Bitmap pointer (do not delete) or nullptr if not cached
    Gdiplus::Bitmap* get(const std::string& key);
    
    // Add thumbnail to cache
    // Takes ownership of bitmap
    void put(const std::string& key, std::unique_ptr<Gdiplus::Bitmap> bitmap);
    
    // Prefetch range of items
    void prefetch(const std::vector<std::string>& keys);
    
    // Clear all cached items
    void clear();
    
    // Get cache statistics
    CacheStats get_stats() const;
    
private:
    // Auto-detect optimal cache size based on RAM
    size_t detect_optimal_cache_size() const;
    
    // Evict least recently used items
    void evict_lru(size_t bytes_needed);
    
    // Calculate bitmap memory usage
    size_t calculate_bitmap_size(Gdiplus::Bitmap* bmp) const;
};
```

## Implementation Notes

### Critical Sections
- **Memory limit enforcement**: Never exceed configured limit
- **Thread safety**: Mutex protection for concurrent access
- **Bitmap lifetime**: Cache owns bitmaps, callers must not delete

### Error Handling
- **Out of memory**: Evict LRU items aggressively
- **Corrupted bitmap**: Remove from cache, return nullptr
- **Cache miss**: Return nullptr, trigger async load

### Performance Considerations
- Use unordered_map for O(1) lookup
- Track access order with linked list
- Batch eviction to reduce overhead
- Monitor memory pressure

### Memory Limits (Auto-Detection)
```cpp
System RAM    | Cache Size | Prefetch Range
------------- | ---------- | --------------
32GB+         | 2GB        | 50 items
16-32GB       | 1GB        | 50 items  
8-16GB        | 512MB      | 30 items
4-8GB         | 256MB      | 20 items
<4GB          | 128MB      | 10 items
```

## Testing

### Test Coverage
- [ ] Basic put/get operations
- [ ] LRU eviction policy
- [ ] Memory limit enforcement
- [ ] Prefetching behavior
- [ ] Cache statistics accuracy
- [ ] Thread safety
- [ ] Large library stress test

### Known Issues
- Previous versions had unbounded memory growth

## Change Log
| Date | Version | Change | Author |
|------|---------|--------|--------|
| 2025-01-09 | 0.0.1 | Initial documentation | System |