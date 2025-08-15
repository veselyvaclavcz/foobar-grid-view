// Album Art Grid Component v9.5 - with Artist Image Support
// Added: Display artist images when grouping by artist-related modes

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

#define FOOBAR2000_TARGET_VERSION 82
#include "../../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "../../SDK-2025-03-07/foobar2000/helpers/helpers.h"

// Component validation
#ifdef _DEBUG
#define COMPONENT_NAME "foo_albumart_grid_v95_debug"
#else
#define COMPONENT_NAME "foo_albumart_grid_v95"
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
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()
    
    AlbumArtGridWindow() : m_scroll_pos(0), m_item_size(DEFAULT_ITEM_SIZE), 
                           m_hover_index(-1), m_tracking(false), m_last_selected(-1),
                           m_tooltip(NULL), m_tooltip_index(-1) {
        m_config.load();
    }
    
    void set_callback(ui_element_instance_callback::ptr callback) {
        m_callback = callback;
    }
    
    void refresh_contents(const pfc::list_base_const_t<metadb_handle_ptr>& tracks) {
        m_items.clear();
        m_selected_indices.clear();
        
        if (tracks.get_count() == 0) {
            InvalidateRect(m_hwnd, NULL, TRUE);
            return;
        }
        
        // Group tracks
        std::map<pfc::string8, std::shared_ptr<GridItem>> groups;
        
        for (t_size i = 0; i < tracks.get_count(); i++) {
            auto track = tracks[i];
            pfc::string8 group_key;
            pfc::string8 display_name;
            
            switch (m_config.grouping) {
                case GROUP_BY_FOLDER: {
                    const char* path = track->get_path();
                    const char* filename = strrchr(path, '\\');
                    if (filename) {
                        group_key.set_string(path, filename - path);
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
                        if (album && *album) {
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
                        if (artist && *artist) {
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
                        if (!album_artist || !*album_artist) {
                            album_artist = info.meta_get("artist", 0);
                        }
                        if (album_artist && *album_artist) {
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
                        const char* artist = info.meta_get("album artist", 0);
                        if (!artist || !*artist) artist = info.meta_get("artist", 0);
                        const char* album = info.meta_get("album", 0);
                        
                        if (artist && *artist) {
                            group_key = artist;
                            if (album && *album) {
                                group_key += " - ";
                                group_key += album;
                            }
                        } else if (album && *album) {
                            group_key = album;
                        } else {
                            group_key = "Unknown";
                        }
                        display_name = group_key;
                    }
                    break;
                }
                
                case GROUP_BY_PERFORMER: {
                    file_info_impl info;
                    if (track->get_info(info)) {
                        const char* performer = info.meta_get("performer", 0);
                        if (!performer || !*performer) {
                            performer = info.meta_get("artist", 0);
                        }
                        if (performer && *performer) {
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
                        if (composer && *composer) {
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
                        if (genre && *genre) {
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
                        const char* date = info.meta_get("date", 0);
                        if (date && *date) {
                            // Extract year from date
                            pfc::string8 year;
                            year.set_string(date, 4);
                            group_key = year;
                            display_name = year;
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
                        if (!label || !*label) label = info.meta_get("publisher", 0);
                        if (label && *label) {
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
                        if (rating && *rating) {
                            int stars = atoi(rating);
                            if (stars > 0) {
                                group_key.clear();
                                for (int j = 0; j < stars && j < 5; j++) {
                                    group_key += "★";
                                }
                                display_name = group_key;
                            } else {
                                group_key = "Unrated";
                                display_name = "Unrated";
                            }
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
                        if (comment && *comment) {
                            // Use first 50 chars of comment for grouping
                            pfc::string8 truncated;
                            truncated.set_string(comment, min(strlen(comment), (size_t)50));
                            group_key = truncated;
                            display_name = truncated;
                        } else {
                            group_key = "No Comment";
                            display_name = "No Comment";
                        }
                    }
                    break;
                }
            }
            
            auto it = groups.find(group_key);
            if (it == groups.end()) {
                auto item = std::make_shared<GridItem>();
                item->key = group_key;
                item->display_name = display_name;
                item->tracks.add_item(track);
                
                // Generate sort key based on first track
                generate_sort_key(item.get(), track);
                
                groups[group_key] = item;
            } else {
                it->second->tracks.add_item(track);
            }
        }
        
        // Convert to vector for sorting
        m_items.clear();
        for (auto& pair : groups) {
            m_items.push_back(pair.second);
        }
        
        // Sort items
        sort_items();
        
        // Start loading album art
        if (!m_items.empty()) {
            load_album_art();
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
    int m_item_size;
    int m_hover_index;
    bool m_tracking;
    HWND m_tooltip;
    int m_tooltip_index;
    
    void generate_sort_key(GridItem* item, metadb_handle_ptr track) {
        file_info_impl info;
        track->get_info(info);
        
        switch (m_config.sort_mode) {
            case SORT_BY_NAME:
                item->sort_key = item->display_name;
                break;
            
            case SORT_BY_ARTIST: {
                const char* artist = info.meta_get("artist", 0);
                item->sort_key = artist ? artist : "ZZZ";
                break;
            }
            
            case SORT_BY_ALBUM: {
                const char* album = info.meta_get("album", 0);
                item->sort_key = album ? album : "ZZZ";
                break;
            }
            
            case SORT_BY_YEAR: {
                const char* date = info.meta_get("date", 0);
                item->sort_key = date ? date : "9999";
                break;
            }
            
            case SORT_BY_GENRE: {
                const char* genre = info.meta_get("genre", 0);
                item->sort_key = genre ? genre : "ZZZ";
                break;
            }
            
            case SORT_BY_DATE_MODIFIED: {
                t_filetimestamp ts = track->get_filetimestamp();
                char buffer[32];
                sprintf_s(buffer, "%llu", ts);
                item->sort_key = buffer;
                break;
            }
            
            case SORT_BY_TOTAL_SIZE: {
                t_uint64 total_size = 0;
                for (t_size i = 0; i < item->tracks.get_count(); i++) {
                    total_size += item->tracks[i]->get_filesize();
                }
                char buffer[32];
                sprintf_s(buffer, "%020llu", total_size);
                item->sort_key = buffer;
                break;
            }
            
            case SORT_BY_TRACK_COUNT: {
                char buffer[32];
                sprintf_s(buffer, "%08u", (unsigned)item->tracks.get_count());
                item->sort_key = buffer;
                break;
            }
            
            case SORT_BY_RATING: {
                const char* rating = info.meta_get("rating", 0);
                if (rating && *rating) {
                    int stars = atoi(rating);
                    char buffer[8];
                    sprintf_s(buffer, "%d", 5 - stars);  // Reverse for descending
                    item->sort_key = buffer;
                } else {
                    item->sort_key = "9";
                }
                break;
            }
            
            case SORT_BY_PATH:
                item->sort_key = track->get_path();
                break;
            
            case SORT_RANDOM_SHUFFLE:
                // Will be handled differently
                break;
        }
    }
    
    void sort_items() {
        if (m_config.sort_mode == SORT_RANDOM_SHUFFLE) {
            // Random shuffle
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(m_items.begin(), m_items.end(), gen);
        } else {
            // Sort by sort_key
            std::sort(m_items.begin(), m_items.end(),
                [](const std::shared_ptr<GridItem>& a, const std::shared_ptr<GridItem>& b) {
                    return pfc::stricmp_ascii(a->sort_key.c_str(), b->sort_key.c_str()) < 0;
                }
            );
        }
    }
    
    void load_album_art() {
        static_api_ptr_t<album_art_manager_v2> art_api;
        abort_callback_dummy abort;
        
        for (auto& item : m_items) {
            if (item->tracks.get_count() > 0) {
                // Use the new function with fallback support
                item->artwork = get_album_art_with_fallback(
                    art_api,
                    item->tracks[0],
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
    }
    
    Gdiplus::Bitmap* create_thumbnail(album_art_data_ptr data, int size) {
        if (!data.is_valid()) return nullptr;
        
        IStream* stream = SHCreateMemStream((const BYTE*)data->get_ptr(), (UINT)data->get_size());
        if (!stream) return nullptr;
        
        Gdiplus::Bitmap* original = Gdiplus::Bitmap::FromStream(stream);
        stream->Release();
        
        if (!original || original->GetLastStatus() != Gdiplus::Ok) {
            if (original) delete original;
            return nullptr;
        }
        
        // Create thumbnail with proper aspect ratio
        int orig_width = original->GetWidth();
        int orig_height = original->GetHeight();
        
        if (orig_width <= 0 || orig_height <= 0) {
            delete original;
            return nullptr;
        }
        
        float scale = (float)size / max(orig_width, orig_height);
        int thumb_width = (int)(orig_width * scale);
        int thumb_height = (int)(orig_height * scale);
        
        Gdiplus::Bitmap* thumbnail = new Gdiplus::Bitmap(size, size, PixelFormat32bppARGB);
        Gdiplus::Graphics graphics(thumbnail);
        
        // High quality settings
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        
        // Clear with transparent
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
        
        // Center the image
        int x = (size - thumb_width) / 2;
        int y = (size - thumb_height) / 2;
        
        graphics.DrawImage(original, x, y, thumb_width, thumb_height);
        
        delete original;
        return thumbnail;
    }
    
    void calculate_layout() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int width = rc.right;
        
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
        
        // Get font from foobar2000 preferences
        HFONT hFont = m_callback->query_font_ex(ui_font_default);
        if (!hFont) {
            return m_config.text_lines * 14;  // Fallback
        }
        
        HDC hdc = GetDC(m_hwnd);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        
        SelectObject(hdc, hOldFont);
        ReleaseDC(m_hwnd, hdc);
        
        int line_height = tm.tmHeight + tm.tmExternalLeading;
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
        
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        si.nMin = 0;
        si.nMax = total_height - 1;
        si.nPage = rc.bottom;
        si.nPos = m_scroll_pos;
        
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
        
        // Adjust scroll position if needed
        if (m_scroll_pos > max(0, total_height - (int)rc.bottom)) {
            m_scroll_pos = max(0, total_height - rc.bottom);
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
        }
    }
    
    void paint(HDC hdc) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Create double buffer
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        
        // Fill background
        bool dark_mode = m_callback.is_valid() ? m_callback->is_dark_mode() : false;
        HBRUSH hbrBg = CreateSolidBrush(dark_mode ? RGB(30, 30, 30) : RGB(255, 255, 255));
        FillRect(hdcMem, &rc, hbrBg);
        DeleteObject(hbrBg);
        
        // Draw edit mode indicator if needed
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 140, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            
            Rectangle(hdcMem, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
            
            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hPen);
        }
        
        // Setup GDI+
        Gdiplus::Graphics graphics(hdcMem);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        
        // Calculate layout
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        
        // Center the grid horizontally
        int total_width = cols * m_item_size + (cols + 1) * PADDING;
        int start_x = (rc.right - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Calculate visible range
        int first_visible_row = m_scroll_pos / (item_total_height + PADDING);
        int last_visible_row = (m_scroll_pos + rc.bottom) / (item_total_height + PADDING) + 1;
        
        int first_visible = first_visible_row * cols;
        int last_visible = min((int)m_items.size(), (last_visible_row + 1) * cols);
        
        // Get font for text
        HFONT hFont = m_callback.is_valid() ? m_callback->query_font_ex(ui_font_default) : NULL;
        if (hFont) {
            SelectObject(hdcMem, hFont);
        }
        
        // Draw items
        for (int i = first_visible; i < last_visible; i++) {
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
            
            // Draw text
            if (m_config.show_text && text_height > 0) {
                draw_item_text(hdcMem, item.get(), x, y + m_item_size, m_item_size, text_height, dark_mode);
            }
        }
        
        // Blit to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
        
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
        
        // Use same layout calculation as paint
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        
        // Use same centering as paint
        int total_width = cols * m_item_size + (cols + 1) * PADDING;
        int start_x = (rc.right - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Check if click is within grid bounds
        if (mx < start_x + PADDING || mx >= start_x + total_width - PADDING) {
            return -1;
        }
        
        // Calculate column
        int rel_x = mx - start_x - PADDING;
        int col = rel_x / (m_item_size + PADDING);
        
        // Check if click is in padding between columns
        int col_start = col * (m_item_size + PADDING);
        if (rel_x < col_start || rel_x >= col_start + m_item_size) {
            return -1;
        }
        
        if (col >= cols) {
            return -1;
        }
        
        // Calculate row
        int adjusted_y = my + m_scroll_pos - PADDING;
        if (adjusted_y < 0) {
            return -1;
        }
        
        int row = adjusted_y / (item_total_height + PADDING);
        
        // Check if click is in padding between rows
        int row_start = row * (item_total_height + PADDING);
        if (adjusted_y < row_start || adjusted_y >= row_start + item_total_height) {
            return -1;
        }
        
        int index = row * cols + col;
        
        if (index >= 0 && index < (int)m_items.size()) {
            return index;
        }
        
        return -1;
    }
    
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
        m_hwnd = m_hWnd;
        
        // Enable scrollbar
        SetScrollRange(m_hwnd, SB_VERT, 0, 0, FALSE);
        
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
            ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
            ti.hwnd = m_hwnd;
            ti.uId = 0;
            ti.lpszText = const_cast<LPTSTR>(TEXT(""));
            
            SendMessage(m_tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
            SendMessage(m_tooltip, TTM_SETMAXTIPWIDTH, 0, 300);
        }
        
        // Apply dark mode if available
        if (m_callback.is_valid() && m_callback->is_dark_mode()) {
            CCoreDarkModeHooks::SetDarkMode(m_hwnd, true);
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
                m_scroll_pos -= 20;
                break;
            case SB_LINEDOWN:
                m_scroll_pos += 20;
                break;
            case SB_PAGEUP:
                m_scroll_pos -= rc.bottom;
                break;
            case SB_PAGEDOWN:
                m_scroll_pos += rc.bottom;
                break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                m_scroll_pos = HIWORD(wParam);
                break;
        }
        
        m_scroll_pos = max(0, min(m_scroll_pos, max(0, total_height - rc.bottom)));
        
        if (m_scroll_pos != old_pos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT OnMouseWheel(UINT, WPARAM wParam, LPARAM, BOOL&) {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            // Ctrl+Wheel: Adjust columns
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
                SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
        
        return 0;
    }
    
    LRESULT OnMouseMove(UINT, WPARAM, LPARAM lParam, BOOL&) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        if (!m_tracking) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            TrackMouseEvent(&tme);
            m_tracking = true;
        }
        
        int index = hit_test(x, y);
        if (index != m_hover_index) {
            m_hover_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
            
            // Update tooltip - only show if text display is disabled
            if (m_tooltip && !m_config.show_text && index >= 0 && index < (int)m_items.size()) {
                // Show tooltip with full text only when labels are hidden
                auto& item = m_items[index];
                pfc::string8 tooltip_text = item->display_name;
                if (m_config.show_track_count && item->tracks.get_count() > 0) {
                    tooltip_text << " (" << item->tracks.get_count() << " tracks)";
                }
                
                // Add indicator if showing artist image
                if (should_use_artist_image(m_config.grouping) && item->artwork.is_valid()) {
                    tooltip_text << " [Artist Image]";
                }
                
                pfc::stringcvt::string_os_from_utf8 tooltip_w(tooltip_text.c_str());
                
                TOOLINFO ti = {};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = m_hwnd;
                ti.uId = 0;
                ti.lpszText = const_cast<LPTSTR>(tooltip_w.get_ptr());
                SendMessage(m_tooltip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
                
                // Position tooltip below the item, centered
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                calculate_layout();
                int text_height = calculate_text_height();
                int item_total_height = m_item_size + text_height;
                int cols = m_config.columns;
                int total_width = cols * m_item_size + (cols + 1) * PADDING;
                int start_x = (rc.right - total_width) / 2;
                
                int col = index % cols;
                int row = index / cols;
                int item_x = start_x + col * (m_item_size + PADDING) + PADDING + m_item_size / 2;
                int item_y = row * (item_total_height + PADDING) + PADDING + item_total_height - m_scroll_pos + 5;
                
                POINT pt = {item_x, item_y};
                ClientToScreen(m_hwnd, &pt);
                SendMessage(m_tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
                SendMessage(m_tooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
                
                m_tooltip_index = index;
            } else if (m_tooltip) {
                // Hide tooltip
                TOOLINFO ti = {};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = m_hwnd;
                ti.uId = 0;
                SendMessage(m_tooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
                m_tooltip_index = -1;
            }
        }
        
        return 0;
    }
    
    LRESULT OnMouseLeave(UINT, WPARAM, LPARAM, BOOL&) {
        m_tracking = false;
        if (m_hover_index != -1) {
            m_hover_index = -1;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        // Hide tooltip
        if (m_tooltip) {
            TOOLINFO ti = {};
            ti.cbSize = sizeof(TOOLINFO);
            ti.hwnd = m_hwnd;
            ti.uId = 0;
            SendMessage(m_tooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
            m_tooltip_index = -1;
        }
        
        return 0;
    }
    
    LRESULT OnLButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        // In edit mode, don't handle item selection - let the host handle element selection
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            SetFocus(m_hwnd);
            return 0;
        }
        
        int index = hit_test(x, y);
        if (index >= 0) {
            if (wParam & MK_CONTROL) {
                // Ctrl+Click: Toggle selection
                if (m_selected_indices.find(index) != m_selected_indices.end()) {
                    m_selected_indices.erase(index);
                } else {
                    m_selected_indices.insert(index);
                }
                m_last_selected = index;
            } else if (wParam & MK_SHIFT && m_last_selected >= 0) {
                // Shift+Click: Range selection
                m_selected_indices.clear();
                int start = min(m_last_selected, index);
                int end = max(m_last_selected, index);
                for (int i = start; i <= end; i++) {
                    m_selected_indices.insert(i);
                }
            } else {
                // Normal click: Single selection
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                m_last_selected = index;
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // Click on empty space: Clear selection
            m_selected_indices.clear();
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        SetFocus(m_hwnd);
        return 0;
    }
    
    LRESULT OnLButtonDblClk(UINT, WPARAM, LPARAM lParam, BOOL&) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        // In edit mode, don't handle double-clicks
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            return 0;
        }
        
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_items.size()) {
            // Play the item
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            if (playlist != pfc::infinite_size) {
                pm->playlist_clear(playlist);
                
                // Add all selected items or just the double-clicked one
                if (!m_selected_indices.empty() && 
                    m_selected_indices.find(index) != m_selected_indices.end()) {
                    // Play all selected items
                    for (int sel_index : m_selected_indices) {
                        if (sel_index < (int)m_items.size()) {
                            pm->playlist_add_items(playlist, m_items[sel_index]->tracks, bit_array_false());
                        }
                    }
                } else {
                    // Play only the double-clicked item
                    pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                }
                
                pm->set_playing_playlist(playlist);
                static_api_ptr_t<playback_control> pc;
                pc->play_start();
            }
        }
        return 0;
    }
    
    LRESULT OnRButtonDown(UINT, WPARAM, LPARAM lParam, BOOL&) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        // In edit mode, pass through to default handler for element context menu
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            SetFocus(m_hwnd);
            return DefWindowProc(WM_RBUTTONDOWN, 0, lParam);
        }
        
        int index = hit_test(x, y);
        if (index >= 0) {
            // Select item if not already selected
            if (m_selected_indices.find(index) == m_selected_indices.end()) {
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                m_last_selected = index;
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
        
        SetFocus(m_hwnd);
        return 0;
    }
    
    LRESULT OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&) {
        // In edit mode, pass through to default handler
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            return DefWindowProc(WM_CONTEXTMENU, 0, lParam);
        }
        
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        // Convert to client coordinates for hit test
        POINT pt = {x, y};
        ScreenToClient(m_hwnd, &pt);
        
        HMENU menu = CreatePopupMenu();
        
        // Play options
        if (!m_selected_indices.empty()) {
            AppendMenu(menu, MF_STRING, 1, TEXT("Play"));
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to current playlist"));
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
            wsprintf(buf, TEXT("%d columns"), i);
            AppendMenu(colMenu, MF_STRING | (m_config.columns == i ? MF_CHECKED : 0), 300 + i, buf);
        }
        AppendMenu(menu, MF_POPUP, (UINT_PTR)colMenu, TEXT("Columns"));
        
        // Text lines submenu
        HMENU textMenu = CreatePopupMenu();
        AppendMenu(textMenu, MF_STRING | (!m_config.show_text ? MF_CHECKED : 0), 400, TEXT("Hide text"));
        AppendMenu(textMenu, MF_STRING | (m_config.show_text && m_config.text_lines == 1 ? MF_CHECKED : 0), 401, TEXT("1 line"));
        AppendMenu(textMenu, MF_STRING | (m_config.show_text && m_config.text_lines == 2 ? MF_CHECKED : 0), 402, TEXT("2 lines"));
        AppendMenu(textMenu, MF_STRING | (m_config.show_text && m_config.text_lines == 3 ? MF_CHECKED : 0), 403, TEXT("3 lines"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)textMenu, TEXT("Text display"));
        
        // Track count option
        AppendMenu(menu, MF_STRING | (m_config.show_track_count ? MF_CHECKED : 0), 500, TEXT("Show track count"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING, 999, TEXT("Album Art Grid v9.5"));
        
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, m_hwnd, NULL);
        
        DestroyMenu(groupMenu);
        DestroyMenu(sortMenu);
        DestroyMenu(colMenu);
        DestroyMenu(textMenu);
        DestroyMenu(menu);
        
        bool needs_refresh = false;
        bool needs_resort = false;
        
        switch (cmd) {
            case 1: // Play
                if (!m_selected_indices.empty()) {
                    static_api_ptr_t<playlist_manager> pm;
                    t_size playlist = pm->get_active_playlist();
                    if (playlist != pfc::infinite_size) {
                        pm->playlist_clear(playlist);
                        for (int index : m_selected_indices) {
                            if (index < (int)m_items.size()) {
                                pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                            }
                        }
                        pm->set_playing_playlist(playlist);
                        static_api_ptr_t<playback_control> pc;
                        pc->play_start();
                    }
                }
                break;
                
            case 2: // Add to playlist
                if (!m_selected_indices.empty()) {
                    static_api_ptr_t<playlist_manager> pm;
                    t_size playlist = pm->get_active_playlist();
                    if (playlist != pfc::infinite_size) {
                        for (int index : m_selected_indices) {
                            if (index < (int)m_items.size()) {
                                pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                            }
                        }
                    }
                }
                break;
                
            case 100: case 101: case 102: case 103: case 104: case 105: case 106: 
            case 107: case 108: case 109: case 110: case 111: case 112:
                m_config.grouping = (GroupingMode)(cmd - 100);
                m_config.save();
                needs_refresh = true;
                break;
                
            case 200: case 201: case 202: case 203: case 204: case 205: case 206:
            case 207: case 208: case 209: case 210:
                m_config.sort_mode = (SortMode)(cmd - 200);
                m_config.save();
                needs_resort = true;
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
        }
        
        if (needs_refresh && m_callback.is_valid()) {
            metadb_handle_list tracks;
            m_callback->query_tracks(tracks);
            refresh_contents(tracks);
        } else if (needs_resort) {
            // Regenerate sort keys and resort
            for (auto& item : m_items) {
                if (item->tracks.get_count() > 0) {
                    generate_sort_key(item.get(), item->tracks[0]);
                }
            }
            sort_items();
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL&) {
        switch (wParam) {
            case VK_UP:
            case VK_DOWN:
            case VK_LEFT:
            case VK_RIGHT:
            case VK_HOME:
            case VK_END:
            case VK_PRIOR:
            case VK_NEXT:
                // Handle navigation keys
                // TODO: Implement keyboard navigation
                break;
        }
        return 0;
    }
    
    LRESULT OnFocus(UINT msg, WPARAM, LPARAM, BOOL&) {
        if (m_callback.is_valid()) {
            if (msg == WM_SETFOCUS) {
                m_callback->on_focus(true);
            } else {
                m_callback->on_focus(false);
            }
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }
};

// UI Element
class AlbumArtGridElement : public ui_element_instance {
private:
    AlbumArtGridWindow m_window;
    ui_element_instance_callback::ptr m_callback;
    
public:
    AlbumArtGridElement(HWND parent, ui_element_config::ptr config, ui_element_instance_callback::ptr callback) 
        : m_callback(callback) {
        m_window.set_callback(callback);
        m_window.Create(parent, NULL, TEXT("Album Art Grid"), 
                       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                       WS_EX_CONTROLPARENT);
    }
    
    HWND get_wnd() { return m_window.m_hWnd; }
    
    void set_configuration(ui_element_config::ptr config) {
        // Configuration is handled through cfg_vars
    }
    
    ui_element_config::ptr get_configuration() {
        return ui_element_config::g_create_empty(guid_ui_element);
    }
    
    static GUID g_get_guid() {
        return guid_ui_element;
    }
    
    static void g_get_name(pfc::string_base& out) {
        out = "Album Art Grid";
    }
    
    static ui_element_config::ptr g_get_default_configuration() {
        return ui_element_config::g_create_empty(guid_ui_element);
    }
    
    static const char* g_get_description() {
        return "Displays album art in a customizable grid layout with artist image support.";
    }
    
    static GUID g_get_subclass() {
        return ui_element_subclass_media_library_viewers;
    }
    
    void notify(const GUID& what, t_size param1, const void* param2, t_size param2size) {
        if (what == ui_element_notify_colors_changed || what == ui_element_notify_font_changed) {
            m_window.InvalidateRect(NULL, TRUE);
        } else if (what == ui_element_notify_playlist_tracks_selection_change) {
            metadb_handle_list tracks;
            m_callback->query_tracks(tracks);
            m_window.refresh_contents(tracks);
        }
    }
};

// Factory
class AlbumArtGridFactory : public ui_element_v2 {
public:
    GUID get_guid() { return AlbumArtGridElement::g_get_guid(); }
    
    void get_name(pfc::string_base& out) { 
        AlbumArtGridElement::g_get_name(out); 
    }
    
    ui_element_config::ptr get_default_configuration() { 
        return AlbumArtGridElement::g_get_default_configuration(); 
    }
    
    ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) {
        return new service_impl_single_t<AlbumArtGridElement>(parent, cfg, callback);
    }
    
    void get_description(pfc::string_base& out) { 
        out = AlbumArtGridElement::g_get_description(); 
    }
    
    GUID get_subclass() { 
        return AlbumArtGridElement::g_get_subclass(); 
    }
    
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) {
        return NULL;
    }
    
    bool is_user_addable() { 
        return true; 
    }
    
    t_uint32 get_flags() {
        return KFlagSupportsMergeUI;
    }
    
    bool get_element_group(pfc::string_base& out) {
        out = "Media Library";
        return true;
    }
};

static ui_element_factory_t<AlbumArtGridFactory> g_ui_element_factory;

// GDI+ management
class GdiPlusInitializer {
    ULONG_PTR m_token;
    
public:
    GdiPlusInitializer() {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&m_token, &input, NULL);
    }
    
    ~GdiPlusInitializer() {
        Gdiplus::GdiplusShutdown(m_token);
    }
};

static service_factory_single_t<GdiPlusInitializer> g_gdi_plus_init;

// Component declaration
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "9.5.0",
    "Album Art Grid Component for foobar2000\n"
    "Displays album art in a customizable grid layout.\n"
    "Now with artist image support when grouping by artist!\n\n"
    "Features:\n"
    "- Artist images displayed for artist groupings\n" 
    "- Automatic fallback to album art when artist image unavailable\n"
    "- 13 grouping modes with smart image selection\n"
    "- 11 sorting options\n"
    "- Auto-fill mode\n"
    "- Ctrl+Mouse Wheel column control\n"
    "- High-quality image rendering\n\n"
    "🎤 = Shows artist images when available\n\n"
    "Created with assistance from Anthropic's Claude AI"
);