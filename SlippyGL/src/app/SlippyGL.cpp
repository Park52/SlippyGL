#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>

#include "core/TileMath.hpp"
#include "core/Types.hpp"
#include "net/HttpClient.hpp"
#include "net/TileEndpoint.hpp"
#include "cache/DiskCache.hpp"
#include "cache/CacheTypes.hpp"
#include "tile/TileDownloader.hpp"

namespace slippygl::smoketest 
{
    namespace fs = std::filesystem;

	using namespace slippygl::core;
	using namespace slippygl::net;
    using namespace slippygl::cache;
	using namespace slippygl::tile;

	// 슬리피맵 타일을 가져와서 저장하는 간단한 테스트 프로그램
    void RunSlippyGLTest() 
    {
        // 1) 타겟 위치/줌 (서울시청 근처)
        constexpr double lat = 37.5665;
        constexpr double lon = 126.9780;
        constexpr int z = 12;

        // 2) 위경도 -> TileID (우리 함수명/네임스페이스와 일치)
        const slippygl::core::TileID id = slippygl::core::TileMath::lonlatToTileID(lon, lat, z);
        std::cout << "[Tile] " << id.toString() << std::endl;

        // 3) URL 생성 (타일 서버 기본값 사용)
        slippygl::net::TileEndpoint ep;
        const std::string url = ep.rasterUrl(id);
        std::cout << "[URL]  " << url << std::endl;

        // 4) HttpClient 설정 및 GET
        slippygl::net::NetConfig cfg;
        cfg.setUserAgent("SlippyGL/0.1 (+you@example.com)")
            .setVerifyTLS(true)
            .setHttp2(true)
            .setMaxRetries(2);
        slippygl::net::HttpClient http(cfg);

        auto resp = http.get(url);
        std::cout << "[HTTP] status=" << resp.status() << " bytes=" << resp.body().size() << std::endl;

        // 5) 성공 시 파일 저장
        if ((200 == resp.status()) && (!resp.body().empty())) 
        {
            std::ofstream ofs("tile.png", std::ios::binary);
            const auto& b = resp.body();
            ofs.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
            ofs.close();
            std::cout << "[SAVE] tile.png written\n";
            return;
        }
        else 
        {
            std::cerr << "[ERROR] fetch failed\n";
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
        std::cout << (ok ? "[OK] " : "[FAIL] ") << what << "\n";
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
            std::cout << "[INFO] cache root: " << root.string() << "\n";

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

            std::cout << "\n[RESULT] " << (ok ? "ALL PASS" : "SOME FAILED") << "\n";
        }
        catch (const std::exception& ex) {
            std::cerr << "[EXCEPTION] " << ex.what() << "\n";
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
}

int main() 
{
    //  slippygl::smoketest::RunSlippyGLTest();
	//  slippygl::smoketest::RunDiskCacheTest();
	slippygl::smoketest::RunTileDownloaderTest();
	return 0;
}
