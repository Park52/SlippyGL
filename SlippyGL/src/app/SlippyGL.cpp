#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "core/TileMath.hpp"
#include "core/Types.hpp"
#include "net/HttpClient.hpp"
#include "net/TileEndpoint.hpp"
#include "cache/DiskCache.hpp"
#include "cache/CacheTypes.hpp"
#include "tile/TileDownloader.hpp"
#include "decode/Image.hpp"
#include "decode/PngCodec.hpp"
#include "render/GlBootstrap.hpp"
#include "render/TextureManager.hpp"
#include "render/QuadRenderer.hpp"
#include "render/Camera2D.hpp"
#include "render/InputHandler.hpp"
#include "tile/TileRenderer.hpp"
#include "tile/TileKey.hpp"

namespace slippygl::smoketest 
{
    namespace fs = std::filesystem;

	using namespace slippygl::core;
	using namespace slippygl::net;
    using namespace slippygl::cache;
	using namespace slippygl::tile;
	using namespace slippygl::decode;
	// Simple test program to fetch and save a slippy map tile
    void RunSlippyGLTest() 
    {
        // 1) Target location/zoom (near Seoul City Hall)
        constexpr double lat = 37.5665;
        constexpr double lon = 126.9780;
        constexpr int z = 12;

        // 2) lat/lon -> TileID (matching our function/namespace)
        const slippygl::core::TileID id = slippygl::core::TileMath::lonlatToTileID(lon, lat, z);
        spdlog::info("[Tile] {}", id.toString());

        // 3) Generate URL (using default tile server)
        slippygl::net::TileEndpoint ep;
        const std::string url = ep.rasterUrl(id);
        spdlog::info("[URL]  {}", url);

        // 4) HttpClient setup and GET
        slippygl::net::NetConfig cfg;
        cfg.setUserAgent("SlippyGL/0.1 (+you@example.com)")
            .setVerifyTLS(true)
            .setHttp2(true)
            .setMaxRetries(2);
        slippygl::net::HttpClient http(cfg);

        auto resp = http.get(url);
        spdlog::info("[HTTP] status={} bytes={}", resp.status(), resp.body().size());

        // 5) Save file on success
        if ((200 == resp.status()) && (!resp.body().empty())) 
        {
            std::ofstream ofs("tile.png", std::ios::binary);
            const auto& b = resp.body();
            ofs.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
            ofs.close();
            spdlog::info("[SAVE] tile.png written");
            return;
        }
        else 
        {
            spdlog::error("[ERROR] fetch failed");
            return;
        }
	}
    
    // ---- helpers ----
    static std::vector<uint8_t> makeDummyPng(size_t payload = 2048) {
        static const uint8_t pngSig[8] = { 0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A };
        std::vector<uint8_t> v;
        v.insert(v.end(), pngSig, pngSig + 8);
        v.resize(payload, 0xAB);
        return v;
    }
    static void printOk(bool ok, const char* what) {
        if (ok) {
            spdlog::info("[OK] {}", what);
        } else {
            spdlog::warn("[FAIL] {}", what);
        }
    }

    // ---- scenarios ----
    bool Smoke_MissSaveHit(DiskCache& cache, const TileID& id) {
        std::vector<uint8_t> out;
        bool miss = !cache.loadRaster(id, out);
        printOk(miss, "initial MISS");

        auto bytes = makeDummyPng(2048);
        bool saved = cache.saveRaster(id, bytes);
        printOk(saved, "saveRaster");

        out.clear();
        bool hit = cache.loadRaster(id, out);
        bool eq = hit && out == bytes;
        printOk(eq, "HIT after save (bytes equal)");
        return miss && saved && eq;
    }

