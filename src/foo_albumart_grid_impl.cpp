#include "../include/foo_albumart_grid.h"
#include "../include/album_art_cache.h"
#include "../include/grid_renderer.h"

// Continue implementation of AlbumArtGridPanel private methods

void AlbumArtGridPanel::InitializeDirect2D() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2d_factory);
    if (FAILED(hr)) return;
    
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                            reinterpret_cast<IUnknown**>(&m_dwrite_factory));
    if (FAILED(hr)) return;
    
    // Create render target
    RECT client_rect;
    GetClientRect(&client_rect);
    
    D2D1_SIZE_U size = D2D1::SizeU(
        client_rect.right - client_rect.left,
        client_rect.bottom - client_rect.top);
    
    hr = m_d2d_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hWnd, size),
        &m_render_target);
    
    if (SUCCEEDED(hr)) {
        // Create text format
        hr = m_dwrite_factory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"en-US",
            &m_text_format);
        
        if (SUCCEEDED(hr)) {
            m_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void AlbumArtGridPanel::CleanupDirect2D() {
    if (m_text_format) {
        m_text_format->Release();
        m_text_format = nullptr;
    }
    
    if (m_render_target) {
        m_render_target->Release();
        m_render_target = nullptr;
    }
    
    if (m_dwrite_factory) {
        m_dwrite_factory->Release();
        m_dwrite_factory = nullptr;
    }
    
    if (m_d2d_factory) {
        m_d2d_factory->Release();
        m_d2d_factory = nullptr;
    }
}

void AlbumArtGridPanel::CreateGridItems() {
    std::lock_guard<std::mutex> lock(m_items_mutex);
    
    m_items.clear();
    
    // Get all tracks from media library
    static_api_ptr_t<library_manager> lib_manager;
    metadb_handle_list all_tracks;
    lib_manager->get_all_items(all_tracks);
    
    if (all_tracks.get_count() == 0) return;
    
    // Group tracks based on grouping mode
    std::unordered_map<std::string, std::unique_ptr<GridItem>> grouped_items;
    
    for (t_size i = 0; i < all_tracks.get_count(); ++i) {
        metadb_handle_ptr track = all_tracks[i];
        if (!track.is_valid()) continue;
        
        // Get metadata
        file_info_impl info;
        if (!track->get_info(info)) continue;
        
        std::string key;
        pfc::string8 album, artist, genre, year, folder_path;
        
        // Extract metadata
        const char* album_ptr = info.meta_get("ALBUM", 0);
        const char* artist_ptr = info.meta_get("ARTIST", 0);
        const char* genre_ptr = info.meta_get("GENRE", 0);
        const char* date_ptr = info.meta_get("DATE", 0);
        
        album = album_ptr ? album_ptr : "Unknown Album";
        artist = artist_ptr ? artist_ptr : "Unknown Artist";
        genre = genre_ptr ? genre_ptr : "Unknown Genre";
        
        // Extract year from date
        if (date_ptr && strlen(date_ptr) >= 4) {
            year.set_string(date_ptr, 4);
        } else {
            year = "Unknown Year";
        }
        
        // Get folder path
        const char* path = track->get_path();
        if (path) {
            const char* last_slash = strrchr(path, '\\');
            if (!last_slash) last_slash = strrchr(path, '/');
            if (last_slash) {
                folder_path.set_string(path, last_slash - path);
            } else {
                folder_path = path;
            }
        }
        
        // Generate grouping key
        switch (m_config.grouping_mode) {
        case GroupingMode::BY_ALBUM:
            key = std::string(artist.get_ptr()) + " - " + std::string(album.get_ptr());
            break;
        case GroupingMode::BY_ARTIST:
            key = std::string(artist.get_ptr());
            break;
        case GroupingMode::BY_GENRE:
            key = std::string(genre.get_ptr());
            break;
        case GroupingMode::BY_YEAR:
            key = std::string(year.get_ptr());
            break;
        case GroupingMode::BY_FOLDER:
            key = std::string(folder_path.get_ptr());
            break;
        }
        
        // Find or create grid item
        auto it = grouped_items.find(key);
        if (it == grouped_items.end()) {
            auto item = std::make_unique<GridItem>();
            item->album = album;
            item->artist = artist;
            item->genre = genre;
            item->year = year;
            item->folder_path = folder_path;
            item->tracks.add_item(track);
            grouped_items[key] = std::move(item);
        } else {
            it->second->tracks.add_item(track);
        }
    }
    
    // Convert to vector
    m_items.reserve(grouped_items.size());
    for (auto& pair : grouped_items) {
        m_items.push_back(std::move(pair.second));
    }
    
    // Sort items
    std::sort(m_items.begin(), m_items.end(), 
        [this](const std::unique_ptr<GridItem>& a, const std::unique_ptr<GridItem>& b) {
            switch (m_config.grouping_mode) {
            case GroupingMode::BY_ALBUM:
                return strcmp(a->album.get_ptr(), b->album.get_ptr()) < 0;
            case GroupingMode::BY_ARTIST:
                return strcmp(a->artist.get_ptr(), b->artist.get_ptr()) < 0;
            case GroupingMode::BY_GENRE:
                return strcmp(a->genre.get_ptr(), b->genre.get_ptr()) < 0;
            case GroupingMode::BY_YEAR:
                return strcmp(a->year.get_ptr(), b->year.get_ptr()) < 0;
            case GroupingMode::BY_FOLDER:
                return strcmp(a->folder_path.get_ptr(), b->folder_path.get_ptr()) < 0;
            default:
                return false;
            }
        });
}

void AlbumArtGridPanel::UpdateLayout() {
    if (!m_renderer) return;
    
    RECT client_rect;
    GetClientRect(&client_rect);
    
    GridLayout layout;
    m_renderer->CalculateLayout(client_rect, m_config.grid_size, 
                               (int)m_filtered_indices.size(), layout);
    
    m_items_per_row = layout.items_per_row;
    m_visible_rows = layout.visible_rows;
    m_total_rows = layout.total_rows;
    m_item_width = layout.item_width;
    m_item_height = layout.item_height;
    
    UpdateScrollInfo();
}

void AlbumArtGridPanel::InvalidateGrid() {
    InvalidateRect(nullptr);
}

int AlbumArtGridPanel::GetItemAtPoint(CPoint pt) {
    if (m_items_per_row == 0 || m_item_height == 0) return -1;
    
    int col = pt.x / m_item_width;
    int row = (pt.y + m_scroll_pos) / m_item_height;
    
    if (col < 0 || col >= m_items_per_row || row < 0) return -1;
    
    int item_index = row * m_items_per_row + col;
    if (item_index >= (int)m_filtered_indices.size()) return -1;
    
    return item_index;
}

void AlbumArtGridPanel::EnsureItemVisible(int item_index) {
    if (item_index < 0 || item_index >= (int)m_filtered_indices.size()) return;
    if (m_items_per_row == 0 || m_item_height == 0) return;
    
    int row = item_index / m_items_per_row;
    int item_top = row * m_item_height;
    int item_bottom = item_top + m_item_height;
    
    RECT client_rect;
    GetClientRect(&client_rect);
    int visible_top = m_scroll_pos;
    int visible_bottom = m_scroll_pos + (client_rect.bottom - client_rect.top);
    
    if (item_top < visible_top) {
        m_scroll_pos = item_top;
    } else if (item_bottom > visible_bottom) {
        m_scroll_pos = item_bottom - (client_rect.bottom - client_rect.top);
    }
    
    int max_scroll = max(0, m_total_rows * m_item_height - (client_rect.bottom - client_rect.top));
    m_scroll_pos = max(0, min(m_scroll_pos, max_scroll));
    
    UpdateScrollInfo();
}

void AlbumArtGridPanel::UpdateScrollInfo() {
    RECT client_rect;
    GetClientRect(&client_rect);
    int client_height = client_rect.bottom - client_rect.top;
    int total_height = m_total_rows * m_item_height;
    
    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = total_height;
    si.nPage = client_height;
    si.nPos = m_scroll_pos;
    
    SetScrollInfo(SB_VERT, &si, TRUE);
}

void AlbumArtGridPanel::ScrollToItem(int item_index) {
    if (item_index >= 0 && item_index < (int)m_filtered_indices.size()) {
        EnsureItemVisible(item_index);
        InvalidateRect(nullptr);
    }
}

void AlbumArtGridPanel::PlayItem(int item_index) {
    if (item_index < 0 || item_index >= (int)m_filtered_indices.size()) return;
    
    std::lock_guard<std::mutex> lock(m_items_mutex);
    int actual_index = m_filtered_indices[item_index];
    if (actual_index < 0 || actual_index >= (int)m_items.size()) return;
    
    const auto& item = m_items[actual_index];
    if (item->tracks.get_count() > 0) {
        static_api_ptr_t<playlist_manager> playlist_api;
        playlist_api->activeplaylist_clear();
        playlist_api->activeplaylist_add_items(item->tracks, bit_array_true());
        static_api_ptr_t<playback_control> playback_api;
        playback_api->start();
    }
}

void AlbumArtGridPanel::AddItemToPlaylist(int item_index) {
    if (item_index < 0 || item_index >= (int)m_filtered_indices.size()) return;
    
    std::lock_guard<std::mutex> lock(m_items_mutex);
    int actual_index = m_filtered_indices[item_index];
    if (actual_index < 0 || actual_index >= (int)m_items.size()) return;
    
    const auto& item = m_items[actual_index];
    if (item->tracks.get_count() > 0) {
        static_api_ptr_t<playlist_manager> playlist_api;
        playlist_api->activeplaylist_add_items(item->tracks, bit_array_true());
    }
}

void AlbumArtGridPanel::ShowContextMenu(CPoint pt, int item_index) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    
    AppendMenu(menu, MF_STRING, 1, L"Play");
    AppendMenu(menu, MF_STRING, 2, L"Add to playlist");
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(menu, MF_STRING, 3, L"Properties...");
    
    ClientToScreen(&pt);
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, 
                            pt.x, pt.y, 0, m_hWnd, nullptr);
    
    switch (cmd) {
    case 1:
        PlayItem(item_index);
        break;
    case 2:
        AddItemToPlaylist(item_index);
        break;
    case 3:
        // Show properties dialog
        break;
    }
    
    DestroyMenu(menu);
}

