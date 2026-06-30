#pragma once
#include <vector>
#include <string>
#include <cstdint>

#include "../core/Types.hpp"        // TileID
#include "../net/HttpClient.hpp"    // HttpClient, HttpResponse
#include "../net/TileEndpoint.hpp"  // TileEndpoint

namespace slippygl::tile
{
enum class FetchCode : uint8_t
{
	kDownloaded = 0,  // Downloaded from network
	kNotFound,        // 404 etc.
	kError            // Network/IO error
};

struct FetchResult
{
	FetchCode code = FetchCode::kError;
	long      httpStatus = 0;                 // 200/404/...
	std::string effectiveUrl;                 // Final URL after redirect
	// Body is PNG bytes
	std::vector<std::uint8_t> body;

	bool ok() const
	{
		return code == FetchCode::kDownloaded;
	}
};

// Network-only tile fetcher (OSM policy: no disk persistence).
// Repeated-access reuse is provided by the in-memory texture LRU (TileCache);
// nothing is written to disk here.
class TileDownloader
{
public:
	TileDownloader(slippygl::net::HttpClient& http,
		slippygl::net::TileEndpoint& endpoint);

	// Download the tile PNG over HTTP. No caching at this layer.
	FetchResult ensureRaster(const slippygl::core::TileID& id);

private:
	slippygl::net::HttpClient& http_;
	slippygl::net::TileEndpoint& ep_;
};

} // namespace slippygl::tile