    bool Smoke_MetaRoundTrip(DiskCache& cache, const TileID& id) {
        CacheMeta meta;
        meta.setEtag("\"abcd1234\"")
            .setLastModified("Mon, 21 Aug 2025 12:34:56 GMT")
            .setContentType("image/png")
            .setContentEncoding(std::nullopt)
            .setContentLength(2048)
            .touch(static_cast<std::uint64_t>(std::time(nullptr)));

        bool saved = cache.saveMeta(id, meta);
        printOk(saved, "saveMeta");

        CacheMeta m2;
        bool loaded = cache.loadMeta(id, m2);
        bool same =
            m2.etag() == meta.etag() &&
            m2.lastModified() == meta.lastModified() &&
            m2.contentType() == meta.contentType() &&
            m2.contentEncoding() == meta.contentEncoding() &&
            m2.contentLength() == meta.contentLength();
        printOk(loaded && same, "loadMeta equals saved");
        return saved && loaded && same;
    }

    bool Smoke_Overwrite(DiskCache& cache, const TileID& id) {
        auto a = makeDummyPng(1024);
        auto b = makeDummyPng(4096);

        bool s1 = cache.saveRaster(id, a);
        printOk(s1, "saveRaster a");

        std::vector<uint8_t> out;
        bool h1 = cache.loadRaster(id, out) && out == a;
        printOk(h1, "load a");

        bool s2 = cache.saveRaster(id, b);
        printOk(s2, "overwrite with b");

        out.clear();
        bool h2 = cache.loadRaster(id, out) && out == b;
        printOk(h2, "load b (overwritten)");
        return s1 && h1 && s2 && h2;
    }

    bool Smoke_ExistsRemove(DiskCache& cache, const TileID& id) {
        auto bytes = makeDummyPng(1536);
        cache.saveRaster(id, bytes);

        bool ex = cache.exists(id);
        printOk(ex, "exists == true");

        bool rm = cache.remove(id);
        printOk(rm, "remove(id)");

        std::vector<uint8_t> out;
        bool miss = !cache.loadRaster(id, out);
        printOk(miss, "MISS after remove");
        return ex && rm && miss;
    }

    bool Smoke_ClearAll(DiskCache& cache, const TileID& id) {
        auto bytes = makeDummyPng(1024);
        cache.saveRaster(id, bytes);

        CacheMeta meta;
        meta.setContentType("image/png").setContentLength(bytes.size());
        cache.saveMeta(id, meta);

        cache.clearAll();
        bool ex = cache.exists(id);
        printOk(!ex, "exists == false after clearAll");
        return !ex;
    }

    bool Smoke_Concurrency(DiskCache& cache, const TileID& id) {
        auto bytes = makeDummyPng(800);
        auto worker = [&](int /*idx*/) {
            for (int i = 0; i < 20; ++i) {
                if (i % 3 == 0) (void)cache.saveRaster(id, bytes);
                std::vector<uint8_t> tmp;
                (void)cache.loadRaster(id, tmp);
            }
            };
        std::thread t1(worker, 1), t2(worker, 2), t3(worker, 3);
        t1.join(); t2.join(); t3.join();
        printOk(true, "concurrency smoke (no crash)");
        return true;
    }

    void RunDiskCacheTest() 
    {
        try {
            fs::path root = fs::current_path() / "temp-cache";
            spdlog::info("[INFO] cache root: {}", root.string());

            CacheConfig cfg(root.string());
            DiskCache cache(cfg);

            TileID id(12, 3554, 1609);

            bool ok = true;
            ok &= Smoke_MissSaveHit(cache, id);
            ok &= Smoke_MetaRoundTrip(cache, id);
            ok &= Smoke_Overwrite(cache, id);
            ok &= Smoke_ExistsRemove(cache, id);
            ok &= Smoke_ClearAll(cache, id);
            ok &= Smoke_Concurrency(cache, id);

            spdlog::info("\n[RESULT] {}", ok ? "ALL PASS" : "SOME FAILED");
        }
        catch (const std::exception& ex) {
            spdlog::error("[EXCEPTION] {}", ex.what());
        }
    }

