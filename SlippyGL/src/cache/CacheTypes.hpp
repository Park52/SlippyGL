#pragma once
#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace slippygl::cache 
{
class CacheConfig 
{
public:
	CacheConfig() = default;
	explicit CacheConfig(const std::string& rootDir) : rootDir_(rootDir) {}

	const std::string& rootDir() const { return rootDir_; }
	CacheConfig& setRootDir(const std::string& v) { rootDir_ = v; return *this; }

	// 향후 확장: 최대 용량, 파일 만료일, 폴더명 커스터마이즈 등
	const std::uint64_t maxBytes() const { return maxBytes_; }
	CacheConfig& setMaxBytes(const std::uint64_t v) { maxBytes_ = v; return *this; }

	const std::string& rasterDirName() const { return rasterDirName_; }
	const std::string& metaDirName()   const { return metaDirName_; }

private:
	std::string rootDir_;
	std::uint64_t maxBytes_ = 0; // 0 = 무제한
	std::string rasterDirName_ = "raster";
	std::string metaDirName_ = "meta";
};

// HTTP 응답 메타데이터를 캐시에 같이 저장 (사이드카 JSON)
class CacheMeta 
{
public:
	const std::optional<std::string>& etag() const { return etag_; }
	const std::optional<std::string>& lastModified() const { return lastModified_; }
	const std::optional<std::string>& contentType() const { return contentType_; }
	const std::optional<std::string>& contentEncoding() const { return contentEncoding_; }
	const std::uint64_t contentLength() const { return contentLength_; }
	const std::uint64_t lastAccessUnixSec() const { return lastAccessUnixSec_; }

	CacheMeta& setEtag(const std::optional<std::string>& v) { etag_ = v; return *this; }
	CacheMeta& setLastModified(const std::optional<std::string>& v) { lastModified_ = v; return *this; }
	CacheMeta& setContentType(const std::optional<std::string>& v) { contentType_ = v; return *this; }
	CacheMeta& setContentEncoding(const std::optional<std::string>& v) { contentEncoding_ = v; return *this; }
	CacheMeta& setContentLength(const std::uint64_t v) { contentLength_ = v; return *this; }
	CacheMeta& touch(const std::uint64_t unixSec) { lastAccessUnixSec_ = unixSec; return *this; }

	static std::string toJsonString(const CacheMeta& _meta);
	static CacheMeta fromJsonString(const std::string& _json);

private:
	std::optional<std::string> etag_, lastModified_, contentType_, contentEncoding_;
	std::uint64_t contentLength_ = 0;
	std::uint64_t lastAccessUnixSec_ = 0;
};

} // namespace slippygl::cache
