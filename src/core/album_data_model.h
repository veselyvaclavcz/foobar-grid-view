#pragma once

#include "../foundation/lifecycle_manager.h"
#include "../foundation/callback_manager.h"
#include <foobar2000/SDK/foobar2000.h>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace albumart_grid {

// Grouping modes
enum class GroupingMode {
    FOLDER_STRUCTURE,
    ALBUM,
    ARTIST,
    ALBUM_ARTIST,
    YEAR,
    GENRE,
    DATE_MODIFIED,
    DATE_ADDED,
    FILE_SIZE,
    TRACK_COUNT,
    RATING,
    PLAYCOUNT,
    CUSTOM
};

// Sorting modes
enum class SortingMode {
    NAME,
    ARTIST,
    ALBUM,
    YEAR,
    GENRE,
    DATE_MODIFIED,
    TOTAL_SIZE,
    TRACK_COUNT,
    RATING,
    PATH,
    RANDOM
};

// Track sorting within albums
enum class TrackSortMode {
    TRACK_NUMBER,
    TITLE,
    FILENAME,
    PATH
};

// View mode
enum class ViewMode {
    LIBRARY,
    PLAYLIST
};

// Album item representing a group of tracks
struct album_item {
    metadb_handle_list tracks;
    pfc::string8 primary_text;    // Main display text
    pfc::string8 secondary_text;   // Secondary info
    pfc::string8 sort_key;        // For sorting
    pfc::string8 search_text;     // Combined searchable text
    
    // Calculated fields
    uint64_t total_size = 0;
    uint32_t track_count = 0;
    float average_rating = 0.0f;
    
    // State
    bool is_now_playing = false;
    bool is_selected = false;
    bool matches_filter = true;
};

// Data model for album grid
class AlbumDataModel : public ILifecycleAware,
                       public std::enable_shared_from_this<AlbumDataModel> {
public:
    AlbumDataModel();
    ~AlbumDataModel();
    
    // ILifecycleAware implementation
    void on_initialize() override;
    void on_shutdown() override;
    bool is_valid() const override;
    
    // Configuration
    void set_grouping(GroupingMode mode, const char* custom_pattern = nullptr);
    void set_sorting(SortingMode mode, bool descending = false);
    void set_track_sorting(TrackSortMode mode);
    void set_view_mode(ViewMode mode);
    
    // Get current settings
    GroupingMode get_grouping() const { return m_grouping_mode; }
    SortingMode get_sorting() const { return m_sorting_mode; }
    bool is_descending() const { return m_sort_descending; }
    ViewMode get_view_mode() const { return m_view_mode; }
    
    // Data access
    const std::vector<album_item>& get_items() const { return m_items; }
    size_t get_item_count() const { return m_items.size(); }
    const album_item* get_item(size_t index) const;
    
    // Search/filter
    void set_filter(const char* search_text);
    void clear_filter();
    bool has_filter() const { return !m_filter_text.is_empty(); }
    
    // Find items
    int find_item_for_track(metadb_handle_ptr track) const;
    int find_now_playing_item() const;
    
    // Selection
    void set_selected(size_t index, bool selected);
    void clear_selection();
    std::vector<size_t> get_selected_indices() const;
    
    // Refresh data
    void refresh();
    void refresh_from_library();
    void refresh_from_playlist();
    
    // Callbacks for data changes
    using change_callback = std::function<void()>;
    void add_change_callback(change_callback callback);
    
private:
    // Internal methods
    void group_tracks(const metadb_handle_list& tracks);
    void sort_items();
    void apply_filter();
    void update_now_playing();
    void calculate_item_fields(album_item& item);
    pfc::string8 format_grouping_key(const metadb_handle_ptr& track);
    pfc::string8 format_sort_key(const album_item& item);
    
    // Notify callbacks
    void notify_changes();
    
    // Settings
    GroupingMode m_grouping_mode = GroupingMode::ALBUM;
    SortingMode m_sorting_mode = SortingMode::NAME;
    TrackSortMode m_track_sort_mode = TrackSortMode::TRACK_NUMBER;
    ViewMode m_view_mode = ViewMode::LIBRARY;
    bool m_sort_descending = false;
    pfc::string8 m_custom_pattern;
    pfc::string8 m_filter_text;
    
    // Data
    std::vector<album_item> m_items;
    std::vector<album_item> m_filtered_items;
    
    // State
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shutting_down{false};
    
    // Synchronization
    mutable std::mutex m_data_mutex;
    std::vector<change_callback> m_change_callbacks;
    
    // Services
    static_api_ptr_t<library_manager> m_library_manager;
    static_api_ptr_t<playlist_manager> m_playlist_manager;
    static_api_ptr_t<titleformat_compiler> m_titleformat_compiler;
    static_api_ptr_t<playback_control> m_playback_control;
};

// Callback handler for library changes
class library_callback_handler : public callback_handler<library_callback> {
public:
    explicit library_callback_handler(std::weak_ptr<AlbumDataModel> model);
    
    // library_callback methods
    void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr>& items) override;
    void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr>& items) override;
    void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr>& items) override;
    
private:
    std::weak_ptr<AlbumDataModel> m_model;
};

// Callback handler for playlist changes
class playlist_callback_handler : public callback_handler<playlist_callback_single>,
                                 public playlist_callback_single {
public:
    explicit playlist_callback_handler(std::weak_ptr<AlbumDataModel> model);
    
    // playlist_callback_single methods
    void on_items_added(t_size playlist, t_size start, 
                        const pfc::list_base_const_t<metadb_handle_ptr>& items, 
                        const pfc::bit_array& selection) override;
    void on_items_reordered(t_size playlist, const t_size* order, t_size count) override;
    void on_items_removing(t_size playlist, const pfc::bit_array& mask, 
                           t_size old_count, t_size new_count) override;
    void on_items_removed(t_size playlist, const pfc::bit_array& mask, 
                         t_size old_count, t_size new_count) override;
    void on_items_selection_change(t_size playlist, const pfc::bit_array& affected, 
                                   const pfc::bit_array& state) override;
    void on_item_focus_change(t_size playlist, t_size from, t_size to) override;
    void on_items_modified(t_size playlist, const pfc::bit_array& mask) override;
    void on_items_modified_fromplayback(t_size playlist, const pfc::bit_array& mask, 
                                       play_control::t_display_level level) override;
    void on_items_replaced(t_size playlist, const pfc::bit_array& mask, 
                          const pfc::list_base_const_t<t_on_items_replaced_entry>& data) override;
    void on_item_ensure_visible(t_size playlist, t_size item) override;
    void on_playlist_activate(t_size old_playlist, t_size new_playlist) override;
    void on_playlist_created(t_size playlist, const char* name, t_size name_len) override;
    void on_playlists_reorder(const t_size* order, t_size count) override;
    void on_playlists_removing(const pfc::bit_array& mask, t_size old_count, 
                               t_size new_count) override;
    void on_playlists_removed(const pfc::bit_array& mask, t_size old_count, 
                             t_size new_count) override;
    void on_playlist_renamed(t_size playlist, const char* new_name, t_size new_name_len) override;
    void on_default_format_changed() override {}
    void on_playback_order_changed(t_size new_order) override {}
    void on_playlist_locked(t_size playlist, bool locked) override {}
    
private:
    std::weak_ptr<AlbumDataModel> m_model;
    t_size m_monitored_playlist;
};

} // namespace albumart_grid