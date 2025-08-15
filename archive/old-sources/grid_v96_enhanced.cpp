// Album Art Grid Component v9.6 - Enhanced with Track Count Badge and Themed Scrollbar
// Added: Track count badge when labels hidden, themed scrollbar using foobar2000 colors

#define FOOBAR2000_TARGET_VERSION 82
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "SDK-2025-03-07/foobar2000/helpers/helpers.h"

// Component validation
#ifdef _DEBUG
#define COMPONENT_NAME "foo_albumart_grid_v96_debug"
#else
#define COMPONENT_NAME "foo_albumart_grid_v96"
#endif

VALIDATE_COMPONENT_FILENAME(COMPONENT_NAME ".dll");

// GUIDs
static const GUID guid_config_columns = { 0x1234abcd, 0x5678, 0x9012, { 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78 } };
static const GUID guid_config_grouping = { 0x1234abcd, 0x5678, 0x9012, { 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x79 } };
static const GUID guid_config_text_lines = { 0x1234abcd, 0x5678, 0x9012, { 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x7a } };
static const GUID guid_config_show_text = { 0x1234abcd, 0x5678, 0x9012, { 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x7b } };
static const GUID guid_config_show_track_count = { 0x1234abcd, 0x5678, 0x9012, { 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x7c } };
static const GUID guid_config_sort_mode = { 0x1234abcd, 0x5678, 0x9012, { 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x7d } };
static const GUID guid_ui_element = { 0x98765432, 0x1234, 0x5678, { 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34 } };

// Configuration
static cfg_int cfg_columns(guid_config_columns, 3);
static cfg_int cfg_grouping(guid_config_grouping, 0);
static cfg_int cfg_text_lines(guid_config_text_lines, 2);
static cfg_bool cfg_show_text(guid_config_show_text, true);
static cfg_bool cfg_show_track_count(guid_config_show_track_count, true);
static cfg_int cfg_sort_mode(guid_config_sort_mode, 0);

// Constants
const int PADDING = 10;
const int MIN_ITEM_SIZE = 50;
const int MAX_ITEM_SIZE = 250;
const int DEFAULT_ITEM_SIZE = 120;
const int MIN_COLUMNS = 3;
const int MAX_COLUMNS = 10;
const int SCROLLBAR_WIDTH = 14;  // Custom scrollbar width

enum GroupingMode {
    GROUP_BY_FOLDER = 0,
    GROUP_BY_DIRECTORY,
    GROUP_BY_ALBUM,
    GROUP_BY_ARTIST,
    GROUP_BY_ALBUM_ARTIST,
    GROUP_BY_ARTIST_ALBUM,
    GROUP_BY_PERFORMER,
    GROUP_BY_COMPOSER,
    GROUP_BY_GENRE,
    GROUP_BY_YEAR,
    GROUP_BY_LABEL,
    GROUP_BY_RATING,
    GROUP_BY_COMMENT
};

enum SortMode {
    SORT_BY_NAME = 0,
    SORT_BY_ARTIST,
    SORT_BY_ALBUM,
    SORT_BY_YEAR,
    SORT_BY_GENRE,
    SORT_BY_DATE_MODIFIED,
    SORT_BY_TOTAL_SIZE,
    SORT_BY_TRACK_COUNT,
    SORT_BY_RATING,
    SORT_BY_PATH,
    SORT_RANDOM_SHUFFLE
};

struct Config {
    int columns = 3;
    GroupingMode grouping = GROUP_BY_FOLDER;
    int text_lines = 2;
    bool show_text = true;
    bool show_track_count = true;
    SortMode sort_mode = SORT_BY_NAME;
    
    void load() {
        columns = cfg_columns.get_value();
        grouping = (GroupingMode)cfg_grouping.get_value();
        text_lines = cfg_text_lines.get_value();
        show_text = cfg_show_text.get_value();
        show_track_count = cfg_show_track_count.get_value();
        sort_mode = (SortMode)cfg_sort_mode.get_value();
    }
    
    void save() {
        cfg_columns = columns;
        cfg_grouping = grouping;
        cfg_text_lines = text_lines;
        cfg_show_text = show_text;
        cfg_show_track_count = show_track_count;
        cfg_sort_mode = sort_mode;
    }
};

// Helper function to determine if we should try artist image first
bool should_use_artist_image(GroupingMode mode) {
    return mode == GROUP_BY_ARTIST || 
           mode == GROUP_BY_ALBUM_ARTIST || 
           mode == GROUP_BY_ARTIST_ALBUM ||
           mode == GROUP_BY_PERFORMER ||
           mode == GROUP_BY_COMPOSER;
}

// Helper function to get appropriate album art with fallback
album_art_data_ptr get_album_art_with_fallback(
    album_art_manager_v2::ptr art_api,
    metadb_handle_ptr track,
    GroupingMode grouping_mode,
    abort_callback& abort) {
    
    album_art_data_ptr result;
    
    // Determine primary art type based on grouping mode
    if (should_use_artist_image(grouping_mode)) {
        // Try artist image first
        try {
            auto extractor = art_api->open(
                pfc::list_single_ref_t<metadb_handle_ptr>(track),
                pfc::list_single_ref_t<GUID>(album_art_ids::artist),
                abort
            );
            
            result = extractor->query(album_art_ids::artist, abort);
            if (result.is_valid() && result->get_size() > 0) {
                return result;
            }
        } catch (...) {
            // Artist image not found, will fall back to cover
        }
    }
    
    // Fall back to front cover (or use it as primary for non-artist groupings)
    try {
        auto extractor = art_api->open(
            pfc::list_single_ref_t<metadb_handle_ptr>(track),
            pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),
            abort
        );
        
        result = extractor->query(album_art_ids::cover_front, abort);
        if (result.is_valid()) {
            return result;
        }
    } catch (...) {}
    
    return album_art_data_ptr();
}

struct GridItem {
    pfc::string8 key;
    pfc::string8 display_name;
    pfc::string8 sort_key;
    metadb_handle_list tracks;
    album_art_data_ptr artwork;
    
    struct ThumbnailData {
        Gdiplus::Bitmap* bitmap = nullptr;
        int cached_size = 0;
        
        ~ThumbnailData() {
            if (bitmap) delete bitmap;
        }
    };
    std::unique_ptr<ThumbnailData> thumbnail;
    
    GridItem() : thumbnail(std::make_unique<ThumbnailData>()) {}
    ~GridItem() = default;
};

// Forward declaration
class AlbumArtGridElement;

class AlbumArtGridWindow : public CWindowImpl<AlbumArtGridWindow> {
public:
    DECLARE_WND_CLASS_EX(TEXT("{98765432-1234-5678-9012-345678901234}"), CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, (-1));
    
