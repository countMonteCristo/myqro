#pragma once

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <limits>
#include <ostream>
#include <string>


// =============================================================================

namespace myqro
{

// =============================================================================

enum class LogLevel : size_t
{
    CRITICAL    = 0,
    ERROR       = 10,
    WARNING     = 20,
    DEBUG       = 30,
    INFO        = 40,
    VOID        = std::numeric_limits<size_t>::max(),
};

// =============================================================================

class Logger
{
public:
    Logger() = delete;

    template<typename... Args>
    static void Log(LogLevel l, std::format_string<Args...> format, Args&&... args)
    {
        if (IsLogLevelNotVisible(l)) return;
        const std::string msg = std::format(format, std::forward<Args>(args)...);
        LogImpl(l, msg);
    }

    static void Log(LogLevel l)
    {
        if (IsLogLevelNotVisible(l)) return;
        LogImpl(l, "");
    }

    static void SetLogLevel(LogLevel l) { GetLevel() = l; }
    static void SetLogLevel(const std::string& l);
    static LogLevel GetLogLevel() { return GetLevel(); }

    static void SetStream(std::ostream& s) { GetStream() = &s; }

private:
    static bool IsLogLevelNotVisible(LogLevel l) { return l < GetLevel(); };
    static void LogImpl(LogLevel l, std::string_view msg);

    static LogLevel& GetLevel();
    static std::ostream*& GetStream();
};

// =============================================================================

void SetLogLevel(LogLevel l);
void SetLogLevel(const std::string& l);
LogLevel GetLogLevel();

// =============================================================================

// =============================================================================

#define LogCritical(...) do { ::myqro::Logger::Log(::myqro::LogLevel::CRITICAL __VA_OPT__(, ) __VA_ARGS__); ::std::abort(); } while(0)
#define LogError(...)   ::myqro::Logger::Log(::myqro::LogLevel::ERROR   __VA_OPT__(, ) __VA_ARGS__)
#define LogWarning(...) ::myqro::Logger::Log(::myqro::LogLevel::WARNING __VA_OPT__(, ) __VA_ARGS__)
#define LogDebug(...)   ::myqro::Logger::Log(::myqro::LogLevel::DEBUG   __VA_OPT__(, ) __VA_ARGS__)
#define LogInfo(...)    ::myqro::Logger::Log(::myqro::LogLevel::INFO    __VA_OPT__(, ) __VA_ARGS__)

// =============================================================================

} // namespace myqro

// =============================================================================
