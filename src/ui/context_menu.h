#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>

namespace albumart_grid {

// Context menu for grid items and background
class ContextMenu {
public:
    enum MenuCommand {
        // Playback
        CMD_PLAY = 1000,
        CMD_ADD_TO_QUEUE,
        CMD_ADD_TO_PLAYLIST,
        
        // View
        CMD_VIEW_LIBRARY = 2000,
        CMD_VIEW_PLAYLIST,
        CMD_VIEW_SELECTION,
        
        // Grouping
        CMD_GROUP_ALBUM = 3000,
        CMD_GROUP_ARTIST,
        CMD_GROUP_ALBUMARTIST,
        CMD_GROUP_GENRE,
        CMD_GROUP_YEAR,
        CMD_GROUP_DIRECTORY,
        CMD_GROUP_ARTIST_ALBUM,
        CMD_GROUP_GENRE_ALBUM,
        CMD_GROUP_LABEL,
        CMD_GROUP_COMPOSER,
        CMD_GROUP_PERFORMER,
        CMD_GROUP_ALBUM_FLAT,
        CMD_GROUP_ARTIST_FLAT,
        
        // Sorting  
        CMD_SORT_NAME = 4000,
        CMD_SORT_ARTIST,
        CMD_SORT_ALBUM,
        CMD_SORT_YEAR,
        CMD_SORT_GENRE,
        CMD_SORT_TRACKCOUNT,
        CMD_SORT_DURATION,
        CMD_SORT_PLAYCOUNT,
        CMD_SORT_RATING,
        CMD_SORT_DATE_ADDED,
        CMD_SORT_RANDOM,
        
        // Other
        CMD_REFRESH = 5000,
        CMD_SETTINGS,
        CMD_SELECT_ALL,
        CMD_SELECT_NONE,
        CMD_INVERT_SELECTION,
        CMD_SEARCH
    };
    
    using CommandCallback = std::function<void(MenuCommand cmd)>;
    
    ContextMenu();
    ~ContextMenu();
    
    // Set command callback
    void set_command_callback(CommandCallback cb) { m_command_cb = cb; }
    
    // Show context menu
    void show(HWND hwnd, int x, int y, bool has_selection);
    
    // Show background context menu (no selection)
    void show_background(HWND hwnd, int x, int y);
    
    // Get current grouping/sorting mode
    MenuCommand get_current_grouping() const { return m_current_grouping; }
    MenuCommand get_current_sorting() const { return m_current_sorting; }
    
    // Set current grouping/sorting mode
    void set_current_grouping(MenuCommand cmd);
    void set_current_sorting(MenuCommand cmd);
    
private:
    HMENU create_menu(bool has_selection);
    HMENU create_grouping_submenu();
    HMENU create_sorting_submenu();
    HMENU create_selection_submenu();
    
    void add_menu_item(HMENU menu, MenuCommand cmd, const std::wstring& text, bool checked = false);
    void add_separator(HMENU menu);
    
    std::wstring get_command_text(MenuCommand cmd) const;
    
private:
    CommandCallback m_command_cb;
    MenuCommand m_current_grouping;
    MenuCommand m_current_sorting;
};

} // namespace albumart_grid