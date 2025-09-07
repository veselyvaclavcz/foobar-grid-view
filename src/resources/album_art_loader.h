#pragma once

#include <foobar2000/SDK/foobar2000.h>
#include <gdiplus.h>
#include <memory>
#include <functional>

namespace albumart_grid {

// Album art loader - loads album art asynchronously
class AlbumArtLoader {
public:
    using LoadCallback = std::function<void(const std::string& key, std::unique_ptr<Gdiplus::Bitmap>)>;
    
    AlbumArtLoader();
    ~AlbumArtLoader();
    
    // Load album art for tracks
    void load_async(const metadb_handle_list& tracks, 
                   const std::string& key,
                   int target_size,
                   LoadCallback callback);
    
    // Cancel pending loads
    void cancel_all();
    
private:
    class impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace albumart_grid