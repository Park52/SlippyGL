#include "TileDownloader.hpp"
#include <spdlog/spdlog.h>

namespace slippygl::tile {

TileDownloader::TileDownloader(slippygl::net::HttpClient& http,
                               slippygl::net::TileEndpoint& endpoint)
: http_(http), ep_(endpoint)
{}

FetchResult TileDownloader::ensureRaster(const slippygl::core::TileID& id)
{
    FetchResult r;

    // Network download (no disk cache — OSM policy).
    const std::string url = ep_.rasterUrl(id);
    const auto resp = http_.get(url);

    r.httpStatus   = resp.status();
    r.effectiveUrl = resp.effectiveUrl();

    if (resp.status() == 200)
    {
        r.body = resp.body();
        r.code = FetchCode::kDownloaded;
        return r;
    }
    else if (resp.status() == 404)
    {
        spdlog::warn("Tile not found: {}", url);
        r.code = FetchCode::kNotFound;
        return r;
    }
    else
    {
        spdlog::warn("HTTP error {} for {}", resp.status(), url);
        r.code = FetchCode::kError;
        return r;
    }
}

} // namespace slippygl::tile