    BEGIN_MSG_MAP(AlbumArtGridWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()
    
    void initialize(ui_element_instance_callback::ptr callback) {
        m_callback = callback;
        m_config.load();
    }
    
    void refresh_library() {
        m_items.clear();
        m_selected_indices.clear();
        m_last_selected = -1;
        
        try {
            auto api = static_api_ptr_t<library_manager>();
            metadb_handle_list all_items;
            api->get_all_items(all_items);
            
            if (all_items.get_count() == 0) {
                InvalidateRect(m_hwnd, NULL, TRUE);
                return;
            }
            
            // Group tracks
            std::map<pfc::string8, std::shared_ptr<GridItem>> groups;
            
            for (t_size i = 0; i < all_items.get_count(); i++) {
                auto track = all_items.get_item(i);
                pfc::string8 group_key;
                pfc::string8 display_name;
                pfc::string8 sort_key;
                
                switch (m_config.grouping) {
                case GROUP_BY_FOLDER: {
                    const char* path = track->get_path();
                    const char* last_slash = strrchr(path, '\\');
                    if (last_slash) {
                        group_key.set_string(path, last_slash - path);
                        display_name = group_key;
                    } else {
                        group_key = path;
                        display_name = path;
                    }
                    break;
                }
                
                case GROUP_BY_DIRECTORY: {
                    const char* path = track->get_path();
                    const char* last_slash = strrchr(path, '\\');
                    if (last_slash) {
                        const char* second_last = last_slash - 1;
                        while (second_last >= path && *second_last != '\\') second_last--;
                        if (second_last >= path) {
                            group_key.set_string(second_last + 1, last_slash - second_last - 1);
                            display_name = group_key;
                        } else {
                            group_key = "Root";
                            display_name = "Root";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_ALBUM: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* album = info.meta_get("album", 0);
                        if (album) {
                            group_key = album;
                            display_name = album;
                        } else {
                            group_key = "Unknown Album";
                            display_name = "Unknown Album";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_ARTIST: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* artist = info.meta_get("artist", 0);
                        if (artist) {
                            group_key = artist;
                            display_name = artist;
                        } else {
                            group_key = "Unknown Artist";
                            display_name = "Unknown Artist";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_ALBUM_ARTIST: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* album_artist = info.meta_get("album artist", 0);
                        if (!album_artist) album_artist = info.meta_get("artist", 0);
                        if (album_artist) {
                            group_key = album_artist;
                            display_name = album_artist;
                        } else {
                            group_key = "Unknown Artist";
                            display_name = "Unknown Artist";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_ARTIST_ALBUM: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* artist = info.meta_get("artist", 0);
                        const char* album = info.meta_get("album", 0);
                        if (artist && album) {
                            group_key << artist << " - " << album;
                            display_name = group_key;
                        } else if (artist) {
                            group_key = artist;
                            display_name = artist;
                        } else if (album) {
                            group_key = album;
                            display_name = album;
                        } else {
                            group_key = "Unknown";
                            display_name = "Unknown";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_PERFORMER: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* performer = info.meta_get("performer", 0);
                        if (!performer) performer = info.meta_get("artist", 0);
                        if (performer) {
                            group_key = performer;
                            display_name = performer;
                        } else {
                            group_key = "Unknown Performer";
                            display_name = "Unknown Performer";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_COMPOSER: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* composer = info.meta_get("composer", 0);
                        if (composer) {
                            group_key = composer;
                            display_name = composer;
                        } else {
                            group_key = "Unknown Composer";
                            display_name = "Unknown Composer";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_GENRE: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* genre = info.meta_get("genre", 0);
                        if (genre) {
                            group_key = genre;
                            display_name = genre;
                        } else {
                            group_key = "Unknown Genre";
                            display_name = "Unknown Genre";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_YEAR: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* year = info.meta_get("date", 0);
                        if (year) {
                            // Extract just the year part
                            pfc::string8 year_str = year;
                            if (year_str.length() >= 4) {
                                year_str.truncate(4);
                                group_key = year_str;
                                display_name = year_str;
                            } else {
                                group_key = year;
                                display_name = year;
                            }
                        } else {
                            group_key = "Unknown Year";
                            display_name = "Unknown Year";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_LABEL: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* label = info.meta_get("label", 0);
                        if (!label) label = info.meta_get("publisher", 0);
                        if (label) {
                            group_key = label;
                            display_name = label;
                        } else {
                            group_key = "Unknown Label";
                            display_name = "Unknown Label";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_RATING: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* rating = info.meta_get("rating", 0);
                        if (rating) {
                            group_key = rating;
                            display_name << rating << " Stars";
                        } else {
                            group_key = "Unrated";
                            display_name = "Unrated";
                        }
                    }
                    break;
                }
                
                case GROUP_BY_COMMENT: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* comment = info.meta_get("comment", 0);
                        if (comment) {
                            // Truncate long comments for grouping
                            pfc::string8 comment_str = comment;
                            if (comment_str.length() > 50) {
                                comment_str.truncate(47);
                                comment_str += "...";
                            }
                            group_key = comment_str;
                            display_name = comment_str;
                        } else {
                            group_key = "No Comment";
                            display_name = "No Comment";
                        }
                    }
                    break;
                }
                }
                
                // Generate sort key based on sort mode
                switch (m_config.sort_mode) {
                    case SORT_BY_NAME:
                        sort_key = display_name;
                        break;
                    case SORT_BY_ARTIST: {
                        file_info_impl info;
                        if (track->get_info(info)) {
                            const char* artist = info.meta_get("artist", 0);
                            sort_key = artist ? artist : "";
                        }
                        break;
                    }
                    case SORT_BY_ALBUM: {
                        file_info_impl info;
                        if (track->get_info(info)) {
                            const char* album = info.meta_get("album", 0);
                            sort_key = album ? album : "";
                        }
                        break;
                    }
                    case SORT_BY_YEAR: {
                        file_info_impl info;
                        if (track->get_info(info)) {
                            const char* year = info.meta_get("date", 0);
                            sort_key = year ? year : "9999";
                        }
                        break;
                    }
                    case SORT_BY_GENRE: {
                        file_info_impl info;
                        if (track->get_info(info)) {
                            const char* genre = info.meta_get("genre", 0);
                            sort_key = genre ? genre : "";
                        }
                        break;
                    }
                    case SORT_BY_PATH:
                        sort_key = track->get_path();
                        break;
                    default:
                        sort_key = display_name;
                        break;
                }
                
                auto it = groups.find(group_key);
                if (it == groups.end()) {
                    auto item = std::make_shared<GridItem>();
                    item->key = group_key;
                    item->display_name = display_name;
                    item->sort_key = sort_key;
                    item->tracks.add_item(track);
                    groups[group_key] = item;
                } else {
                    it->second->tracks.add_item(track);
                }
            }
            
            // Convert map to vector
            m_items.reserve(groups.size());
            for (auto& pair : groups) {
                m_items.push_back(pair.second);
            }
            
            // Sort items
            sort_items();
            
            // Load album art
            load_album_art();
        } catch (std::exception const& e) {
            console::error(e.what());
        }
        
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    
private:
    HWND m_hwnd;
    ui_element_instance_callback::ptr m_callback;
    Config m_config;
    std::vector<std::shared_ptr<GridItem>> m_items;
    std::set<int> m_selected_indices;
    int m_last_selected;
    int m_scroll_pos;
    int m_item_size = DEFAULT_ITEM_SIZE;
    HWND m_tooltip;
    int m_hover_index = -1;
    
    // Custom scrollbar tracking
    bool m_scrollbar_hovered = false;
    bool m_scrollbar_dragging = false;
    int m_scrollbar_drag_offset = 0;
    
    void sort_items() {
        if (m_config.sort_mode == SORT_RANDOM_SHUFFLE) {
            // Random shuffle
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(m_items.begin(), m_items.end(), g);
            return;
        }
        
        // Special handling for certain sort modes
        if (m_config.sort_mode == SORT_BY_DATE_MODIFIED ||
            m_config.sort_mode == SORT_BY_TOTAL_SIZE ||
            m_config.sort_mode == SORT_BY_TRACK_COUNT ||
            m_config.sort_mode == SORT_BY_RATING) {
            
            std::sort(m_items.begin(), m_items.end(),
                [this](const std::shared_ptr<GridItem>& a, const std::shared_ptr<GridItem>& b) {
                    switch (m_config.sort_mode) {
                        case SORT_BY_DATE_MODIFIED: {
                            // Get most recent modification time from tracks
                            t_filetimestamp time_a = 0, time_b = 0;
                            for (t_size i = 0; i < a->tracks.get_count(); i++) {
                                t_filetimestamp track_time = a->tracks.get_item(i)->get_filetimestamp();
                                if (track_time > time_a) time_a = track_time;
                            }
                            for (t_size i = 0; i < b->tracks.get_count(); i++) {
                                t_filetimestamp track_time = b->tracks.get_item(i)->get_filetimestamp();
                                if (track_time > time_b) time_b = track_time;
                            }
                            return time_a > time_b;  // Most recent first
                        }
                        
                        case SORT_BY_TOTAL_SIZE: {
                            // Calculate total file size
                            t_uint64 size_a = 0, size_b = 0;
                            for (t_size i = 0; i < a->tracks.get_count(); i++) {
                                size_a += a->tracks.get_item(i)->get_filesize();
                            }
                            for (t_size i = 0; i < b->tracks.get_count(); i++) {
                                size_b += b->tracks.get_item(i)->get_filesize();
                            }
                            return size_a > size_b;  // Largest first
                        }
                        
                        case SORT_BY_TRACK_COUNT:
                            return a->tracks.get_count() > b->tracks.get_count();  // Most tracks first
                        
                        case SORT_BY_RATING: {
                            // Get average rating
                            int rating_a = 0, rating_b = 0;
                            int count_a = 0, count_b = 0;
                            
                            for (t_size i = 0; i < a->tracks.get_count(); i++) {
                                file_info_impl info;
                                if (a->tracks.get_item(i)->get_info(info)) {
                                    const char* rating = info.meta_get("rating", 0);
                                    if (rating) {
                                        rating_a += atoi(rating);
                                        count_a++;
                                    }
                                }
                            }
                            
                            for (t_size i = 0; i < b->tracks.get_count(); i++) {
                                file_info_impl info;
                                if (b->tracks.get_item(i)->get_info(info)) {
                                    const char* rating = info.meta_get("rating", 0);
                                    if (rating) {
                                        rating_b += atoi(rating);
                                        count_b++;
                                    }
                                }
                            }
                            
                            float avg_a = count_a > 0 ? (float)rating_a / count_a : 0;
                            float avg_b = count_b > 0 ? (float)rating_b / count_b : 0;
                            return avg_a > avg_b;  // Highest rating first
                        }
                        
                        default:
                            return false;
                    }
                });
        } else {
            // Standard sort by sort_key
            std::sort(m_items.begin(), m_items.end(),
                [](const std::shared_ptr<GridItem>& a, const std::shared_ptr<GridItem>& b) {
                    return stricmp_utf8(a->sort_key.c_str(), b->sort_key.c_str()) < 0;
                });
        }
    }
    
    void load_album_art() {
        try {
            auto art_api = static_api_ptr_t<album_art_manager_v2>();
            
            for (auto& item : m_items) {
                if (item->tracks.get_count() > 0) {
                    abort_callback_dummy abort;
                    
                    item->artwork = get_album_art_with_fallback(
                        art_api,
                        item->tracks.get_item(0),
                        m_config.grouping,
                        abort
                    );
                    
                    if (item->artwork.is_valid()) {
                        calculate_layout();  // Update m_item_size
                        if (item->thumbnail->bitmap) {
                            delete item->thumbnail->bitmap;
                        }
                        item->thumbnail->bitmap = create_thumbnail(item->artwork, m_item_size);
                        item->thumbnail->cached_size = m_item_size;
                        InvalidateRect(m_hwnd, NULL, FALSE);
                    }
                }
            }
        } catch (...) {}
    }
    
    Gdiplus::Bitmap* create_thumbnail(album_art_data_ptr data, int size) {
        if (!data.is_valid()) return nullptr;
        
        IStream* stream = SHCreateMemStream(
            static_cast<const BYTE*>(data->get_ptr()),
            data->get_size()
        );
        
        if (!stream) return nullptr;
        
        Gdiplus::Bitmap* source = Gdiplus::Bitmap::FromStream(stream);
        stream->Release();
        
        if (!source || source->GetLastStatus() != Gdiplus::Ok) {
            if (source) delete source;
            return nullptr;
        }
        
        // Create high-quality thumbnail
        auto* thumbnail = new Gdiplus::Bitmap(size, size, PixelFormat32bppARGB);
        Gdiplus::Graphics graphics(thumbnail);
        
        // Set high quality rendering
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        
        // Calculate aspect ratio preserving dimensions
        int src_width = source->GetWidth();
        int src_height = source->GetHeight();
        float scale = min((float)size / src_width, (float)size / src_height);
        int dst_width = (int)(src_width * scale);
        int dst_height = (int)(src_height * scale);
        int x = (size - dst_width) / 2;
        int y = (size - dst_height) / 2;
        
        // Clear background and draw scaled image
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
        graphics.DrawImage(source, x, y, dst_width, dst_height);
        
        delete source;
        return thumbnail;
    }
    
    void calculate_layout() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int width = rc.right - SCROLLBAR_WIDTH;  // Account for custom scrollbar
        
        if (width <= 0 || m_config.columns <= 0) {
            m_item_size = DEFAULT_ITEM_SIZE;
            return;
        }
        
        // Calculate item size based on available width and columns
        int total_padding = PADDING * (m_config.columns + 1);
        int available_for_items = width - total_padding;
        m_item_size = max(MIN_ITEM_SIZE, min(MAX_ITEM_SIZE, available_for_items / m_config.columns));
    }
    
    int calculate_text_height() {
        if (!m_config.show_text || !m_callback.is_valid()) {
            return 0;
        }
        
        // Get font from foobar2000
        t_ui_font font_id = ui_font_default;
        auto font_api = static_api_ptr_t<fonts::manager>();
        auto font_ptr = font_api->get_font(font_id);
        
        if (!font_ptr.is_valid()) {
            return m_config.text_lines * 14 + 4;  // Fallback
        }
        
        LOGFONT lf = font_ptr->get_logfont();
        HFONT hFont = CreateFontIndirect(&lf);
        HDC hdc = GetDC(m_hwnd);
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        int line_height = tm.tmHeight + tm.tmExternalLeading;
        
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
        ReleaseDC(m_hwnd, hdc);
        
        return m_config.text_lines * line_height + 4;  // Add small padding
    }
    
    void update_scrollbar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        
        int cols = m_config.columns;
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + PADDING) + PADDING;
        
        // We'll draw our own scrollbar, so just track the values
        int max_scroll = max(0, total_height - rc.bottom);
        if (m_scroll_pos > max_scroll) {
            m_scroll_pos = max_scroll;
        }
        
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void draw_track_count_badge(HDC hdc, int count, int x, int y, bool dark_mode) {
        // Prepare text
        char count_str[32];
        sprintf_s(count_str, "%d", count);
        
        // Calculate text size
        SIZE text_size;
        GetTextExtentPoint32A(hdc, count_str, strlen(count_str), &text_size);
        
        // Badge dimensions
        int badge_width = max(20, text_size.cx + 10);
        int badge_height = 20;
        int badge_x = x - badge_width - 5;  // Top-right corner
        int badge_y = y + 5;
        
        // Draw badge background with GDI+
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, badge_width, badge_height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
        
        Gdiplus::Graphics graphics(hdcMem);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        
        // Create rounded rectangle path for badge
        Gdiplus::GraphicsPath path;
        int radius = badge_height / 2;
        path.AddArc(0, 0, radius * 2, radius * 2, 180, 90);
        path.AddArc(badge_width - radius * 2, 0, radius * 2, radius * 2, 270, 90);
        path.AddArc(badge_width - radius * 2, badge_height - radius * 2, radius * 2, radius * 2, 0, 90);
        path.AddArc(0, badge_height - radius * 2, radius * 2, radius * 2, 90, 90);
        path.CloseFigure();
        
        // Fill badge with semi-transparent background
        Gdiplus::Color badge_color = dark_mode ? 
            Gdiplus::Color(200, 40, 40, 40) :  // Dark mode: semi-transparent dark
            Gdiplus::Color(200, 255, 60, 60);  // Light mode: semi-transparent red
        
        Gdiplus::SolidBrush brush(badge_color);
        graphics.FillPath(&brush, &path);
        
        // Draw border
        Gdiplus::Color border_color = dark_mode ?
            Gdiplus::Color(255, 80, 80, 80) :
            Gdiplus::Color(255, 200, 50, 50);
        Gdiplus::Pen pen(border_color, 1.0f);
        graphics.DrawPath(&pen, &path);
        
        // Draw text
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));  // White text
        
        RECT text_rect = { 0, 0, badge_width, badge_height };
        DrawTextA(hdcMem, count_str, -1, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // Blit badge to main DC with alpha blending
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 200, 0 };  // 200/255 opacity
        AlphaBlend(hdc, badge_x, badge_y, badge_width, badge_height,
                   hdcMem, 0, 0, badge_width, badge_height, blend);
        
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
    }
    
    void draw_custom_scrollbar(HDC hdc, RECT& client_rc, bool dark_mode) {
        // Calculate scrollbar area
        RECT scrollbar_rc = {
            client_rc.right - SCROLLBAR_WIDTH,
            0,
            client_rc.right,
            client_rc.bottom
        };
        
        // Get scroll metrics
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + PADDING) + PADDING;
        
        bool needs_scrollbar = total_height > client_rc.bottom;
        
        if (!needs_scrollbar) {
            // No scrollbar needed - fill with background color
            HBRUSH hbrBg = CreateSolidBrush(dark_mode ? RGB(30, 30, 30) : RGB(255, 255, 255));
            FillRect(hdc, &scrollbar_rc, hbrBg);
            DeleteObject(hbrBg);
            return;
        }
        
        // Draw scrollbar background
        HBRUSH hbrTrack = CreateSolidBrush(dark_mode ? RGB(45, 45, 45) : RGB(240, 240, 240));
        FillRect(hdc, &scrollbar_rc, hbrTrack);
        DeleteObject(hbrTrack);
        
        // Calculate thumb size and position
        float visible_ratio = (float)client_rc.bottom / total_height;
        int thumb_height = max(30, (int)(client_rc.bottom * visible_ratio));
        
        float scroll_ratio = (float)m_scroll_pos / (total_height - client_rc.bottom);
        int thumb_y = (int)((client_rc.bottom - thumb_height) * scroll_ratio);
        
        // Draw thumb
        RECT thumb_rc = {
            scrollbar_rc.left + 2,
            thumb_y,
            scrollbar_rc.right - 2,
            thumb_y + thumb_height
        };
        
        COLORREF thumb_color;
        if (m_scrollbar_dragging) {
            thumb_color = dark_mode ? RGB(100, 100, 100) : RGB(120, 120, 120);
        } else if (m_scrollbar_hovered) {
            thumb_color = dark_mode ? RGB(80, 80, 80) : RGB(160, 160, 160);
        } else {
            thumb_color = dark_mode ? RGB(60, 60, 60) : RGB(190, 190, 190);
        }
        
        HBRUSH hbrThumb = CreateSolidBrush(thumb_color);
        FillRect(hdc, &thumb_rc, hbrThumb);
        DeleteObject(hbrThumb);
    }
    
    void paint(HDC hdc) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Create memory DC for double buffering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        
        // Initialize GDI+
        Gdiplus::Graphics graphics(hdcMem);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        
        // Fill background
        bool dark_mode = m_callback.is_valid() ? m_callback->is_dark_mode() : false;
        HBRUSH hbrBg = CreateSolidBrush(dark_mode ? RGB(30, 30, 30) : RGB(255, 255, 255));
        FillRect(hdcMem, &rc, hbrBg);
        DeleteObject(hbrBg);
        
        // Draw edit mode indicator if needed
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 165, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            
            Rectangle(hdcMem, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
            
            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hPen);
        }
        
        // Get font
        HFONT hFont = NULL;
        if (m_callback.is_valid()) {
            t_ui_font font_id = ui_font_default;
            auto font_api = static_api_ptr_t<fonts::manager>();
            auto font_ptr = font_api->get_font(font_id);
            if (font_ptr.is_valid()) {
                LOGFONT lf = font_ptr->get_logfont();
                hFont = CreateFontIndirect(&lf);
                SelectObject(hdcMem, hFont);
            }
        }
        
        // Calculate layout
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        
        // Adjust drawing area for custom scrollbar
        int drawing_width = rc.right - SCROLLBAR_WIDTH;
        
        // Center the grid horizontally
        int total_width = cols * m_item_size + (cols + 1) * PADDING;
        int start_x = (drawing_width - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Calculate visible range
        int first_visible_row = m_scroll_pos / (item_total_height + PADDING);
        int last_visible_row = (m_scroll_pos + rc.bottom) / (item_total_height + PADDING) + 1;
        int first_visible_item = first_visible_row * cols;
        int last_visible_item = min((int)m_items.size(), (last_visible_row + 1) * cols);
        
        // Draw items
        for (int i = first_visible_item; i < last_visible_item; i++) {
            if (i >= m_items.size()) break;
            
            auto& item = m_items[i];
            
            int col = i % cols;
            int row = i / cols;
            
            int x = start_x + col * (m_item_size + PADDING) + PADDING;
            int y = row * (item_total_height + PADDING) + PADDING - m_scroll_pos;
            
            // Draw selection/hover background
            bool selected = m_selected_indices.find(i) != m_selected_indices.end();
            bool hovered = (i == m_hover_index);
            
            if (selected || hovered) {
                COLORREF color;
                if (selected) {
                    color = dark_mode ? RGB(51, 153, 255) : RGB(51, 153, 255);
                } else {
                    color = dark_mode ? RGB(60, 60, 60) : RGB(230, 230, 230);
                }
                
                HBRUSH hbr = CreateSolidBrush(color);
                RECT item_rc = {x - 2, y - 2, x + m_item_size + 2, y + item_total_height + 2};
                FillRect(hdcMem, &item_rc, hbr);
                DeleteObject(hbr);
            }
            
            // Draw album art
            if (item->thumbnail->bitmap) {
                // Check if we need to regenerate thumbnail at different size
                if (item->thumbnail->cached_size != m_item_size && item->artwork.is_valid()) {
                    delete item->thumbnail->bitmap;
                    item->thumbnail->bitmap = create_thumbnail(item->artwork, m_item_size);
                    item->thumbnail->cached_size = m_item_size;
                }
                
                if (item->thumbnail->bitmap) {
                    graphics.DrawImage(item->thumbnail->bitmap, x, y, m_item_size, m_item_size);
                }
            } else {
                // Draw placeholder
                HBRUSH hbr = CreateSolidBrush(dark_mode ? RGB(50, 50, 50) : RGB(200, 200, 200));
                RECT art_rc = {x, y, x + m_item_size, y + m_item_size};
                FillRect(hdcMem, &art_rc, hbr);
                DeleteObject(hbr);
                
                // Draw icon to indicate type when no image available
                if (should_use_artist_image(m_config.grouping)) {
                    // Draw a person icon for artist
                    SetTextColor(hdcMem, dark_mode ? RGB(150, 150, 150) : RGB(100, 100, 100));
                    RECT icon_rc = {x, y + m_item_size/3, x + m_item_size, y + 2*m_item_size/3};
                    DrawText(hdcMem, TEXT("👤"), -1, &icon_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                } else {
                    // Draw a disc icon for album
                    SetTextColor(hdcMem, dark_mode ? RGB(150, 150, 150) : RGB(100, 100, 100));
                    RECT icon_rc = {x, y + m_item_size/3, x + m_item_size, y + 2*m_item_size/3};
                    DrawText(hdcMem, TEXT("💿"), -1, &icon_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            }
            
            // Draw track count badge if text is hidden and track count is enabled
            if (!m_config.show_text && m_config.show_track_count && item->tracks.get_count() > 0) {
                draw_track_count_badge(hdcMem, item->tracks.get_count(), 
                                     x + m_item_size, y, dark_mode);
            }
            
            // Draw text
            if (m_config.show_text && text_height > 0) {
                draw_item_text(hdcMem, item.get(), x, y + m_item_size, m_item_size, text_height, dark_mode);
            }
        }
        
        // Draw custom scrollbar
        draw_custom_scrollbar(hdcMem, rc, dark_mode);
        
        // Blit to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
        
        if (hFont) DeleteObject(hFont);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
    }
    
    void draw_item_text(HDC hdc, GridItem* item, int x, int y, int width, int height, bool dark_mode) {
        RECT text_rc = {x, y, x + width, y + height};
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, dark_mode ? RGB(200, 200, 200) : RGB(50, 50, 50));
        
        // Build display text
        pfc::stringcvt::string_os_from_utf8 display_text(item->display_name);
        if (m_config.show_track_count && item->tracks.get_count() > 0) {
            pfc::string8 text_with_count;
            text_with_count << item->display_name << " (" << item->tracks.get_count() << ")";
            display_text = pfc::stringcvt::string_os_from_utf8(text_with_count.c_str());
        }
        
        // Draw with word wrap for multi-line support
        UINT format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;
        if (m_config.text_lines == 1) {
            format = DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;
        } else if (m_config.text_lines == 2) {
            format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;
        } else {
            // 3 lines - use top alignment to show as much as possible
            format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_TOP;
        }
        
        DrawText(hdc, display_text, -1, &text_rc, format);
    }
    
    int hit_test(int mx, int my) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Check if click is on scrollbar
        if (mx >= rc.right - SCROLLBAR_WIDTH) {
            return -1;  // Scrollbar area
        }
        
        // Use same layout calculation as paint
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        
        // Adjust for custom scrollbar
        int drawing_width = rc.right - SCROLLBAR_WIDTH;
        
        // Use same centering as paint
        int total_width = cols * m_item_size + (cols + 1) * PADDING;
        int start_x = (drawing_width - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Check if click is within grid bounds
        if (mx < start_x || mx > start_x + total_width) {
            return -1;
        }
        
        // Adjust coordinates for scroll position
        my += m_scroll_pos;
        
        // Calculate which item was clicked
        int col = (mx - start_x) / (m_item_size + PADDING);
        int row = my / (item_total_height + PADDING);
        
        // Verify click is within an item (not in padding)
        int item_x = start_x + col * (m_item_size + PADDING) + PADDING;
        int item_y = row * (item_total_height + PADDING) + PADDING;
        
        if (mx >= item_x && mx < item_x + m_item_size &&
            my >= item_y && my < item_y + item_total_height) {
            int index = row * cols + col;
            if (index >= 0 && index < m_items.size()) {
                return index;
            }
        }
        
        return -1;
    }
    
    void handle_scrollbar_click(int mx, int my) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Calculate scroll metrics
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + PADDING) + PADDING;
        
        if (total_height <= rc.bottom) return;  // No scrolling needed
        
        // Calculate thumb position
        float visible_ratio = (float)rc.bottom / total_height;
        int thumb_height = max(30, (int)(rc.bottom * visible_ratio));
        float scroll_ratio = (float)m_scroll_pos / (total_height - rc.bottom);
        int thumb_y = (int)((rc.bottom - thumb_height) * scroll_ratio);
        
        // Check if click is on thumb
        if (my >= thumb_y && my < thumb_y + thumb_height) {
            // Start dragging
            m_scrollbar_dragging = true;
            m_scrollbar_drag_offset = my - thumb_y;
            SetCapture(m_hwnd);
        } else {
            // Page up/down
            if (my < thumb_y) {
                m_scroll_pos = max(0, m_scroll_pos - rc.bottom);
            } else {
                m_scroll_pos = min(total_height - rc.bottom, m_scroll_pos + rc.bottom);
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
        m_hwnd = m_hWnd;
        
        // Don't create Windows scrollbar - we'll draw our own
        
        // Create tooltip
        m_tooltip = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            m_hwnd,
            NULL,
            core_api::get_my_instance(),
            NULL
        );
        
        if (m_tooltip) {
            SetWindowPos(m_tooltip, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            
            TOOLINFO ti = {};
            ti.cbSize = sizeof(TOOLINFO);
            ti.uFlags = TTF_SUBCLASS | TTF_TRACK | TTF_ABSOLUTE;
            ti.hwnd = m_hwnd;
            ti.hinst = core_api::get_my_instance();
            ti.uId = 0;
            ti.lpszText = const_cast<LPTSTR>(TEXT(""));
            ti.rect = {0, 0, 0, 0};
            
            SendMessage(m_tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        }
        
        // Apply dark mode if available
        if (m_callback.is_valid() && m_callback->is_dark_mode()) {
            // Note: We're using custom scrollbar, so no need to theme Windows scrollbar
        }
        
        return 0;
    }
    
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
        if (m_tooltip) {
            DestroyWindow(m_tooltip);
            m_tooltip = NULL;
        }
        return 0;
    }
    
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&) {
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }
    
    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        paint(hdc);
        EndPaint(m_hwnd, &ps);
        return 0;
    }
    
    LRESULT OnVScroll(UINT, WPARAM wParam, LPARAM, BOOL&) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        
        int cols = m_config.columns;
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + PADDING) + PADDING;
        
        int old_pos = m_scroll_pos;
        
        switch (LOWORD(wParam)) {
            case SB_LINEUP:
                m_scroll_pos = max(0, m_scroll_pos - 20);
                break;
            case SB_LINEDOWN:
                m_scroll_pos = min(max(0, total_height - rc.bottom), m_scroll_pos + 20);
                break;
            case SB_PAGEUP:
                m_scroll_pos = max(0, m_scroll_pos - rc.bottom);
                break;
            case SB_PAGEDOWN:
                m_scroll_pos = min(max(0, total_height - rc.bottom), m_scroll_pos + rc.bottom);
                break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                m_scroll_pos = HIWORD(wParam);
                break;
        }
        
        if (m_scroll_pos != old_pos) {
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT OnMouseWheel(UINT, WPARAM wParam, LPARAM, BOOL&) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            // Ctrl+Wheel: Change columns
            if (delta > 0 && m_config.columns < MAX_COLUMNS) {
                m_config.columns++;
                m_config.save();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
            } else if (delta < 0 && m_config.columns > MIN_COLUMNS) {
                m_config.columns--;
                m_config.save();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        } else {
            // Normal wheel: Scroll
            RECT rc;
            GetClientRect(m_hwnd, &rc);
            
            calculate_layout();
            int text_height = calculate_text_height();
            int item_total_height = m_item_size + text_height;
            
            int cols = m_config.columns;
            int rows = (m_items.size() + cols - 1) / cols;
            int total_height = rows * (item_total_height + PADDING) + PADDING;
            
            int old_pos = m_scroll_pos;
            m_scroll_pos -= (delta / WHEEL_DELTA) * 60;
            m_scroll_pos = max(0, min(m_scroll_pos, max(0, total_height - rc.bottom)));
            
            if (m_scroll_pos != old_pos) {
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
        
        return 0;
    }
    
    LRESULT OnLButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Check if click is on scrollbar
        if (mx >= rc.right - SCROLLBAR_WIDTH) {
            handle_scrollbar_click(mx, my);
            return 0;
        }
        
        int index = hit_test(mx, my);
        
        if (index >= 0) {
            bool ctrl = (wParam & MK_CONTROL) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            
            if (ctrl) {
                // Toggle selection
                if (m_selected_indices.find(index) != m_selected_indices.end()) {
                    m_selected_indices.erase(index);
                } else {
                    m_selected_indices.insert(index);
                }
                m_last_selected = index;
            } else if (shift && m_last_selected >= 0) {
                // Range selection
                m_selected_indices.clear();
                int start = min(m_last_selected, index);
                int end = max(m_last_selected, index);
                for (int i = start; i <= end; i++) {
                    m_selected_indices.insert(i);
                }
            } else {
                // Single selection
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                m_last_selected = index;
            }
            
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // Clear selection
            m_selected_indices.clear();
            m_last_selected = -1;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        SetFocus(m_hwnd);
        return 0;
    }
    
    LRESULT OnMouseMove(UINT, WPARAM, LPARAM lParam, BOOL&) {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Handle scrollbar dragging
        if (m_scrollbar_dragging) {
            calculate_layout();
            int text_height = calculate_text_height();
            int item_total_height = m_item_size + text_height;
            int cols = m_config.columns;
            int rows = (m_items.size() + cols - 1) / cols;
            int total_height = rows * (item_total_height + PADDING) + PADDING;
            
            if (total_height > rc.bottom) {
                float visible_ratio = (float)rc.bottom / total_height;
                int thumb_height = max(30, (int)(rc.bottom * visible_ratio));
                
                int new_thumb_y = my - m_scrollbar_drag_offset;
                new_thumb_y = max(0, min(rc.bottom - thumb_height, new_thumb_y));
                
                float scroll_ratio = (float)new_thumb_y / (rc.bottom - thumb_height);
                m_scroll_pos = (int)(scroll_ratio * (total_height - rc.bottom));
                
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        // Check if mouse is over scrollbar
        bool was_hovered = m_scrollbar_hovered;
        m_scrollbar_hovered = (mx >= rc.right - SCROLLBAR_WIDTH);
        
        if (was_hovered != m_scrollbar_hovered) {
            RECT scrollbar_rc = { rc.right - SCROLLBAR_WIDTH, 0, rc.right, rc.bottom };
            InvalidateRect(m_hwnd, &scrollbar_rc, FALSE);
        }
        
        // Track hover for items
        int old_hover = m_hover_index;
        m_hover_index = hit_test(mx, my);
        
        if (old_hover != m_hover_index) {
            InvalidateRect(m_hwnd, NULL, FALSE);
            
            // Update tooltip
            if (!m_config.show_text && m_tooltip && m_hover_index >= 0 && m_hover_index < m_items.size()) {
                // Show tooltip only when text is hidden
                auto& item = m_items[m_hover_index];
                
                pfc::string8 tooltip_text;
                tooltip_text << item->display_name;
                if (m_config.show_track_count && item->tracks.get_count() > 0) {
                    tooltip_text << " (" << item->tracks.get_count() << " tracks)";
                }
                
                pfc::stringcvt::string_os_from_utf8 tooltip_os(tooltip_text.c_str());
                
                TOOLINFO ti = {};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = m_hwnd;
                ti.uId = 0;
                ti.lpszText = const_cast<LPTSTR>(tooltip_os.get_ptr());
                
                SendMessage(m_tooltip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
                SendMessage(m_tooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
                
                // Position tooltip below the item, centered
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                calculate_layout();
                int text_height = calculate_text_height();
                int item_total_height = m_item_size + text_height;
                int cols = m_config.columns;
                int drawing_width = rc.right - SCROLLBAR_WIDTH;
                int total_width = cols * m_item_size + (cols + 1) * PADDING;
                int start_x = (drawing_width - total_width) / 2;
                
                int col = m_hover_index % cols;
                int row = m_hover_index / cols;
                int item_x = start_x + col * (m_item_size + PADDING) + PADDING + m_item_size / 2;
                int item_y = row * (item_total_height + PADDING) + PADDING + item_total_height - m_scroll_pos + 5;
                
                POINT pt = {item_x, item_y};
                ClientToScreen(m_hwnd, &pt);
                SendMessage(m_tooltip, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
            } else if (m_tooltip) {
                SendMessage(m_tooltip, TTM_TRACKACTIVATE, FALSE, 0);
            }
        }
        
        // Track mouse for leave detection
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        TrackMouseEvent(&tme);
        
        return 0;
    }
    
    LRESULT OnMouseLeave(UINT, WPARAM, LPARAM, BOOL&) {
        if (m_scrollbar_dragging) {
            // Don't change hover state while dragging
            return 0;
        }
        
        m_hover_index = -1;
        m_scrollbar_hovered = false;
        
        if (m_tooltip) {
            SendMessage(m_tooltip, TTM_TRACKACTIVATE, FALSE, 0);
        }
        
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }
    
    LRESULT OnLButtonDblClk(UINT, WPARAM, LPARAM lParam, BOOL&) {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        
        int index = hit_test(mx, my);
        
        if (index >= 0 && index < m_items.size()) {
            auto& item = m_items[index];
            
            if (item->tracks.get_count() > 0) {
                static_api_ptr_t<playlist_manager> pm;
                auto pc = static_api_ptr_t<playback_control>();
                
                t_size playlist = pm->get_active_playlist();
                if (playlist == pfc_infinite) {
                    playlist = pm->create_playlist("Album Art Grid", ~0, ~0);
                    pm->set_active_playlist(playlist);
                }
                
                pm->playlist_clear(playlist);
                pm->playlist_add_items(playlist, item->tracks, bit_array_true());
                
                pc->start(playback_control::track_command_play);
            }
        }
        
        return 0;
    }
    
    LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL&) {
        if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+A: Select all
            m_selected_indices.clear();
            for (size_t i = 0; i < m_items.size(); i++) {
                m_selected_indices.insert(i);
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    void show_context_menu(int x, int y) {
        HMENU menu = CreatePopupMenu();
        
        // Play submenu
        if (!m_selected_indices.empty()) {
            AppendMenu(menu, MF_STRING, 1, TEXT("Play"));
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to Current Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        }
        
        // Grouping submenu
        HMENU groupMenu = CreatePopupMenu();
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_FOLDER ? MF_CHECKED : 0), 100, TEXT("By Folder"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_DIRECTORY ? MF_CHECKED : 0), 101, TEXT("By Directory"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_ALBUM ? MF_CHECKED : 0), 102, TEXT("By Album"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_ARTIST ? MF_CHECKED : 0), 103, TEXT("By Artist 🎤"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_ALBUM_ARTIST ? MF_CHECKED : 0), 104, TEXT("By Album Artist 🎤"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_ARTIST_ALBUM ? MF_CHECKED : 0), 105, TEXT("By Artist/Album 🎤"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_PERFORMER ? MF_CHECKED : 0), 106, TEXT("By Performer 🎤"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_COMPOSER ? MF_CHECKED : 0), 107, TEXT("By Composer 🎤"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_GENRE ? MF_CHECKED : 0), 108, TEXT("By Genre"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_YEAR ? MF_CHECKED : 0), 109, TEXT("By Year"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_LABEL ? MF_CHECKED : 0), 110, TEXT("By Label"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_RATING ? MF_CHECKED : 0), 111, TEXT("By Rating"));
        AppendMenu(groupMenu, MF_STRING | (m_config.grouping == GROUP_BY_COMMENT ? MF_CHECKED : 0), 112, TEXT("By Comment"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)groupMenu, TEXT("Grouping"));
        
        // Sorting submenu
        HMENU sortMenu = CreatePopupMenu();
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_NAME ? MF_CHECKED : 0), 200, TEXT("By Name"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_ARTIST ? MF_CHECKED : 0), 201, TEXT("By Artist"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_ALBUM ? MF_CHECKED : 0), 202, TEXT("By Album"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_YEAR ? MF_CHECKED : 0), 203, TEXT("By Year"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_GENRE ? MF_CHECKED : 0), 204, TEXT("By Genre"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_DATE_MODIFIED ? MF_CHECKED : 0), 205, TEXT("By Date Modified"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_TOTAL_SIZE ? MF_CHECKED : 0), 206, TEXT("By Total Size"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_TRACK_COUNT ? MF_CHECKED : 0), 207, TEXT("By Track Count"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_RATING ? MF_CHECKED : 0), 208, TEXT("By Rating"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_BY_PATH ? MF_CHECKED : 0), 209, TEXT("By Path"));
        AppendMenu(sortMenu, MF_STRING | (m_config.sort_mode == SORT_RANDOM_SHUFFLE ? MF_CHECKED : 0), 210, TEXT("Random/Shuffle"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)sortMenu, TEXT("Sorting"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        
        // Columns submenu
        HMENU colMenu = CreatePopupMenu();
        for (int i = MIN_COLUMNS; i <= MAX_COLUMNS; i++) {
            TCHAR buf[32];
            _stprintf_s(buf, TEXT("%d Columns"), i);
            AppendMenu(colMenu, MF_STRING | (m_config.columns == i ? MF_CHECKED : 0), 300 + i, buf);
        }
        AppendMenu(menu, MF_POPUP, (UINT_PTR)colMenu, TEXT("Columns"));
        
        // Text options submenu
        HMENU textMenu = CreatePopupMenu();
        AppendMenu(textMenu, MF_STRING | (!m_config.show_text ? MF_CHECKED : 0), 400, TEXT("Hide Labels"));
        AppendMenu(textMenu, MF_STRING | (m_config.show_text && m_config.text_lines == 1 ? MF_CHECKED : 0), 401, TEXT("1 Line"));
        AppendMenu(textMenu, MF_STRING | (m_config.show_text && m_config.text_lines == 2 ? MF_CHECKED : 0), 402, TEXT("2 Lines"));
        AppendMenu(textMenu, MF_STRING | (m_config.show_text && m_config.text_lines == 3 ? MF_CHECKED : 0), 403, TEXT("3 Lines"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)textMenu, TEXT("Text Display"));
        
        // Track count toggle
        AppendMenu(menu, MF_STRING | (m_config.show_track_count ? MF_CHECKED : 0), 500, TEXT("Show Track Count"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING, 600, TEXT("Refresh"));
        
        // Show menu
        POINT pt = {x, y};
        ClientToScreen(m_hwnd, &pt);
        
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, m_hwnd, NULL);
        
        DestroyMenu(menu);
        
        // Handle command
        switch (cmd) {
            case 1: // Play
                play_selected();
                break;
                
            case 2: // Add to playlist
                add_to_playlist();
                break;
                
            case 100: case 101: case 102: case 103: case 104: case 105:
            case 106: case 107: case 108: case 109: case 110: case 111: case 112:
                m_config.grouping = (GroupingMode)(cmd - 100);
                m_config.save();
                refresh_library();
                break;
                
            case 200: case 201: case 202: case 203: case 204:
            case 205: case 206: case 207: case 208: case 209: case 210:
                m_config.sort_mode = (SortMode)(cmd - 200);
                m_config.save();
                refresh_library();
                break;
                
            case 303: case 304: case 305: case 306: case 307: case 308: case 309: case 310:
                m_config.columns = cmd - 300;
                m_config.save();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
                
            case 400: // Hide text
                m_config.show_text = false;
                m_config.save();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
                
            case 401: case 402: case 403: // Text lines
                m_config.show_text = true;
                m_config.text_lines = cmd - 400;
                m_config.save();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
                
            case 500: // Toggle track count
                m_config.show_track_count = !m_config.show_track_count;
                m_config.save();
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
                
            case 600: // Refresh
                refresh_library();
                break;
        }
    }
    
    LRESULT OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
        // Check if we're in edit mode
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            // Pass through to the host
            bHandled = FALSE;
            return 0;
        }
        
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        // If from keyboard (x == -1, y == -1), use current selection
        if (x == -1 && y == -1) {
            RECT rc;
            GetClientRect(m_hwnd, &rc);
            x = rc.right / 2;
            y = rc.bottom / 2;
        } else {
            // Convert screen coordinates to client
            POINT pt = {x, y};
            ScreenToClient(m_hwnd, &pt);
            x = pt.x;
            y = pt.y;
        }
        
        show_context_menu(x, y);
        return 0;
    }
    
    void play_selected() {
        if (m_selected_indices.empty()) return;
        
        metadb_handle_list items;
        for (int index : m_selected_indices) {
            if (index >= 0 && index < m_items.size()) {
                items.add_items(m_items[index]->tracks);
            }
        }
        
        if (items.get_count() > 0) {
            static_api_ptr_t<playlist_manager> pm;
            auto pc = static_api_ptr_t<playback_control>();
            
            t_size playlist = pm->get_active_playlist();
            if (playlist == pfc_infinite) {
                playlist = pm->create_playlist("Album Art Grid", ~0, ~0);
                pm->set_active_playlist(playlist);
            }
            
            pm->playlist_clear(playlist);
            pm->playlist_add_items(playlist, items, bit_array_true());
            
            pc->start(playback_control::track_command_play);
        }
    }
    
    void add_to_playlist() {
        if (m_selected_indices.empty()) return;
        
        metadb_handle_list items;
        for (int index : m_selected_indices) {
            if (index >= 0 && index < m_items.size()) {
                items.add_items(m_items[index]->tracks);
            }
        }
        
        if (items.get_count() > 0) {
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            if (playlist != pfc_infinite) {
                pm->playlist_add_items(playlist, items, bit_array_true());
            }
        }
    }
};

// UI Element
class album_grid_ui_element : public ui_element_instance {
    CWindow m_wnd;
    ui_element_instance_callback::ptr m_callback;
    AlbumArtGridWindow m_grid_window;
    
public:
    album_grid_ui_element(HWND parent, ui_element_config::ptr, ui_element_instance_callback::ptr callback) 
        : m_callback(callback) {
        
        m_grid_window.initialize(callback);
        m_wnd = m_grid_window.Create(parent, NULL, NULL, 
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL,
            WS_EX_CONTROLPARENT);
    }
    
    ~album_grid_ui_element() {
        if (m_wnd.IsWindow()) {
            m_wnd.DestroyWindow();
        }
    }
    
    HWND get_wnd() override { return m_wnd; }
    
    void set_configuration(ui_element_config::ptr) override {}
    ui_element_config::ptr get_configuration() override { return ui_element_config::g_create_empty(); }
    
    static GUID g_get_guid() { return guid_ui_element; }
    static void g_get_name(pfc::string_base& out) { out = "Album Art Grid"; }
    static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(); }
    static const char* g_get_description() { return "Displays album art in a customizable grid view"; }
    static GUID g_get_subclass() { return ui_element_subclass_media_library_viewers; }
    
    void notify(const GUID& what, t_size, const void*, t_size) override {
        if (what == ui_element_notify_colors_changed || what == ui_element_notify_font_changed) {
            m_wnd.InvalidateRect(NULL, FALSE);
        }
    }
};

class album_grid_ui_element_factory : public ui_element_impl<album_grid_ui_element> {};
static service_factory_single_t<album_grid_ui_element_factory> g_album_grid_ui_element_factory;

// Library callback
class library_callback_impl : public library_callback {
public:
    void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr>&) override {
        refresh_all();
    }
    
    void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr>&) override {
        refresh_all();
    }
    
    void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr>&) override {
        refresh_all();
    }
    
private:
    void refresh_all() {
        // Find and refresh all grid windows
        EnumThreadWindows(GetCurrentThreadId(), [](HWND hwnd, LPARAM) -> BOOL {
            TCHAR className[256];
            if (GetClassName(hwnd, className, 256)) {
                if (_tcscmp(className, TEXT("{98765432-1234-5678-9012-345678901234}")) == 0) {
                    AlbumArtGridWindow* window = reinterpret_cast<AlbumArtGridWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                    if (window) {
                        window->refresh_library();
                    }
                }
            }
            return TRUE;
        }, 0);
    }
};

static service_factory_single_t<library_callback_impl> g_library_callback_factory;

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "9.6.0",
    "Album Art Grid Component for foobar2000\n"
    "Displays album art in a customizable grid view\n\n"
    "Version 9.6 Features:\n"
    "- Track count badge when labels are hidden\n"
    "- Custom themed scrollbar using foobar2000 colors\n"
    "- Artist image support (v9.5)\n"
    "- 13 grouping modes\n"
    "- 11 sorting options\n"
    "- Auto-fill grid mode\n"
    "- 3-10 columns (Ctrl+Mouse Wheel)\n"
    "- High-quality image rendering\n\n"
    "Created with Anthropic's Claude AI"
);

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    return TRUE;
}

// Required foobar2000 SDK exports
extern "C" {
    FOOBAR2000_API foobar2000_client* foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE hIns) {
        core_api::ptr = p_api;
        
        static class foobar_client : public foobar2000_client {
        public:
            t_uint32 get_version() override { return FOOBAR2000_CLIENT_VERSION; }
            void get_config(foobar2000_config* p_out) override {
                p_out->bogo = 0;
                p_out->ui_element_api_version = 1;
                p_out->ui_element_v2 = false;
            }
        } g_client;
        
        return &g_client;
    }
}