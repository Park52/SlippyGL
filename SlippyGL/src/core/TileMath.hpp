#pragma once
#include <cmath>
#include <algorithm>
#include "Types.hpp"

namespace slippygl::core
{

// Web Mercator 상수
class WebMercator
{
public:
	static constexpr int32_t   kTileSize = 256;
	static constexpr double kMinLatDeg = -85.05112878;
	static constexpr double kMaxLatDeg = 85.05112878;
	static constexpr double kMinLonDeg = -180.0;
	static constexpr double kMaxLonDeg = 180.0;

	static inline double clampLat(const double latDeg) noexcept
	{
		return std::max(kMinLatDeg, std::min(kMaxLatDeg, latDeg));
	}

	static inline double clampLon(const double lonDeg) noexcept
	{
		// 경도는 -180..180 범위로 노멀라이즈
		double lon = std::fmod(lonDeg + 180.0, 360.0);
		if (lon < 0) lon += 360.0;
		return lon - 180.0;
	}
};

// 좌표 변환/유틸 집합 (정적 메서드)
class TileMath
{
public:
	// 줌 z에서의 월드 픽셀 크기(한 변 길이)
	static constexpr int32_t worldSizePx(const int32_t z, const int32_t tileSize = WebMercator::kTileSize) noexcept
	{
		return tileSize * (1 << z);
	}

	// 경도→월드 X픽셀
	static inline double lonToXpx(const double lonDeg, const int32_t z, const int32_t tileSize = WebMercator::kTileSize) noexcept
	{
		const double clampedLon = WebMercator::clampLon(lonDeg);
		const double world = static_cast<double>(worldSizePx(z, tileSize));
		return (clampedLon + 180.0) / 360.0 * world;
	}

	// 위도→월드 Y픽셀
	static inline double latToYpx(const double latDeg, const int32_t z, const int32_t tileSize = WebMercator::kTileSize) noexcept
	{
		constexpr double kPi = 3.14159265358979323846;
		const double clampedLat = std::clamp(latDeg, WebMercator::kMinLatDeg, WebMercator::kMaxLatDeg);
		const double phi = clampedLat * kPi / 180.0;
		const double world = static_cast<double>(worldSizePx(z, tileSize));
		const double s = std::tan(phi) + 1.0 / std::cos(phi); // tanφ + secφ
		const double clampedLat = std::clamp(latDeg, WebMercator::kMinLatDeg, WebMercator::kMaxLatDeg);
		const double phi = clampedLat * M_PI / 180.0;
		const double world = static_cast<double>(worldSizePx(z, tileSize));
		const double s = std::tan(phi) + 1.0 / std::cos(phi); // tanφ + secφ
		return (1.0 - std::log(s) / M_PI) * 0.5 * world;
	}

	// 월드 픽셀 → 타일 인덱스
	static constexpr int32_t pxToTile(const double px, const int32_t tileSize = WebMercator::kTileSize) noexcept
	{
		return static_cast<int32_t>(std::floor(px / static_cast<double>(tileSize)));
	}

	// 위경도 → 타일 좌표
	static inline TileID lonlatToTileID(const double lonDeg, const double latDeg, const int32_t z, const int32_t tileSize = WebMercator::kTileSize) noexcept
	{
		const double xpx = lonToXpx(lonDeg, z, tileSize);
		const double ypx = latToYpx(latDeg, z, tileSize);
		int32_t tx = pxToTile(xpx, tileSize);
		int32_t ty = pxToTile(ypx, tileSize);
		const int32_t maxT = (1 << z);

		// X는 랩어라운드
		tx = ((tx % maxT) + maxT) % maxT;
		// Y는 0..maxT-1로 클램프
		ty = std::max(0, std::min(maxT - 1, ty));
		return TileID(z, tx, ty);
	}

	// 뷰포트(스크린 픽셀)와 카메라 중심(lon,lat) 기준으로 가시 타일 범위 계산
	// centerLon/centerLat는 카메라 중심. 반환은 [minX..maxX], [minY..maxY]
	static inline TileRange computeVisibleTiles(const double centerLonDeg,
												const double centerLatDeg,
												const int32_t z,
												const Viewport& vp,
												const int32_t tileSize = WebMercator::kTileSize) noexcept
	{
		const double cx = lonToXpx(centerLonDeg, z, tileSize);
		const double cy = latToYpx(centerLatDeg, z, tileSize);
		const double halfW = vp.w() * 0.5;
		const double halfH = vp.h() * 0.5;

		const double left = cx - halfW;
		const double right = cx + halfW;
		const double top = cy - halfH;
		const double bottom = cy + halfH;

		const auto toTile = [tileSize](const double px) noexcept { 
			return static_cast<int32_t>(std::floor(px / static_cast<double>(tileSize))); 
		};
		int32_t minX = toTile(left), maxX = toTile(right);
		int32_t minY = toTile(top), maxY = toTile(bottom);

		const int32_t maxT = (1 << z);
		const auto wrapX = [maxT](const int32_t x) noexcept { 
			const int32_t m = x % maxT; 
			return (m < 0) ? (m + maxT) : m; 
		};

		minX = wrapX(minX);
		maxX = wrapX(maxX);
		minY = std::max(0, std::min(maxT - 1, minY));
		maxY = std::max(0, std::min(maxT - 1, maxY));

		return TileRange(z, minX, minY, maxX, maxY);
	}
};
} // namespace slippygl::core
