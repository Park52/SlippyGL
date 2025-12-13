#include "TileDownloader.hpp"
#include <spdlog/spdlog.h>

namespace slippygl::tile {

TileDownloader::TileDownloader(slippygl::cache::DiskCache& disk,
                               slippygl::net::HttpClient& http,
                               slippygl::net::TileEndpoint& endpoint)
: disk_(disk), http_(http), ep_(endpoint) 
{}

bool TileDownloader::tryLoadFromDisk(const slippygl::core::TileID& id,
                                     std::vector<std::uint8_t>& outBytes,
                                     std::optional<slippygl::cache::CacheMeta>& outMeta) const
{
    std::lock_guard<std::mutex> lk(mtx_);
    const bool hit = disk_.loadRaster(id, outBytes);
	if (!hit)
	{
		return false;
	}

    slippygl::cache::CacheMeta m;
    if (disk_.loadMeta(id, m))
    {
        outMeta = m;
    }
    else
    {
        outMeta.reset();
    }
    return true;
}

FetchResult TileDownloader::ensureRaster(const slippygl::core::TileID& id) 
{
    FetchResult r;

    // 1) Check disk cache
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (disk_.loadRaster(id, r.body)) 
        {
            slippygl::cache::CacheMeta m;
            if (disk_.loadMeta(id, m))
            {
                r.meta = m;
            }
            r.code = FetchCode::kHitDisk;
            r.httpStatus = 0;
            return r;
        }
    }

    // 2) Network download
    const std::string url = ep_.rasterUrl(id);
    const auto resp = http_.get(url);

    r.httpStatus  = resp.status();
    r.effectiveUrl = resp.effectiveUrl();

    if (resp.status() == 200) 
    {
        r.body = resp.body();
        // Build meta
        slippygl::cache::CacheMeta meta;
        if (const auto& e = resp.headers().etag())
        {
            meta.setEtag(e);
        }
        if (const auto& lm = resp.headers().lastModified())
        {
            meta.setLastModified(lm);
        }
        if (const auto& ct = resp.headers().contentType())
        {
            meta.setContentType(ct);
        }
        if (const auto& ce = resp.headers().contentEncoding())
        {
            meta.setContentEncoding(ce);
        }
        if (const auto& cl = resp.headers().contentLength())
        {
            meta.setContentLength(*cl);
        }
        meta.touch(static_cast<std::uint64_t>(time(nullptr)));

        // 3) Save (atomic)
        {
            std::lock_guard<std::mutex> lk(mtx_);
            disk_.saveRaster(id, r.body, meta);
        }
        r.meta = meta;
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

FetchResult TileDownloader::ensureRasterConditional(const slippygl::core::TileID& id) 
{
    // If cache meta exists, make conditional request with If-None-Match/If-Modified-Since headers
    std::optional<slippygl::cache::CacheMeta> cachedMeta;
    std::vector<std::uint8_t> cachedBytes;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (disk_.loadRaster(id, cachedBytes))
        {
            slippygl::cache::CacheMeta m;
            if (disk_.loadMeta(id, m))
            {
                cachedMeta = m;
            }
        }
    }
    if (!cachedMeta.has_value()) 
    {
        return ensureRaster(id); // No meta, use normal path
    }

    slippygl::net::Conditional cond;
    if (cachedMeta->etag())
    {
        cond.setIfNoneMatch(*cachedMeta->etag());
    }
    if (cachedMeta->lastModified())
    {
        cond.setIfModifiedSince(*cachedMeta->lastModified());
    }

    const std::string url = ep_.rasterUrl(id);
    auto resp = http_.get(url, /*headers*/nullptr, &cond);

    FetchResult r;
    r.httpStatus = resp.status();
    r.effectiveUrl = resp.effectiveUrl();

    if (resp.status() == 304) 
    { 
        // Not Modified -> reuse cache
        r.code = FetchCode::kNotModified;
        r.body = std::move(cachedBytes);
        r.meta = cachedMeta;
        // If you want to update access time only, call DiskCache.saveMeta(...)
        return r;
    }
    if (resp.status() == 200) 
    {
        r.body = resp.body();
        slippygl::cache::CacheMeta meta;
        if (const auto& e = resp.headers().etag())
        {
            meta.setEtag(e);
        }
        if (const auto& lm = resp.headers().lastModified())
        {
            meta.setLastModified(lm);
        }
        if (const auto& ct = resp.headers().contentType())
        {
            meta.setContentType(ct);
        }
        if (const auto& ce = resp.headers().contentEncoding())
        {
            meta.setContentEncoding(ce);
        }
        if (const auto& cl = resp.headers().contentLength())
        {
            meta.setContentLength(*cl);
        }

        meta.touch(static_cast<std::uint64_t>(time(nullptr)));
        {
            std::lock_guard<std::mutex> lk(mtx_);
            (void)disk_.saveRaster(id, r.body, meta);
        }
        r.meta = meta;
        r.code = FetchCode::kDownloaded;
        return r;
    }
    if (resp.status() == 404) 
    {
        r.code = FetchCode::kNotFound;
        return r;
    }
    r.code = FetchCode::kError;
    return r;
}

} // namespace slippygl::tile
