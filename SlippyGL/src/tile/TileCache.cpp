#include "TileCache.hpp"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace slippygl::tile
{

TileCache::TileCache(std::size_t budgetBytes)
    : budgetBytes_(budgetBytes)
{
    spdlog::info("TileCache initialized with {} MB budget", budgetBytes / (1024 * 1024));
}

TileCache::~TileCache()
{
    clear();
}

bool TileCache::get(const TileKey& key, render::TexHandle& outTex)
{
    auto it = cache_.find(key);
    if (it == cache_.end())
    {
        ++missCount_;
        return false;
    }

    // Update LRU order
    moveToFront(key);
    
    // Update last used time
    it->second.entry.lastUsed = std::chrono::steady_clock::now();
    
    outTex = it->second.entry.texture;
    ++hitCount_;
    
    return true;
}

void TileCache::put(const TileKey& key, render::TexHandle tex, std::size_t sizeBytes)
{
    // If already exists, remove old entry first
    auto it = cache_.find(key);
    if (it != cache_.end())
    {
        usedBytes_ -= it->second.entry.sizeBytes;
        lruList_.erase(it->second.lruIter);
        glDeleteTextures(1, &it->second.entry.texture);
        cache_.erase(it);
    }

    // Evict if needed before adding new entry
    evictIfNeeded(budgetBytes_ - sizeBytes);

    // Add new entry
    lruList_.push_front(key);
    
    CacheNode node;
    node.entry.texture = tex;
    node.entry.sizeBytes = sizeBytes;
    node.entry.lastUsed = std::chrono::steady_clock::now();
    node.lruIter = lruList_.begin();
    
    cache_[key] = std::move(node);
    usedBytes_ += sizeBytes;

    spdlog::debug("TileCache: put {} ({} KB), total {} MB / {} MB",
        key.toString(), 
        sizeBytes / 1024,
        usedBytes_ / (1024 * 1024),
        budgetBytes_ / (1024 * 1024));
}

bool TileCache::contains(const TileKey& key) const
{
    return cache_.find(key) != cache_.end();
}

void TileCache::evictIfNeeded(std::size_t targetBytes)
{
    if (targetBytes == 0)
    {
        targetBytes = budgetBytes_;
    }

    while (usedBytes_ > targetBytes && !lruList_.empty())
    {
        evictOne();
    }
}

void TileCache::clear()
{
    for (auto& [key, node] : cache_)
    {
        if (node.entry.texture != 0)
        {
            glDeleteTextures(1, &node.entry.texture);
        }
    }
    
    spdlog::info("TileCache: cleared {} entries, freed {} MB",
        cache_.size(), usedBytes_ / (1024 * 1024));

    cache_.clear();
    lruList_.clear();
    usedBytes_ = 0;
}

void TileCache::moveToFront(const TileKey& key)
{
    auto it = cache_.find(key);
    if (it == cache_.end()) return;

    // Move to front of LRU list
    lruList_.erase(it->second.lruIter);
    lruList_.push_front(key);
    it->second.lruIter = lruList_.begin();
}

void TileCache::evictOne()
{
    if (lruList_.empty()) return;

    // Get least recently used (back of list)
    const TileKey& lruKey = lruList_.back();
    
    auto it = cache_.find(lruKey);
    if (it != cache_.end())
    {
        spdlog::debug("TileCache: evicting {} ({} KB)",
            lruKey.toString(), it->second.entry.sizeBytes / 1024);

        // Delete OpenGL texture
        if (it->second.entry.texture != 0)
        {
            glDeleteTextures(1, &it->second.entry.texture);
        }

        usedBytes_ -= it->second.entry.sizeBytes;
        cache_.erase(it);
    }

    lruList_.pop_back();
}

} // namespace slippygl::tile
