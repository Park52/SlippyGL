#pragma once

#include <cstdint>
#include <cmath>
#include <functional>
#include <string>
#include <glm/glm.hpp>

namespace slippygl::tile
{
    /// Tile size in pixels (standard OSM tiles)
    constexpr int kTileSizePx = 256;

    /**
     * Unique key for a map tile (Z/X/Y)
     * Supports hashing for use in unordered containers
     */
    struct TileKey
    {
        int z = 0;  // Zoom level (0-22)
        int x = 0;  // X index (0 to 2^z - 1)
        int y = 0;  // Y index (0 to 2^z - 1)

        TileKey() = default;
        TileKey(int z_, int x_, int y_) : z(z_), x(x_), y(y_) {}

        bool operator==(const TileKey& other) const noexcept
        {
            return z == other.z && x == other.x && y == other.y;
        }

        bool operator!=(const TileKey& other) const noexcept
        {
            return !(*this == other);
        }

        /// For ordered containers
        bool operator<(const TileKey& other) const noexcept
        {
            if (z != other.z) return z < other.z;
            if (x != other.x) return x < other.x;
            return y < other.y;
        }

        /// String representation for logging
        std::string toString() const
        {
            return std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y);
        }

        /// Check if tile index is valid for zoom level
        bool isValid() const noexcept
        {
            if (z < 0 || z > 22) return false;
            const int maxIdx = (1 << z) - 1;
            return x >= 0 && x <= maxIdx && y >= 0 && y <= maxIdx;
        }

        /// Get maximum tile index for this zoom level
        int maxIndex() const noexcept
        {
            return (1 << z) - 1;
        }
    };

    /**
     * Coordinate conversion utilities for tile mapping
     * All functions use Y-down coordinate system
     */
    namespace TileCoord
    {
        /**
         * Convert world pixel coordinate to tile index
         * @param worldPx World pixel coordinate (X or Y)
         * @param tileSizePx Tile size in pixels (default 256)
         * @return Tile index
         */
        inline int worldPxToTileIndex(float worldPx, int tileSizePx = kTileSizePx)
        {
            return static_cast<int>(std::floor(worldPx / static_cast<float>(tileSizePx)));
        }

        /**
         * Convert world pixel coordinate to offset within tile
         * @param worldPx World pixel coordinate
         * @param tileSizePx Tile size in pixels
         * @return Pixel offset within tile (0 to tileSizePx-1)
         */
        inline float worldPxToTileOffset(float worldPx, int tileSizePx = kTileSizePx)
        {
            float tilePx = std::floor(worldPx / static_cast<float>(tileSizePx)) * tileSizePx;
            return worldPx - tilePx;
        }

        /**
         * Convert tile index to world pixel coordinate (top-left of tile)
         * @param tileIndex Tile X or Y index
         * @param tileSizePx Tile size in pixels
         * @return World pixel coordinate of tile's top-left corner
         */
        inline float tileIndexToWorldPx(int tileIndex, int tileSizePx = kTileSizePx)
        {
            return static_cast<float>(tileIndex * tileSizePx);
        }

        /**
         * Clamp tile index to valid range for zoom level
         * @param index Tile index
         * @param zoom Zoom level
         * @return Clamped index (0 to 2^zoom - 1)
         */
        inline int clampTileIndex(int index, int zoom)
        {
            const int maxIdx = (1 << zoom) - 1;
            if (index < 0) return 0;
            if (index > maxIdx) return maxIdx;
            return index;
        }
    }

    /**
     * Convert TileKey to world pixel coordinate (top-left of tile)
     * @param key TileKey with z, x, y
     * @return World pixel coordinate of tile's top-left corner as glm::vec2
     */
    inline glm::vec2 tileToWorldPixel(const TileKey& key)
    {
        return glm::vec2(
            TileCoord::tileIndexToWorldPx(key.x),
            TileCoord::tileIndexToWorldPx(key.y)
        );
    }

    /**
     * Convert world pixel to TileKey
     * @param worldPx World pixel coordinate
     * @param zoom Zoom level
     * @return TileKey for the tile containing the pixel
     */
    inline TileKey worldPixelToTile(const glm::vec2& worldPx, int zoom)
    {
        return TileKey(
            zoom,
            TileCoord::worldPxToTileIndex(worldPx.x),
            TileCoord::worldPxToTileIndex(worldPx.y)
        );
    }

} // namespace slippygl::tile

// Hash specialization for TileKey
namespace std
{
    template<>
    struct hash<slippygl::tile::TileKey>
    {
        size_t operator()(const slippygl::tile::TileKey& key) const noexcept
        {
            // Combine z, x, y into a single hash
            // z is small (0-22), x and y can be up to 2^22
            size_t h = static_cast<size_t>(key.z);
            h = h * 31 + static_cast<size_t>(key.x);
            h = h * 31 + static_cast<size_t>(key.y);
            return h;
        }
    };
}