void AlbumArtGridPanel::LoadAlbumArt() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_items_mutex);
        
        if (m_loader_stop) break;
        
        // Find items that need album art
        bool found_work = false;
        for (auto& item : m_items) {
            if (!item->art_loaded && !item->art_loading && item->tracks.get_count() > 0) {
                item->art_loading = true;
                found_work = true;
                
                // Request album art from cache
                std::string key = AlbumArtUtils::GetAlbumKey(
                    item->artist.get_ptr(), item->album.get_ptr());
                
                int art_size = (int)m_config.grid_size - 16; // Leave room for padding
                
                lock.unlock();
                
                m_cache->RequestAlbumArt(key, item->tracks[0], art_size, 
                                        m_hWnd, WM_USER + 1);
                
                // Small delay to prevent overwhelming the system
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                break;
            }
        }
        
        if (!found_work) {
            m_loader_cv.wait(lock);
        }
    }
}

void AlbumArtGridPanel::ApplyFilter() {
    m_filtered_indices.clear();
    
    if (m_filter_text.is_empty()) {
        // No filter, show all items
        for (int i = 0; i < (int)m_items.size(); ++i) {
            m_filtered_indices.push_back(i);
        }
    } else {
        // Apply filter
        pfc::string8 filter_lower = m_filter_text;
        filter_lower.to_lower();
        
        for (int i = 0; i < (int)m_items.size(); ++i) {
            const auto& item = m_items[i];
            
            pfc::string8 search_text;
            search_text << item->artist << " " << item->album << " " 
                       << item->genre << " " << item->year;
            search_text.to_lower();
            
            if (strstr(search_text.get_ptr(), filter_lower.get_ptr()) != nullptr) {
                m_filtered_indices.push_back(i);
            }
        }
    }
    
    // Reset selection if it's out of range
    if (m_selected_item >= (int)m_filtered_indices.size()) {
        m_selected_item = -1;
    }
}