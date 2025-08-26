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
	kHitDisk = 0,     // 디스크 캐시에서 바로 가져옴
	kDownloaded,      // 네트워크에서 받아와서 캐시에 저장
	kNotModified,     // 304 (조건부 요청 성공, 캐시 재사용)
	kNotFound,        // 404 등
	kError            // 네트워크/IO 오류
};

struct FetchResult
{
	FetchCode code = FetchCode::kError;
	long      httpStatus = 0;                 // 200/304/404/...
	std::string effectiveUrl;                 // redirect 후 최종 URL
	std::optional<slippygl::cache::CacheMeta> meta; // 응답/캐시 메타
	// 바디는 PNG 바이트
	std::vector<std::uint8_t> body;

	bool ok() const
	{
		return ((code == FetchCode::kHitDisk) || (code == FetchCode::kDownloaded) || (code == FetchCode::kNotModified));
	}
};

// 단일 쓰레드 동기 구현(초기). 이후 multi-queue/취소 토큰은 확장.
class TileDownloader
{
public:
	TileDownloader(slippygl::cache::DiskCache& disk,
		slippygl::net::HttpClient& http,
		slippygl::net::TileEndpoint& endpoint);

	// 기본 경로: 캐시 히트면 바로 반환, 미스면 다운로드 후 저장
	FetchResult ensureRaster(const slippygl::core::TileID& id);

	// 조건부 요청 사용(캐시에 메타가 있으면 If-None-Match / If-Modified-Since 세팅)
	// 캐시에 메타가 없으면 ensureRaster와 동일 동작
	FetchResult ensureRasterConditional(const slippygl::core::TileID& id);

	// 캐시만 확인(네트워크 접근 X)
	bool tryLoadFromDisk(const slippygl::core::TileID& id, std::vector<std::uint8_t>& outBytes,
		std::optional<slippygl::cache::CacheMeta>& outMeta) const;

private:
	slippygl::cache::DiskCache& disk_;
	slippygl::net::HttpClient& http_;
	slippygl::net::TileEndpoint& ep_;

	// 동시성 보호 (초기엔 coarse lock; 나중에 타일별 세분화 가능)
	mutable std::mutex mtx_;
};

} // namespace slippygl::tile