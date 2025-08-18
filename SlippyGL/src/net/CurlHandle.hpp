#pragma once
#include <curl/curl.h>
#include <stdexcept>

namespace slippygl::net 
{

class CurlGlobal 
{
public:
    CurlGlobal();
    ~CurlGlobal();
    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
};

class CurlEasy 
{
public:
    CurlEasy();
    ~CurlEasy();
    CurlEasy(const CurlEasy&) = delete;
    CurlEasy& operator=(const CurlEasy&) = delete;
    CurlEasy(CurlEasy&&) noexcept;
    CurlEasy& operator=(CurlEasy&&) noexcept;
    CURL* get() noexcept { return h_; }
    const CURL* get() const noexcept { return h_; }
    operator CURL*() noexcept { return h_; }
    operator const CURL*() const noexcept { return h_; }
private:
    CURL* h_ = nullptr;
};

} // namespace slippygl::net
