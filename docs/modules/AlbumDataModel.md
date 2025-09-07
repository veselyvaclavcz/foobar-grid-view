# Module: AlbumDataModel

## Overview
**Layer**: Core (Business Logic)  
**Purpose**: Manages album data, grouping logic, and sorting algorithms  
**Status**: Not Started  
**Version**: 0.0.1  
**Last Updated**: 2025-01-09  

## Responsibilities
- Album grouping (13 modes)
- Sorting implementation (11 options)
- Track aggregation within albums
- Metadata extraction and caching
- Search/filter functionality
- Data change notifications

## Dependencies
### Internal Dependencies
- LifecycleManager: For lifecycle management
- CallbackManager: For library change notifications

### External Dependencies
- foobar2000 SDK: metadb_handle, titleformat, library_manager
- STL: vector, unordered_map, algorithm

## Interface

### Public Methods
```cpp
class AlbumDataModel : public ILifecycleAware {
public:
    // Grouping modes
    enum GroupingMode {
        GROUP_BY_FOLDER,
        GROUP_BY_ALBUM,
        GROUP_BY_ARTIST,
        GROUP_BY_ALBUM_ARTIST,
        GROUP_BY_YEAR,
        GROUP_BY_GENRE,
        GROUP_BY_DATE_MODIFIED,
        GROUP_BY_DATE_ADDED,
        GROUP_BY_FILE_SIZE,
        GROUP_BY_TRACK_COUNT,
        GROUP_BY_RATING,
        GROUP_BY_PLAYCOUNT,
        GROUP_BY_CUSTOM
    };
    
    // Sorting modes
    enum SortingMode {
        SORT_BY_NAME,
        SORT_BY_ARTIST,
        SORT_BY_ALBUM,
        SORT_BY_YEAR,
        SORT_BY_GENRE,
        SORT_BY_DATE_MODIFIED,
        SORT_BY_TOTAL_SIZE,
        SORT_BY_TRACK_COUNT,
        SORT_BY_RATING,
        SORT_BY_PATH,
        SORT_BY_RANDOM
    };
    
    // Set grouping mode
    void set_grouping(GroupingMode mode, const char* custom_pattern = nullptr);
    
    // Set sorting mode
    void set_sorting(SortingMode mode, bool descending = false);
    
    // Get grouped items
    const std::vector<album_item>& get_items() const;
    
    // Search/filter
    void set_filter(const char* search_text);
    
    // Refresh from library
    void refresh();
    
    // Find item containing track
    int find_item_for_track(metadb_handle_ptr track) const;
};

struct album_item {
    metadb_handle_list tracks;
    pfc::string8 primary_text;    // Main display text
    pfc::string8 secondary_text;   // Secondary info
    pfc::string8 sort_key;        // For sorting
    
    // Calculated fields
    uint64_t total_size;
    uint32_t track_count;
    float average_rating;
};
```

## Implementation Notes

### Critical Sections
- **Grouping logic**: Must handle edge cases (missing metadata)
- **Custom patterns**: Validate titleformat syntax
- **Large libraries**: Optimize for 10,000+ albums

### Error Handling
- **Invalid titleformat**: Fall back to default grouping
- **Missing metadata**: Use sensible defaults
- **Memory pressure**: Limit cache size

### Performance Considerations
- Lazy evaluation of calculated fields
- Cache titleformat compilation results
- Use hash maps for grouping operations

## Testing

### Test Coverage
- [ ] All 13 grouping modes
- [ ] All 11 sorting options  
- [ ] Custom titleformat patterns
- [ ] Search with Unicode text
- [ ] Large library performance (10k+ albums)
- [ ] Empty library handling

### Known Issues
- None yet (new implementation)

## Change Log
| Date | Version | Change | Author |
|------|---------|--------|--------|
| 2025-01-09 | 0.0.1 | Initial documentation | System |