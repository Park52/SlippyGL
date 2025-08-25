#include "CacheTypes.hpp"
#include "nlohmann/json.hpp"

using namespace slippygl::cache;
using namespace nlohmann;

std::string slippygl::cache::CacheMeta::toJsonString(const CacheMeta& _meta)
{
	/*
		{
			"etag":"\"abcd1234\"",
			"lastModified":"Mon, 21 Aug 2025 12:34:56 GMT",
			"contentType":"image/png",
			"contentEncoding":null,
			"contentLength": 10342,
			"lastAccessUnixSec": 1692600000
		}
	*/
	json j;
	if (_meta.etag().has_value())
	{
		j["etag"] = _meta.etag().value();
	}
	if (_meta.lastModified().has_value())
	{
		j["lastModified"] = _meta.lastModified().value();
	}
	if (_meta.contentType().has_value())
	{
		j["contentType"] = _meta.contentType().value();
	}
	if (_meta.contentEncoding().has_value())
	{
		j["contentEncoding"] = _meta.contentEncoding().value();
	}
	j["contentLength"] = _meta.contentLength();
	j["lastAccessUnixSec"] = _meta.lastAccessUnixSec();

	// 직렬화
#ifdef PRETTY_PRINT_JSON
	return j.dump(4);
#else
	return j.dump();
#endif
}

CacheMeta slippygl::cache::CacheMeta::fromJsonString(const std::string& _json)
{
	if (_json.empty())
	{
		return CacheMeta();
	}

	json j = json::parse(_json, nullptr, false);
	if ((j.is_discarded()) || (!j.is_object()))
	{
		return CacheMeta();
	}

	CacheMeta meta;
	if ((j.contains("etag")) && (j["etag"].is_string()))
	{
		meta.setEtag(j["etag"].get<std::string>());
	}

	if ((j.contains("lastModified")) && (j["lastModified"].is_string()))
	{
		meta.setLastModified(j["lastModified"].get<std::string>());
	}

	if ((j.contains("contentType")) && (j["contentType"].is_string()))
	{
		meta.setContentType(j["contentType"].get<std::string>());
	}

	if ((j.contains("contentEncoding")) && (j["contentEncoding"].is_string()))
	{
		meta.setContentEncoding(j["contentEncoding"].get<std::string>());
	}

	if ((j.contains("contentLength")) && (j["contentLength"].is_number_unsigned()))
	{
		meta.setContentLength(j["contentLength"].get<std::uint64_t>());
	}

	if ((j.contains("lastAccessUnixSec")) && (j["lastAccessUnixSec"].is_number_unsigned()))
	{
		meta.touch(j["lastAccessUnixSec"].get<std::uint64_t>());
	}

	return meta;
}
