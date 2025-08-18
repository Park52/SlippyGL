#pragma once
#include "HttpTypes.hpp"
#include "CurlHandle.hpp"
#include <memory>

namespace slippygl::net 
{

class HttpClient 
{
public:
    explicit HttpClient(NetConfig cfg = {});
    ~HttpClient();

    const NetConfig& config() const noexcept;
    void setConfig(const NetConfig& cfg) noexcept;

    HttpResponse get(const std::string& url,
                     const RequestHeaders* optHeaders = nullptr,
                     const Conditional*    cond = nullptr);

private:
    class Impl;                    // PIMPL로 libcurl 의존 숨김
    std::unique_ptr<Impl> impl_;
};

} // namespace slippygl::net
