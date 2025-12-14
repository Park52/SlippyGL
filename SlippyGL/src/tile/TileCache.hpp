#pragma once

#include "TileKey.hpp"
#include "../render/TextureManager.hpp"
#include <unordered_map>
#include <list>
#include <cstdint>
#include <chrono>

namespace slippygl::tile
{
    /**
     * Cached texture entry
     */
    struct CacheEntry
    {
        render::TexHandle texture = 0;
        std::size_t sizeBytes = 0;
        std::chrono::steady_clock::time_point lastUsed;
    };

    /**
     * LRU texture cache for map tiles
     * - Stores OpenGL textures by TileKey
     * - Evicts least recently used entries when budget exceeded
     * - Thread-unsafe (single-threaded rendering assumed)
     */
    class TileCache
    {
    public:
        /// Default cache budget: 128 MB
        static constexpr std::size_t kDefaultBudgetBytes = 128 * 1024 * 1024;

        explicit TileCache(std::size_t budgetBytes = kDefaultBudgetBytes);
        ~TileCache();

        // Non-copyable
        TileCache(const TileCache&) = delete;
        TileCache& operator=(const TileCache&) = delete;

        /**
         * Get texture for tile (updates LRU order)
         * @param key Tile key
         * @param outTex Output texture handle
         * @return true if found, false if cache miss
         */
        bool get(const TileKey& key, render::TexHandle& outTex);

        /**
         * Put texture into cache
         * @param key Tile key
         * @param tex Texture handle (cache takes ownership)
         * @param sizeBytes Texture memory size in bytes
         */
        void put(const TileKey& key, render::TexHandle tex, std::size_t sizeBytes);

        /**
         * Check if tile is in cache (without updating LRU)
         */
        bool contains(const TileKey& key) const;

        /**
         * Evict entries if over budget
         * @param targetBytes Target budget (default: configured budget)
         */
        void evictIfNeeded(std::size_t targetBytes = 0);

        /**
         * Clear all cached textures
         */
        void clear();

        /**
         * Get current cache statistics
         */
        std::size_t size() const noexcept { return cache_.size(); }
        std::size_t usedBytes() const noexcept { return usedBytes_; }
        std::size_t budgetBytes() const noexcept { return budgetBytes_; }
        std::size_t hitCount() const noexcept { return hitCount_; }
        std::size_t missCount() const noexcept { return missCount_; }

        /**
         * Reset hit/miss counters
         */
        void resetStats() noexcept { hitCount_ = 0; missCount_ = 0; }

    private:
        std::size_t budgetBytes_;
        std::size_t usedBytes_ = 0;
        std::size_t hitCount_ = 0;
        std::size_t missCount_ = 0;

        // LRU list: front = most recently used, back = least recently used
        std::list<TileKey> lruList_;

        // Map from TileKey to (entry, LRU iterator)
        struct CacheNode
        {
            CacheEntry entry;
            std::list<TileKey>::iterator lruIter;
        };
        std::unordered_map<TileKey, CacheNode> cache_;

        void moveToFront(const TileKey& key);
        void evictOne();
    };

} // namespace slippygl::tile
