#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "core/TileMath.hpp"
#include "core/Types.hpp"
#include "net/HttpClient.hpp"
#include "net/TileEndpoint.hpp"

namespace slippygl::smoketest 
{
	using namespace slippygl::core;
	using namespace slippygl::net;

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
}

int main() 
{
    slippygl::smoketest::RunSlippyGLTest();
    return 0;
}
