#include "HttpClient.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <cctype>

namespace slippygl::net 
{

class HttpClient::Impl 
{
public:
    explicit Impl(NetConfig c) : cfg_(std::move(c)) {}

    const NetConfig& cfg() const noexcept { return cfg_; }
    void setCfg(const NetConfig& c) noexcept { cfg_ = c; }

    HttpResponse doGet(const std::string& url,
                       const RequestHeaders* optHeaders,
                       const Conditional* cond) {
        CurlEasy easy;              // per request
        Bytes body;
        ResponseHeaders rhdr;

        // 기본 옵션
        curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
        curl_easy_setopt(easy, CURLOPT_USERAGENT, cfg_.userAgent().c_str());
        curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, cfg_.connectTimeoutMs());
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS,        cfg_.totalTimeoutMs());
        curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION,    cfg_.followRedirects() ? 1L : 0L);
        curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(easy, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        // 압축 자동 처리(gzip/deflate/br 가능한 경우)
#ifdef CURLOPT_ACCEPT_ENCODING
        curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
#endif

        // HTTP/2 시도
#ifdef CURLOPT_HTTP_VERSION
        if (cfg_.http2()) {
            curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
        }
#endif

        // TLS 검증
        curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, cfg_.verifyTLS() ? 1L : 0L);
        curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, cfg_.verifyTLS() ? 2L : 0L);

        // 바디 콜백
        curl_easy_setopt(
            easy, CURLOPT_WRITEFUNCTION,
            +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* out = static_cast<Bytes*>(userdata);
                size_t bytes = size * nmemb;
                out->insert(out->end(),
                            reinterpret_cast<std::uint8_t*>(ptr),
                            reinterpret_cast<std::uint8_t*>(ptr) + bytes);
                return bytes;
            }
        );
        curl_easy_setopt(easy, CURLOPT_WRITEDATA, &body);

        // 헤더 콜백
        curl_easy_setopt(
            easy, CURLOPT_HEADERFUNCTION,
            +[](char* buffer, size_t size, size_t nitems, void* userdata) -> size_t {
                auto* rh = static_cast<ResponseHeaders*>(userdata);
                size_t bytes = size * nitems;
                std::string line(buffer, buffer + bytes);
                rh->addRaw(line);

                // 간단 파서: "Key: Value"
                auto lower = [](std::string s){
                    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    return s;
                };
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string key = lower(line.substr(0, pos));
                    std::string val = line.substr(pos + 1);
                    // trim
                    while (!val.empty() && (val.front()==' '||val.front()=='\t')) val.erase(val.begin());
                    while (!val.empty() && (val.back()=='\r'||val.back()=='\n'||val.back()==' '||val.back()=='\t')) val.pop_back();

                    if (key == "etag") rh->setEtag(val);
                    else if (key == "last-modified") rh->setLastModified(val);
                    else if (key == "content-encoding") rh->setContentEncoding(val);
                    else if (key == "content-type") rh->setContentType(val);
                    else if (key == "content-length") {
                        try { rh->setContentLength(std::stoll(val)); } catch (...) {}
                    }
                }
                return bytes;
            }
        );
        curl_easy_setopt(easy, CURLOPT_HEADERDATA, &rhdr);

        // 요청 헤더 구성
        struct curl_slist* headers = nullptr;
        auto addHeader = [&](const std::string& h){ headers = curl_slist_append(headers, h.c_str()); };

        if (optHeaders) {
            for (const auto& h : optHeaders->items()) addHeader(h);
        }
        if (cond) {
            if (cond->ifNoneMatch().has_value())
                addHeader(std::string("If-None-Match: ") + *cond->ifNoneMatch());
            if (cond->ifModifiedSince().has_value())
                addHeader(std::string("If-Modified-Since: ") + *cond->ifModifiedSince());
        }
        if (headers) curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

        // 실행
        CURLcode rc = curl_easy_perform(easy);
        long status = 0;
        char* eff = nullptr;
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff);

        if (headers) curl_slist_free_all(headers);

        HttpResponse resp;
        resp.setStatus(rc == CURLE_OK ? status : 0);
        resp.mutableBody() = std::move(body);
        resp.mutableHeaders() = std::move(rhdr);
        if (eff) resp.setEffectiveUrl(eff);

        if (rc != CURLE_OK) {
            spdlog::warn("curl perform error: {} ({})", curl_easy_strerror(rc), static_cast<int>(rc));
        }
        return resp;
    }

    static void sleepMs(const int ms) noexcept {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

private:
    NetConfig  cfg_;
    CurlGlobal global_; // RAII: 프로세스 전역 초기화
};

// ==== HttpClient public API ====

HttpClient::HttpClient(NetConfig cfg)
: impl_(std::make_unique<Impl>(std::move(cfg))) {}

HttpClient::~HttpClient() = default;

const NetConfig& HttpClient::config() const noexcept { return impl_->cfg(); }
void HttpClient::setConfig(const NetConfig& cfg) noexcept { impl_->setCfg(cfg); }

HttpResponse HttpClient::get(const std::string& url,
                             const RequestHeaders* optHeaders,
                             const Conditional* cond)
{
    const auto& cfg = impl_->cfg();
    const int attempts = cfg.maxRetries() + 1;

    for (int i = 0; i < attempts; ++i) {
        auto resp = impl_->doGet(url, optHeaders, cond);

        // 재시도 조건: 네트워크 에러(status=0) 또는 5xx
        if ((resp.status() == 0) || ((resp.status() >= 500) && (resp.status() < 600))) {
            if (i < attempts - 1) 
            {
                const int32_t backoff = (i == 0) ? cfg.retryBackoffMs0() : cfg.retryBackoffMs1();
                spdlog::warn("GET retry {}/{} (status={}) {}", i+1, attempts-1, resp.status(), url);
                Impl::sleepMs(backoff);
                continue;
            }
        }
        return resp;
    }
    return {}; // not reached
}

} // namespace slippygl::net
