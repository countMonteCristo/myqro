#pragma once

#include <exception>
#include <string>


// =============================================================================

namespace myqro
{

// =============================================================================

class Error : public std::exception
{
    public:
    template<typename... Args>
    Error(const std::string& msg) :
        message_(msg)
    {}

    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

// =============================================================================

} // namespace myqro

// =============================================================================
