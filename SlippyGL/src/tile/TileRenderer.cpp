#include "TileRenderer.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace slippygl::tile
{

TileRenderer::TileRenderer(TileCache& cache, TileDownloader& downloader, render::TextureManager& texMgr)
    : cache_(cache)
    , downloader_(downloader)
    , texMgr_(texMgr)
{
    // Placeholder 텍스처를 초기화 시점에 미리 생성
    createPlaceholderTexture();
    spdlog::info("TileRenderer initialized with placeholder texture");
}

int TileRenderer::drawTiles(
    render::QuadRenderer& quadRenderer,
    const render::Camera2D& camera,
    int zoom,
    int fbW, int fbH)
{
    // Reset frame statistics
    lastTileCount_ = 0;
    lastCacheHits_ = 0;
    lastDownloads_ = 0;

    // Compute visible tile range
    const auto range = TileGrid::computeVisibleRange(camera, fbW, fbH, zoom);
    
    spdlog::debug("TileRenderer: zoom={}, visible range: x[{},{}] y[{},{}] = {} tiles",
        zoom, range.minX, range.maxX, range.minY, range.maxY, range.tileCount());
    
    // Get MVP matrix from camera
    const glm::mat4 mvp = camera.mvp(fbW, fbH);

    // 프레임당 최대 다운로드 제한 (너무 많으면 블로킹됨)
    constexpr int kMaxDownloadsPerFrame = 3;
    int downloadsThisFrame = 0;

    // Draw each visible tile
    for (int y = range.minY; y <= range.maxY; ++y)
    {
        for (int x = range.minX; x <= range.maxX; ++x)
        {
            TileKey key(zoom, x, y);
            
            // 캐시에 있는지 먼저 확인
            render::TexHandle tex = 0;
            bool inCache = cache_.get(key, tex);
            
            if (!inCache)
            {
                // 캐시 미스 - 다운로드 시도 (프레임당 제한)
                if (downloadsThisFrame < kMaxDownloadsPerFrame)
                {
                    tex = getOrLoadTexture(key);
                    if (tex != 0) {
                        ++downloadsThisFrame;
                    }
                }
            }
            else
            {
                ++lastCacheHits_;
            }
            
            // 텍스처가 없으면 placeholder 사용
            if (tex == 0)
            {
                spdlog::debug("TileRenderer: using placeholder for tile {}", key.toString());
                tex = getPlaceholderTexture();
            }

            if (tex == 0) 
            {
                spdlog::warn("TileRenderer: no texture available for tile {}", key.toString());
                continue;  // Skip if no placeholder either
            }

            // Calculate tile world position
            const glm::vec2 worldPos = TileGrid::tileWorldPosition(key);

            // Create quad in world coordinates (integer snapped)
            render::Quad q;
            q.x = static_cast<int>(std::floor(worldPos.x));
            q.y = static_cast<int>(std::floor(worldPos.y));
            q.w = kTileSizePx;
            q.h = kTileSizePx;
            q.sx = 0;
            q.sy = 0;
            q.sw = kTileSizePx;
            q.sh = kTileSizePx;

            // Draw tile
            quadRenderer.draw(tex, q, kTileSizePx, kTileSizePx, mvp);
            ++lastTileCount_;
        }
    }

    return lastTileCount_;
}

void TileRenderer::drawDebugOverlay(
    render::TextRenderer& text,
    const render::Camera2D& camera,
    int zoom,
    int fbW, int fbH)
{
    if (!text.ready()) return;

    const auto range = TileGrid::computeVisibleRange(camera, fbW, fbH, zoom);

    const glm::vec4 borderColor(1.0f, 1.0f, 0.0f, 0.6f);  // 반투명 노랑
    const glm::vec4 labelColor (1.0f, 1.0f, 0.2f, 1.0f);  // 밝은 노랑 텍스트
    const glm::vec4 labelBg    (0.0f, 0.0f, 0.0f, 0.55f); // 어두운 배경(가독성)

    for (int y = range.minY; y <= range.maxY; ++y)
    {
        for (int x = range.minX; x <= range.maxX; ++x)
        {
            const TileKey key(zoom, x, y);
            const glm::vec2 wpos = TileGrid::tileWorldPosition(key);

            // 타일 코너를 화면 좌표로 투영 (축 정렬 사각형)
            const glm::vec2 tl = camera.worldToScreen(wpos.x, wpos.y);
            const glm::vec2 br = camera.worldToScreen(
                wpos.x + kTileSizePx, wpos.y + kTileSizePx);
            const float sw = br.x - tl.x;
            const float sh = br.y - tl.y;

            // 타일 경계선
            text.drawRectOutline(tl.x, tl.y, sw, sh, 1.0f, borderColor, fbW, fbH);

            // z/x/y 라벨 (좌상단에서 약간 안쪽, 고정 화면 크기)
            text.drawTextBoxed(key.toString(), tl.x + 3.0f, tl.y + 3.0f,
                               labelColor, labelBg, 2.0f, fbW, fbH);
        }
    }
}

render::TexHandle TileRenderer::getOrLoadTexture(const TileKey& key)
{
    // 이 함수는 캐시 미스 시에만 호출됨 (drawTiles에서 이미 확인)
    // 바로 다운로드 + 디코딩 + 캐싱
    
    spdlog::info("TileRenderer: downloading tile {}", key.toString());
    
    // Convert TileKey to TileID for downloader
    core::TileID tileId(key.z, key.x, key.y);

    auto result = downloader_.ensureRaster(tileId);
    if (!result.ok())
    {
        spdlog::warn("TileRenderer: failed to download tile {} (HTTP {})", 
            key.toString(), result.httpStatus);
        return 0;
    }
    
    spdlog::debug("TileRenderer: downloaded tile {} ({} bytes)", 
        key.toString(), result.body.size());

    // Decode PNG
    decode::Image img;
    std::string decodeErr;
    if (!decode::PngCodec::decode(result.body, img, 4, &decodeErr))
    {
        spdlog::warn("TileRenderer: failed to decode tile {}: {}", key.toString(), decodeErr);
        return 0;
    }

    // Create texture
    render::TexHandle tex = texMgr_.createRGBA8(img.width, img.height, img.pixels.data());
    if (tex == 0)
    {
        spdlog::warn("TileRenderer: failed to create texture for tile {}", key.toString());
        return 0;
    }

    // Calculate texture size in bytes (RGBA8 = 4 bytes per pixel)
    const std::size_t texBytes = static_cast<std::size_t>(img.width) * img.height * 4;

    // Put in cache
    cache_.put(key, tex, texBytes);
    ++lastDownloads_;

    spdlog::debug("TileRenderer: loaded tile {} ({}x{}) into cache", 
        key.toString(), img.width, img.height);

    return tex;
}

render::TexHandle TileRenderer::getPlaceholderTexture()
{
    if (placeholderTex_ == 0)
    {
        createPlaceholderTexture();
    }
    return placeholderTex_;
}

void TileRenderer::createPlaceholderTexture()
{
    // Create a gray checkerboard pattern
    constexpr int size = 256;
    constexpr int checkSize = 16;
    std::vector<uint8_t> pixels(size * size * 4);

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const int idx = (y * size + x) * 4;
            const bool dark = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            const uint8_t gray = dark ? 180 : 200;
            
            pixels[idx + 0] = gray;  // R
            pixels[idx + 1] = gray;  // G
            pixels[idx + 2] = gray;  // B
            pixels[idx + 3] = 255;   // A
        }
    }

    placeholderTex_ = texMgr_.createRGBA8(size, size, pixels.data());
    
    if (placeholderTex_ != 0)
    {
        spdlog::debug("TileRenderer: created placeholder texture");
    }
}

} // namespace slippygl::tile
