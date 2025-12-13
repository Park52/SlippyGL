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
 * OpenGL 타일 렌더링 데모
 * TileDownloader -> PngCodec -> TextureManager -> QuadRenderer 파이프라인
 * Camera2D를 통한 팬/줌 지원
 */
void RunTileRenderDemo()
{
    using namespace slippygl;

    // 1) OpenGL 컨텍스트/윈도우 초기화
    render::GlBootstrap gl;
    render::WindowConfig winCfg{ 800, 600, "SlippyGL - Tile Render Demo (Drag=Pan, Scroll=Zoom, R=Reset)" };
    
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

    // 5) 타일 다운로드 (서울시청 근처, 줌 12)
    constexpr double lat = 37.5665;
    constexpr double lon = 126.9780;
    constexpr int zoom = 12;
    const auto tileId = core::TileMath::lonlatToTileID(lon, lat, zoom);
    
    spdlog::info("Downloading tile: {}", tileId.toString());
    
    const auto fetchResult = downloader.ensureRaster(tileId);
    if (!fetchResult.ok()) {
        spdlog::error("Tile download failed (HTTP {})", fetchResult.httpStatus);
        return;
    }
    
    spdlog::info("Tile downloaded: {} bytes", fetchResult.body.size());

    // 6) PNG 디코딩
    decode::Image img;
    std::string decodeErr;
    
    if (!decode::PngCodec::decode(fetchResult.body, img, 4, &decodeErr)) {
        spdlog::error("PNG decode failed: {}", decodeErr);
        return;
    }
    
    spdlog::info("Image decoded: {}x{} ({} channels)", 
        img.width, img.height, img.channels);

    // 7) 텍스처 생성
    const render::TexHandle tex = texMgr.createRGBA8(img.width, img.height, img.pixels.data());
    if (tex == 0) {
        spdlog::error("Texture creation failed");
        return;
    }
    
    spdlog::info("Texture created (handle={})", tex);

    // 8) 렌더 루프
    spdlog::info("Entering render loop");
    spdlog::info("Controls: Drag=Pan, Scroll=Zoom, R=Reset, ESC=Exit");
    
    while (!gl.shouldClose()) {
        gl.poll();
        gl.beginFrame(0.2f, 0.2f, 0.3f);  // 진한 파란색 배경

        // 프레임버퍼 크기 가져오기
        const int fbW = gl.width();
        const int fbH = gl.height();
        
        // 타일을 월드 좌표 (0, 0)에 배치 (카메라가 뷰를 변환)
        const int tileW = img.width;
        const int tileH = img.height;
        
        // 초기에 화면 중앙에 보이도록 타일 위치 설정
        render::Quad q;
        q.x = (fbW - tileW) / 2;  // 월드 좌표에서 타일 위치
        q.y = (fbH - tileH) / 2;
        q.w = tileW;
        q.h = tileH;
        q.sx = 0;
        q.sy = 0;
        q.sw = tileW;
        q.sh = tileH;

        // 카메라 MVP 행렬 적용
        const glm::mat4 mvp = camera.mvp(fbW, fbH);
        quadRenderer.draw(tex, q, img.width, img.height, mvp);

        gl.endFrame();
    }

    // 9) 리소스 정리
    spdlog::info("Shutting down...");
    inputHandler.detach();
    texMgr.destroy(tex);
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
