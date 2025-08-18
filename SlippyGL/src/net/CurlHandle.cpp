#include "CurlHandle.hpp"

namespace slippygl::net 
{

CurlGlobal::CurlGlobal() 
{
    auto rc = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (rc != 0) 
    {
        throw std::runtime_error("curl_global_init failed");
    }
}
CurlGlobal::~CurlGlobal() { curl_global_cleanup(); }

CurlEasy::CurlEasy() : h_(curl_easy_init()) 
{
    if (!h_) 
    {
        throw std::runtime_error("curl_easy_init failed");
    }
}
CurlEasy::~CurlEasy() 
{
    if (h_) 
    {
        curl_easy_cleanup(h_);
        h_ = nullptr;
    }
}

CurlEasy::CurlEasy(CurlEasy&& o) noexcept : h_(o.h_) 
{
    o.h_ = nullptr;
}

CurlEasy& CurlEasy::operator=(CurlEasy&& o) noexcept 
{
    if (this != &o) 
    {
        if (h_) 
        {
            curl_easy_cleanup(h_);
            h_ = nullptr;
        }
        h_ = o.h_;
        o.h_ = nullptr;
    }
    return *this;
}

} // namespace slippygl::net
