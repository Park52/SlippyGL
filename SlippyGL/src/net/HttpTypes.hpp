#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace slippygl::net {

using Bytes = std::vector<std::uint8_t>;

class NetConfig {
public:
    // getters
    const std::string& userAgent() const noexcept { return userAgent_; }
    long connectTimeoutMs() const noexcept { return connectTimeoutMs_; }
    long totalTimeoutMs()   const noexcept { return totalTimeoutMs_; }
    bool verifyTLS()        const noexcept { return verifyTLS_; }
    bool followRedirects()  const noexcept { return followRedirects_; }
    bool http2()            const noexcept { return http2_; }
    int  maxRetries()       const noexcept { return maxRetries_; }
    int  retryBackoffMs0()  const noexcept { return retryBackoffMs0_; }
    int  retryBackoffMs1()  const noexcept { return retryBackoffMs1_; }
    // fluent setters
    NetConfig& setUserAgent(std::string v) noexcept { userAgent_=std::move(v); return *this; }
    NetConfig& setConnectTimeoutMs(const long v) noexcept { connectTimeoutMs_=v; return *this; }
    NetConfig& setTotalTimeoutMs(const long v) noexcept { totalTimeoutMs_=v; return *this; }
    NetConfig& setVerifyTLS(const bool v) noexcept { verifyTLS_=v; return *this; }
    NetConfig& setFollowRedirects(const bool v) noexcept { followRedirects_=v; return *this; }
    NetConfig& setHttp2(const bool v) noexcept { http2_=v; return *this; }
    NetConfig& setMaxRetries(const int v) noexcept { maxRetries_=v; return *this; }
    NetConfig& setRetryBackoffMs0(const int v) noexcept { retryBackoffMs0_=v; return *this; }
    NetConfig& setRetryBackoffMs1(const int v) noexcept { retryBackoffMs1_=v; return *this; }
private:
    std::string userAgent_ = "SlippyGL/0.1 (+contact@example.com)";
    long connectTimeoutMs_ = 5000;
    long totalTimeoutMs_   = 10000;
    bool verifyTLS_        = true;
    bool followRedirects_  = true;
    bool http2_            = true;
    int  maxRetries_       = 2;
    int  retryBackoffMs0_  = 200;
    int  retryBackoffMs1_  = 500;
};

class RequestHeaders {
public:
    RequestHeaders& add(const std::string& key, const std::string& val);
    RequestHeaders& addRaw(const std::string& line);
    const std::vector<std::string>& items() const noexcept { return items_; }
    void clear() noexcept { items_.clear(); }
private:
    std::vector<std::string> items_;
};

class Conditional {
public:
    const std::optional<std::string>& ifNoneMatch() const noexcept { return ifNoneMatch_; }
    const std::optional<std::string>& ifModifiedSince() const noexcept { return ifModifiedSince_; }
    Conditional& setIfNoneMatch(std::string v) noexcept { ifNoneMatch_=std::move(v); return *this; }
    Conditional& clearIfNoneMatch() noexcept { ifNoneMatch_.reset(); return *this; }
    Conditional& setIfModifiedSince(std::string v) noexcept { ifModifiedSince_=std::move(v); return *this; }
    Conditional& clearIfModifiedSince() noexcept { ifModifiedSince_.reset(); return *this; }
private:
    std::optional<std::string> ifNoneMatch_;
    std::optional<std::string> ifModifiedSince_;
};

class ResponseHeaders {
public:
    const std::optional<std::string>& etag() const noexcept { return etag_; }
    const std::optional<std::string>& lastModified() const noexcept { return lastModified_; }
    const std::optional<std::string>& contentEncoding() const noexcept { return contentEncoding_; }
    const std::optional<std::string>& contentType() const noexcept { return contentType_; }
    const std::optional<long long>&   contentLength() const noexcept { return contentLength_; }
    const std::vector<std::string>&   raw() const noexcept { return raw_; }
    // setters
    void setEtag(std::optional<std::string> v) noexcept { etag_=std::move(v); }
    void setLastModified(std::optional<std::string> v) noexcept { lastModified_=std::move(v); }
    void setContentEncoding(std::optional<std::string> v) noexcept { contentEncoding_=std::move(v); }
    void setContentType(std::optional<std::string> v) noexcept { contentType_=std::move(v); }
    void setContentLength(const std::optional<long long> v) noexcept { contentLength_=v; }
    void addRaw(std::string line) { raw_.push_back(std::move(line)); }
private:
    std::optional<std::string> etag_, lastModified_, contentEncoding_, contentType_;
    std::optional<long long> contentLength_;
    std::vector<std::string> raw_;
};

class HttpResponse {
public:
    long status() const noexcept { return status_; }
    const Bytes& body() const noexcept { return body_; }
    const ResponseHeaders& headers() const noexcept { return headers_; }
    const std::string& effectiveUrl() const noexcept { return effectiveUrl_; }
    // internal setters
    void setStatus(const long s) noexcept { status_ = s; }
    Bytes& mutableBody() noexcept { return body_; }
    ResponseHeaders& mutableHeaders() noexcept { return headers_; }
    void setEffectiveUrl(std::string v) noexcept { effectiveUrl_ = std::move(v); }
private:
    long status_ = 0;
    Bytes body_;
    ResponseHeaders headers_;
    std::string effectiveUrl_;
};

} // namespace slippygl::net
