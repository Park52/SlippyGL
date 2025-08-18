#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <ostream>

namespace slippygl::core 
{

// 슬리피맵 XYZ 타일 식별자
class TileID 
{
public:
    TileID() = default;
    TileID(int32_t z, int32_t x, int32_t y) : z_(z), x_(x), y_(y) {}

    // getters
    int32_t z() const { return z_; }
    int32_t x() const { return x_; }
    int32_t y() const { return y_; }

    // fluent setters
    TileID& setZ(const int32_t v) noexcept { z_ = v; return *this; }
    TileID& setX(const int32_t v) noexcept { x_ = v; return *this; }
    TileID& setY(const int32_t v) noexcept { y_ = v; return *this; }

    // 동등성
    bool operator==(const TileID& o) const noexcept { return z_==o.z_ && x_==o.x_ && y_==o.y_; }
    bool operator!=(const TileID& o) const noexcept { return !(*this == o); }

    // 정렬(필요 시)
    bool operator<(const TileID& o) const noexcept {
        if (z_ != o.z_) return z_ < o.z_;
        if (x_ != o.x_) return x_ < o.x_;
        return y_ < o.y_;
    }

    // 문자열 표현 (디버깅/로그)
    std::string toString() const;

private:
    int32_t z_ = 0;
    int32_t x_ = 0;
    int32_t y_ = 0;
};

// 화면 또는 월드픽셀 사각형(좌상단 기준 y-down)
class RectI 
{
public:
    RectI() = default;
    RectI(int32_t x, int32_t y, int32_t w, int32_t h) : x_(x), y_(y), w_(w), h_(h) {}

    int32_t x() const { return x_; }
    int32_t y() const { return y_; }
    int32_t w() const { return w_; }
    int32_t h() const { return h_; }

    RectI& setX(const int32_t v) noexcept { x_ = v; return *this; }
    RectI& setY(const int32_t v) noexcept { y_ = v; return *this; }
    RectI& setW(const int32_t v) noexcept { w_ = v; return *this; }
    RectI& setH(const int32_t v) noexcept { h_ = v; return *this; }

private:
    int32_t x_ = 0, y_ = 0, w_ = 0, h_ = 0;
};

// 가시 타일 범위(랩어라운드 고려하여 minX..maxX, minY..maxY)
class TileRange 
{
public:
    TileRange() = default;
    TileRange(int32_t z, int32_t minX, int32_t minY, int32_t maxX, int32_t maxY)
        : z_(z), minX_(minX), minY_(minY), maxX_(maxX), maxY_(maxY) {}

    int32_t z() const { return z_; }
    int32_t minX() const { return minX_; }
    int32_t minY() const { return minY_; }
    int32_t maxX() const { return maxX_; }
    int32_t maxY() const { return maxY_; }

    TileRange& setZ(const int32_t v) noexcept { z_ = v; return *this; }
    TileRange& setMinX(const int32_t v) noexcept { minX_ = v; return *this; }
    TileRange& setMinY(const int32_t v) noexcept { minY_ = v; return *this; }
    TileRange& setMaxX(const int32_t v) noexcept { maxX_ = v; return *this; }
    TileRange& setMaxY(const int32_t v) noexcept { maxY_ = v; return *this; }

private:
    int32_t z_ = 0, minX_ = 0, minY_ = 0, maxX_ = 0, maxY_ = 0;
};

// 뷰포트(스크린 픽셀)
class Viewport 
{
public:
    Viewport() = default;
    Viewport(int32_t w, int32_t h) : w_(w), h_(h) {}
    int32_t w() const { return w_; }
    int32_t h() const { return h_; }
    Viewport& setW(const int32_t v) noexcept { w_ = v; return *this; }
    Viewport& setH(const int32_t v) noexcept { h_ = v; return *this; }
private:
    int32_t w_ = 0, h_ = 0;
};

// ostream 지원
inline std::ostream& operator<<(std::ostream& os, const TileID& id) 
{
    return os << "TileID(" << id.z() << "/" << id.x() << "/" << id.y() << ")";
}

} // namespace slippygl::core

// 해시: unordered_map< TileID, T > 사용 가능하게
template<>
struct std::hash<slippygl::core::TileID> {
    size_t operator()(const slippygl::core::TileID& t) const noexcept {
        // 간단한 3-int 해시
        size_t h = std::hash<int32_t>{}(t.z());
        h = h * 1315423911u ^ std::hash<int32_t>{}(t.x());
        h = h * 1315423911u ^ std::hash<int32_t>{}(t.y());
        return h;
    }
};
