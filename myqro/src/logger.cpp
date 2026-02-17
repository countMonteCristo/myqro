#include "logger.hpp"

#include <iostream>
#include <sstream>

#include "error.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

void Logger::SetLogLevel(const std::string& l)
{
    if (l == "CRITICAL" || l == "critical")
    {
        SetLogLevel(LogLevel::CRITICAL);
        return;
    }
    if (l == "ERROR" || l == "error")
    {
        SetLogLevel(LogLevel::ERROR);
        return;
    }
    if (l == "WARNING" || l == "warning")
    {
        SetLogLevel(LogLevel::WARNING);
        return;
    }
    if (l == "DEBUG" || l == "debug")
    {
        SetLogLevel(LogLevel::DEBUG);
        return;
    }
    if (l == "INFO" || l == "info")
    {
        SetLogLevel(LogLevel::INFO);
        return;
    }
    if (l == "VOID" || l == "void")
    {
        SetLogLevel(LogLevel::VOID);
        return;
    }
    throw Error(std::format("Unknown log level: {}", l));
}

// =============================================================================

void Logger::LogImpl(LogLevel l, std::string_view msg)
{
    if (GetStream() == nullptr) return;
    std::ostream& stream = *GetStream();

    std::string_view level_str;
    switch (l)
    {
        case LogLevel::CRITICAL:    level_str = "CRITICAL"; break;
        case LogLevel::ERROR:       level_str = "ERROR"; break;
        case LogLevel::WARNING:     level_str = "WARNING"; break;
        case LogLevel::DEBUG:       level_str = "DEBUG"; break;
        case LogLevel::INFO:        level_str = "INFO"; break;
        case LogLevel::VOID:        return;
        default:                    level_str = "UNKNOWN"; break;
    }

    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::floor<std::chrono::milliseconds>(now);

    stream << std::format("[{:%F %T}] [{}] {}", now_ms, level_str, msg) << std::endl;
}

// =============================================================================

LogLevel& Logger::GetLevel()
{
    static LogLevel level = LogLevel::INFO;
    return level;
}

std::ostream*& Logger::GetStream()
{
    static std::ostream* out = &std::cout;
    return out;
}

// =============================================================================

void SetLogLevel(LogLevel l)
{
    Logger::SetLogLevel(l);
}

void SetLogLevel(const std::string& l)
{
    Logger::SetLogLevel(l);
}

LogLevel GetLogLevel()
{
    return Logger::GetLogLevel();
}

// =============================================================================

} // namespace myqro

// =============================================================================
