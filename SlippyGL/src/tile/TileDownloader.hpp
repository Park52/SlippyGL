#pragma once
#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include <mutex>

#include "../core/Types.hpp"        // TileID
#include "../net/HttpClient.hpp"    // HttpClient, NetConfig, HttpResponse
#include "../net/TileEndpoint.hpp"  // TileEndpoint
#include "../cache/DiskCache.hpp"   // DiskCache, CacheMeta

namespace slippygl::tile
{
enum class FetchCode : uint8_t
{
	kHitDisk = 0,     // Retrieved from disk cache
	kDownloaded,      // Downloaded from network, saved to cache
	kNotModified,     // 304 (conditional request success, cache reuse)
	kNotFound,        // 404 etc.
	kError            // Network/IO error
};

struct FetchResult
{
	FetchCode code = FetchCode::kError;
	long      httpStatus = 0;                 // 200/304/404/...
	std::string effectiveUrl;                 // Final URL after redirect
	std::optional<slippygl::cache::CacheMeta> meta; // Response/cache metadata
	// Body is PNG bytes
	std::vector<std::uint8_t> body;

	bool ok() const
	{
		return ((code == FetchCode::kHitDisk) || (code == FetchCode::kDownloaded) || (code == FetchCode::kNotModified));
	}
};

// Single-threaded synchronous implementation (initial). Multi-queue/cancellation token extension later.
class TileDownloader
{
public:
	TileDownloader(slippygl::cache::DiskCache& disk,
		slippygl::net::HttpClient& http,
		slippygl::net::TileEndpoint& endpoint);

	// Default path: return immediately on cache hit, download and save on miss
	FetchResult ensureRaster(const slippygl::core::TileID& id);

	// Use conditional request (set If-None-Match / If-Modified-Since if cache meta exists)
	// If no meta in cache, behaves same as ensureRaster
	FetchResult ensureRasterConditional(const slippygl::core::TileID& id);

	// Check cache only (no network access)
	bool tryLoadFromDisk(const slippygl::core::TileID& id, std::vector<std::uint8_t>& outBytes,
		std::optional<slippygl::cache::CacheMeta>& outMeta) const;

private:
	slippygl::cache::DiskCache& disk_;
	slippygl::net::HttpClient& http_;
	slippygl::net::TileEndpoint& ep_;

	// Concurrency protection (coarse lock initially; can be refined per-tile later)
	mutable std::mutex mtx_;
};

} // namespace slippygl::tile