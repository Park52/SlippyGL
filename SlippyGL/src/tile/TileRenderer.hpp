#pragma once

#include "TileKey.hpp"
#include "TileGrid.hpp"
#include "TileCache.hpp"
#include "TileDownloader.hpp"
#include "../render/Camera2D.hpp"
#include "../render/QuadRenderer.hpp"
#include "../render/TextureManager.hpp"
#include "../decode/PngCodec.hpp"
#include "../decode/Image.hpp"

#include <vector>
#include <cmath>

namespace slippygl::tile
{
    /**
     * Renders visible tiles for current camera view
     * - Computes visible tile grid
     * - Loads/caches textures on demand
     * - Draws tiles with proper positioning and integer snapping
     */
    class TileRenderer
    {
    public:
        /**
         * Constructor
         * @param cache Texture cache (shared ownership)
         * @param downloader Tile downloader
         * @param texMgr Texture manager for creating textures
         */
        TileRenderer(TileCache& cache, TileDownloader& downloader, render::TextureManager& texMgr);
        ~TileRenderer() = default;

        // Non-copyable
        TileRenderer(const TileRenderer&) = delete;
        TileRenderer& operator=(const TileRenderer&) = delete;

        /**
         * Draw all visible tiles
         * @param quadRenderer Quad renderer for drawing
         * @param camera Current camera state
         * @param zoom Tile zoom level
         * @param fbW Framebuffer width
         * @param fbH Framebuffer height
         * @return Number of tiles rendered
         */
        int drawTiles(
            render::QuadRenderer& quadRenderer,
            const render::Camera2D& camera,
            int zoom,
            int fbW, int fbH);

        /**
         * Get placeholder texture for failed/loading tiles
         */
        render::TexHandle getPlaceholderTexture();

        /**
         * Statistics
         */
        int lastTileCount() const noexcept { return lastTileCount_; }
        int lastCacheHits() const noexcept { return lastCacheHits_; }
        int lastDownloads() const noexcept { return lastDownloads_; }

    private:
        TileCache& cache_;
        TileDownloader& downloader_;
        render::TextureManager& texMgr_;

        render::TexHandle placeholderTex_ = 0;

        // Statistics for last frame
        int lastTileCount_ = 0;
        int lastCacheHits_ = 0;
        int lastDownloads_ = 0;

        /**
         * Load or get texture for tile
         * @param key Tile key
         * @return Texture handle (0 if failed)
         */
        render::TexHandle getOrLoadTexture(const TileKey& key);

        /**
         * Create placeholder texture (gray checkerboard)
         */
        void createPlaceholderTexture();
    };

} // namespace slippygl::tile