    void RunTileDownloaderTest()
    {
        CacheConfig cc((std::filesystem::current_path() / "cache").string());
        DiskCache disk(cc);

        NetConfig nc;
        nc.setUserAgent("SlippyGL/0.1 (+you@example.com)").setVerifyTLS(true).setHttp2(true);
        HttpClient http(nc);

        TileEndpoint ep; // https://tile.openstreetmap.org

        TileDownloader dl(disk, http, ep);

        // 서울 시청 근처 타일
        const auto id = core::TileMath::lonlatToTileID(126.9780, 37.5665, 12);

        const auto r1 = dl.ensureRaster(id);
        if (r1.ok()) 
        {
            std::ofstream f("tile_dl1.png", std::ios::binary);
            f.write(reinterpret_cast<const char*>(r1.body.data()), (std::streamsize)r1.body.size());
        }

        // 조건부 요청 버전(두 번째 호출에 304로 히트 기대)
        const auto r2 = dl.ensureRasterConditional(id);
        // r2.code == kNotModified 또는 kHitDisk/Downloaded 중 하나
    }
    void RunPngCodecTest()
    {
		// 의존 준비(간단 예)
		cache::CacheConfig cc((std::filesystem::current_path() / "cache").string());
		cache::DiskCache disk(cc);

		net::NetConfig nc;
		nc.setUserAgent("SlippyGL/0.1 (+you@example.com)").setVerifyTLS(true).setHttp2(true);
		net::HttpClient http(nc);

		net::TileEndpoint ep;
		tile::TileDownloader dl(disk, http, ep);

		// 서울시청 타일
		auto id = core::TileMath::lonlatToTileID(126.9780, 37.5665, 12);
		auto res = dl.ensureRaster(id);
		if (!res.ok()) 
        { 
            spdlog::error("fetch failed"); 
            return; 
        }

		// PNG -> RGBA
		decode::Image img;
		std::string err;
		if (!decode::PngCodec::decode(res.body, img, 4, &err)) 
        {
			spdlog::error("decode failed: {}", err);
			return;
		}
		spdlog::info("decoded: {}x{} ch={} bytes={}", 
			img.width, img.height, img.channels, img.sizeBytes());

		// (선택) 원본 PNG 저장
		std::ofstream f("tile_raw.png", std::ios::binary);
		f.write(reinterpret_cast<const char*>(res.body.data()), (std::streamsize)res.body.size());
    }
}

/**
 * OpenGL 멀티 타일 렌더링 데모
 * TileRenderer -> TileGrid -> TileCache -> QuadRenderer 파이프라인
 * Camera2D를 통한 팬/줌 지원
 */
