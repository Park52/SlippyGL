#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include "../core/Types.hpp"
#include "CacheTypes.hpp"

namespace slippygl::cache {

// PNG 타일 디스크 캐시 (스레드 안전, 원자적 저장)
class DiskCache {
public:
    explicit DiskCache(const CacheConfig& cfg);

    // PNG 바이트 로드 (히트면 true, miss면 false)
    bool loadRaster(const slippygl::core::TileID& id, std::vector<std::uint8_t>& outBytes) noexcept;

    // PNG 바이트 저장 (원자적 저장 .part -> rename). 메타도 함께 저장 가능.
    bool saveRaster(const slippygl::core::TileID& id,
                    const std::vector<std::uint8_t>& bytes,
                    const std::optional<CacheMeta>& meta = std::nullopt) noexcept;

    // 메타데이터만 개별 로드/저장
    bool loadMeta(const slippygl::core::TileID& id, CacheMeta& out) noexcept;
    bool saveMeta(const slippygl::core::TileID& id, const CacheMeta& meta) noexcept;

    // 존재/삭제/전체 비우기
    bool exists(const slippygl::core::TileID& id) const noexcept;
    bool remove(const slippygl::core::TileID& id) noexcept;
    void clearAll() noexcept;

    // 경로 헬퍼
    std::string rasterPath(const slippygl::core::TileID& id) const;
    std::string metaPath(const slippygl::core::TileID& id) const;

private:
    CacheConfig cfg_;
    mutable std::mutex mtx_; // 간단 전체 락 (필요 시 타일별 파인그레인 락으로 교체 가능)

    // 내부 유틸
    bool ensureParentDir(const std::string& filePath) const noexcept;
    bool atomicWriteFile(const std::string& dstPath, const std::vector<std::uint8_t>& bytes) const noexcept;
    bool atomicWriteText(const std::string& dstPath, const std::string& text) const noexcept;
};

} // namespace slippygl::cache
