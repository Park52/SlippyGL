#include <algorithm>

#include <spdlog/spdlog.h>

#include "core/TileMath.hpp"
#include "core/Types.hpp"
#include "net/HttpClient.hpp"
#include "net/TileEndpoint.hpp"
#include "tile/TileDownloader.hpp"
#include "render/GlBootstrap.hpp"
#include "render/TextureManager.hpp"
#include "render/QuadRenderer.hpp"
#include "render/TextRenderer.hpp"
#include "render/Camera2D.hpp"
#include "render/InputHandler.hpp"
#include "tile/TileCache.hpp"
#include "tile/TileRenderer.hpp"
#include "tile/TileKey.hpp"

/**
 * OpenGL 멀티 타일 렌더링 데모
 * TileRenderer -> TileGrid -> TileCache -> QuadRenderer 파이프라인
 * Camera2D를 통한 팬/줌 지원
 *
 * 캐시는 인메모리 LRU(TileCache)만 사용한다. 타일을 디스크에 저장하지 않는다.
 * (OSM 타일 정책 준수)
 */
void RunTileRenderDemo()
{
    using namespace slippygl;

    // 디버깅을 위해 로그 레벨 설정
    spdlog::set_level(spdlog::level::debug);

    // 1) OpenGL 컨텍스트/윈도우 초기화
    render::GlBootstrap gl;
    render::WindowConfig winCfg{ 800, 600, "SlippyGL - Multi-Tile Render (WASD/Drag=Pan, Scroll=Zoom, R=Reset, F3=Debug)" };

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

    // 저작자 표시 오버레이 (OSM 정책 필수). 폰트 로드 실패해도 앱은 계속 동작.
    render::TextRenderer overlay;
    if (!overlay.init()) {
        spdlog::warn("Attribution overlay disabled (no font); continuing without it");
    }

    // 3) 카메라 및 입력 핸들러 초기화
    render::Camera2D camera;
    render::InputHandler inputHandler;
    inputHandler.attach(gl.window(), &camera);

    // 4) 타일 다운로더 준비 (네트워크 전용, 디스크 저장 없음)
    net::NetConfig netCfg;
    netCfg.setUserAgent("SlippyGL/0.1 (+https://github.com/Park52/SlippyGL)")
          .setVerifyTLS(true)
          .setHttp2(true);
    net::HttpClient http(netCfg);

    net::TileEndpoint endpoint;
    tile::TileDownloader downloader(http, endpoint);

    // 5) TileRenderer 초기화 (인메모리 LRU 텍스처 캐시 포함)
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
    spdlog::info("Controls: WASD/Arrows or Drag=Pan, Scroll=Zoom, R=Reset, F3=Debug overlay, ESC=Exit");
    spdlog::info("Visible tiles will be loaded dynamically");

    int frameCount = 0;
    int tileZoom = initialZoom;       // 현재 타일 줌 레벨(프레임 간 유지되는 상태)
    int lastZoomLevel = initialZoom;  // 로그 출력용 직전 값

    while (!gl.shouldClose()) {
        gl.poll();

        // WASD(+방향키) 패닝 — 부드러운 가속/감속
        inputHandler.update();

        gl.beginFrame(0.2f, 0.2f, 0.3f);  // 진한 파란색 배경

        // 프레임버퍼 크기 가져오기
        const int fbW = gl.width();
        const int fbH = gl.height();

        // 카메라 스케일이 임계치를 넘으면 타일 줌 레벨을 한 단계 바꾸고
        // 카메라 좌표계를 재매핑한다(화면 뷰는 그대로 보존). 월드 픽셀 좌표계가
        // 항상 현재 타일 줌과 일치하도록 유지해야 타일이 정상 표시된다.
        // (scale이 두 배가 되면 한 레벨 인, 절반이 되면 한 레벨 아웃)
        constexpr int kMinZoom = 0;
        constexpr int kMaxZoom = 19;
        while (camera.scale() > 2.0f && tileZoom < kMaxZoom) {
            ++tileZoom;
            camera.applyZoomStep(2.0f);   // worldOrigin*2, scale/2
        }
        while (camera.scale() < 0.5f && tileZoom > kMinZoom) {
            --tileZoom;
            camera.applyZoomStep(0.5f);   // worldOrigin/2, scale*2
        }

        // 줌 레벨이 바뀌면 로그 출력
        if (tileZoom != lastZoomLevel) {
            spdlog::info("Tile zoom level: {} -> {} (scale: {:.2f})",
                lastZoomLevel, tileZoom, camera.scale());
            lastZoomLevel = tileZoom;
        }

        // TileRenderer로 화면에 보이는 모든 타일 렌더링
        const int tilesRendered = tileRenderer.drawTiles(quadRenderer, camera, tileZoom, fbW, fbH);

        // 디버그 오버레이(F3 토글): 타일 경계 + z/x/y
        if (inputHandler.debugMode()) {
            tileRenderer.drawDebugOverlay(overlay, camera, tileZoom, fbW, fbH);
        }

        // 저작자 표시(우하단 상시 노출) — 항상 최상단에 그린다
        overlay.drawAttribution(fbW, fbH);

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
    overlay.shutdown();
    quadRenderer.shutdown();
    gl.shutdown();

    spdlog::info("Done.");
}

int main()
{
    // OpenGL 타일 렌더링 데모 실행
    RunTileRenderDemo();

    return 0;
}