void RunTileRenderDemo()
{
    using namespace slippygl;

    // 디버깅을 위해 로그 레벨 설정
    spdlog::set_level(spdlog::level::debug);

    // 1) OpenGL 컨텍스트/윈도우 초기화
    render::GlBootstrap gl;
    render::WindowConfig winCfg{ 800, 600, "SlippyGL - Multi-Tile Render (Drag=Pan, Scroll=Zoom, R=Reset)" };
    
    if (!gl.init(winCfg)) {
        spdlog::error("OpenGL initialization failed");
        return;
    }

    // 2) 렌더링 모듈 초기화
    render::TextureManager texMgr;
    render::QuadRenderer quadRenderer;
    
    if (!quadRenderer.init()) {
        spdlog::error("QuadRenderer initialization failed");
        return;
    }

    // 3) 카메라 및 입력 핸들러 초기화
    render::Camera2D camera;
    render::InputHandler inputHandler;
    inputHandler.attach(gl.window(), &camera);

    // 4) 타일 다운로더 준비
    cache::CacheConfig cacheCfg((std::filesystem::current_path() / "cache").string());
    cache::DiskCache diskCache(cacheCfg);

    net::NetConfig netCfg;
    netCfg.setUserAgent("SlippyGL/0.1 (+contact@example.com)")
          .setVerifyTLS(true)
          .setHttp2(true);
    net::HttpClient http(netCfg);

    net::TileEndpoint endpoint;
    tile::TileDownloader downloader(diskCache, http, endpoint);

    // 5) TileRenderer 초기화 (LRU 텍스처 캐시 포함)
    tile::TileCache texCache(128 * 1024 * 1024); // 128MB texture budget
    tile::TileRenderer tileRenderer(texCache, downloader, texMgr);

    // 6) 초기 카메라 위치 설정 (서울시청 근처, 줌 12)
    constexpr double lat = 37.5665;
    constexpr double lon = 126.9780;
    constexpr int initialZoom = 12;
    
    // 서울시청 타일의 월드 픽셀 좌표 계산
    const auto seoulTile = core::TileMath::lonlatToTileID(lon, lat, initialZoom);
    const auto worldPos = tile::tileToWorldPixel(
        tile::TileKey{ initialZoom, seoulTile.x(), seoulTile.y() });
    
    spdlog::info("Initial tile: {} -> world ({}, {})", 
        seoulTile.toString(), worldPos.x, worldPos.y);

    // 카메라를 서울시청 타일 중심으로 이동
    camera.setWorldOrigin(glm::vec2(
        worldPos.x + tile::kTileSizePx / 2.0f,
        worldPos.y + tile::kTileSizePx / 2.0f));

    // 8) 렌더 루프
    spdlog::info("Entering render loop");
    spdlog::info("Controls: Drag=Pan, Scroll=Zoom, R=Reset, ESC=Exit");
    spdlog::info("Visible tiles will be loaded dynamically");
    
    int frameCount = 0;
    int lastZoomLevel = initialZoom;
    
    while (!gl.shouldClose()) {
        gl.poll();
        gl.beginFrame(0.2f, 0.2f, 0.3f);  // 진한 파란색 배경

        // 프레임버퍼 크기 가져오기
        const int fbW = gl.width();
        const int fbH = gl.height();
        
        // 카메라 스케일 -> 줌 레벨 계산
        // scale 1.0 = 기본 줌 레벨, 0.5 = 줌 아웃, 2.0 = 줌 인
        const float scale = camera.scale();
        int zoomLevel = initialZoom;
        
        // 스케일에 따른 줌 레벨 조정 (대략적 로그 스케일)
        if (scale < 0.5f) {
            zoomLevel = std::max(0, initialZoom - 2);
        } else if (scale < 0.75f) {
            zoomLevel = std::max(0, initialZoom - 1);
        } else if (scale > 2.0f) {
            zoomLevel = std::min(19, initialZoom + 2);
        } else if (scale > 1.5f) {
            zoomLevel = std::min(19, initialZoom + 1);
        }

        // 줌 레벨이 바뀌면 로그 출력
        if (zoomLevel != lastZoomLevel) {
            spdlog::info("Zoom level changed: {} -> {} (scale: {:.2f})", 
                lastZoomLevel, zoomLevel, scale);
            lastZoomLevel = zoomLevel;
        }

        // TileRenderer로 화면에 보이는 모든 타일 렌더링
        const int tilesRendered = tileRenderer.drawTiles(quadRenderer, camera, zoomLevel, fbW, fbH);
        
        // 프레임 카운터 (주기적으로 통계 출력)
        if (++frameCount % 60 == 0) {
            spdlog::debug("Frame {}: rendered {} tiles, cache: {} MB / {} MB", 
                frameCount, tilesRendered,
                texCache.usedBytes() / (1024 * 1024),
                texCache.budgetBytes() / (1024 * 1024));
        }

        gl.endFrame();
    }

    // 9) 리소스 정리
    spdlog::info("Shutting down...");
    inputHandler.detach();
    texCache.clear();
    quadRenderer.shutdown();
    gl.shutdown();
    
    spdlog::info("Done.");
}

int main() 
{
    // 기존 테스트들 (주석 처리)
    //  slippygl::smoketest::RunSlippyGLTest();
    //  slippygl::smoketest::RunDiskCacheTest();
    //  slippygl::smoketest::RunTileDownloaderTest();
    //  slippygl::smoketest::RunPngCodecTest();
    
    // OpenGL 타일 렌더링 데모 실행
    RunTileRenderDemo();
    
    return 0;
}
