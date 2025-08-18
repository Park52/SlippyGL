#include "HttpTypes.hpp"

namespace slippygl::net 
{

RequestHeaders& RequestHeaders::add(const std::string& key, const std::string& val) 
{
    items_.push_back(key + ": " + val);
    return *this;
}

RequestHeaders& RequestHeaders::addRaw(const std::string& line) 
{
    items_.push_back(line);
    return *this;
}

} // namespace slippygl::net
