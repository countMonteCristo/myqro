#pragma once

#include <filesystem>
#include <format>
#include <fstream>

#include "error.hpp"
#include "canvas.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

struct OutputOptions
{
    OutputOptions(int scale, int indent) :
        scale(scale),
        indent(indent)
    {}

    OutputOptions() = default;

    int scale = 1;
    int indent = 4;
};

// =============================================================================

class Outputter
{
public:
    Outputter() {}
    virtual ~Outputter() {}

    void Output(const Canvas& canvas, const OutputOptions& options = OutputOptions())
    {
        OutputImpl(canvas, options);
    }

private:
    virtual void OutputImpl(const Canvas& canvas, const OutputOptions& options) = 0;
};

// =============================================================================

class FileOutputter : public Outputter
{
public:
    FileOutputter(const std::filesystem::path& path) :
        Outputter(),
        path_(path),
        stream_(path)
    {
        if (!stream_.is_open())
            throw Error(std::format("Error opening file {}", path_.string()));
    }

    virtual ~FileOutputter()
    {
        if (stream_.is_open()) stream_.close();
    }

    std::ofstream& Stream() { return stream_; }
    const std::filesystem::path& Path() const { return path_; }

private:
    std::filesystem::path path_;
    std::ofstream stream_;
};

// =============================================================================

class ConsoleOutputter : public Outputter
{
public:
    ConsoleOutputter(std::ostream& os) :
        Outputter(),
        stream_(os)
    {}

    void OutputImpl(const Canvas& canvas, const OutputOptions& options) final;
private:
    std::ostream& stream_;
};

// =============================================================================

class ImprintOutputter : public Outputter
{
    public:
    ImprintOutputter(std::ostream& os) :
        Outputter(),
        stream_(os)
    {}

    void OutputImpl(const Canvas& canvas, const OutputOptions& options) final;
private:
    std::ostream& stream_;
};

// =============================================================================

class PBMOutputter : public FileOutputter
{
public:
    PBMOutputter(const std::filesystem::path& path) :
        FileOutputter(path)
    {}

    void OutputImpl(const Canvas& canvas, const OutputOptions& options) final;
};

// =============================================================================

class SvgOutputter : public FileOutputter
{
public:
    SvgOutputter(const std::filesystem::path& path) :
        FileOutputter(path)
    {}

    void OutputImpl(const Canvas& canvas, const OutputOptions& options) final;
};

// =============================================================================

class EpsOutputter : public FileOutputter
{
public:
    EpsOutputter(const std::filesystem::path& path) :
        FileOutputter(path)
    {}

    void OutputImpl(const Canvas& canvas, const OutputOptions& options) final;
};

// =============================================================================

} // namespace myqro

// =============================================================================
