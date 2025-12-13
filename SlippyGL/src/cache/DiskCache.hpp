#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include "../core/Types.hpp"
#include "CacheTypes.hpp"

namespace slippygl::cache {

// PNG tile disk cache (thread-safe, atomic save)
class DiskCache {
public:
    explicit DiskCache(const CacheConfig& cfg);

    // Load PNG bytes (true on hit, false on miss)
    bool loadRaster(const slippygl::core::TileID& id, std::vector<std::uint8_t>& outBytes) noexcept;

    // Save PNG bytes (atomic: .part -> rename). Can also save metadata.
    bool saveRaster(const slippygl::core::TileID& id,
                    const std::vector<std::uint8_t>& bytes,
                    const std::optional<CacheMeta>& meta = std::nullopt) noexcept;

    // Load/save metadata only
    bool loadMeta(const slippygl::core::TileID& id, CacheMeta& out) noexcept;
    bool saveMeta(const slippygl::core::TileID& id, const CacheMeta& meta) noexcept;

    // Existence check/remove/clear all
    bool exists(const slippygl::core::TileID& id) const noexcept;
    bool remove(const slippygl::core::TileID& id) noexcept;
    void clearAll() noexcept;

    // Path helpers
    std::string rasterPath(const slippygl::core::TileID& id) const;
    std::string metaPath(const slippygl::core::TileID& id) const;

private:
    CacheConfig cfg_;
    mutable std::mutex mtx_; // Simple global lock (can upgrade to per-tile fine-grained lock if needed)

    // Internal utilities
    bool ensureParentDir(const std::string& filePath) const noexcept;
    bool atomicWriteFile(const std::string& dstPath, const std::vector<std::uint8_t>& bytes) const noexcept;
    bool atomicWriteText(const std::string& dstPath, const std::string& text) const noexcept;
    
    // Internal method called without lock (must be called when lock is already held)
    bool saveMetaInternal(const slippygl::core::TileID& id, const CacheMeta& meta) noexcept;
};

} // namespace slippygl::cache
