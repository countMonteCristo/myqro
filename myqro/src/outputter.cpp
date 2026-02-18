#include "outputter.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

void ConsoleOutputter::OutputImpl(const Canvas& canvas, const OutputOptions& options)
{
    int size = (static_cast<int>(canvas.Size()) + 2*options.indent) * options.scale;

    for (int row = 0; row < size; row++)
    {
        int r = row / options.scale;
        for (int col = 0; col < size; col++)
        {
            int c = col / options.scale;
            if (canvas.IsInside(r - options.indent, c - options.indent))
            {
                const Cell& cell = canvas.At(r - options.indent, c - options.indent);
                stream_ << " #"[cell.value];
            }
            else
            {
                stream_ << ' ';
            }
        }
        stream_ << std::endl;
    }
}

// =============================================================================

void ImprintOutputter::OutputImpl(const Canvas& canvas, const OutputOptions& options)
{
    UNUSED(options);
    for (size_t row = 0; row < canvas.Size(); row++)
    {
        for (size_t col = 0; col < canvas.Size(); col++)
            stream_ << " #"[canvas.At(row, col).value];
    }
}

// =============================================================================

void PBMOutputter::OutputImpl(const Canvas& canvas, const OutputOptions& options)
{
    int size = (static_cast<int>(canvas.Size()) + 2*options.indent) * options.scale;

    Stream() << "P1" << std::endl;
    Stream() << size << " " << size << std::endl;

    for (int row = 0; row < size; row++)
    {
        int r = row / options.scale;
        for (int col = 0; col < size; col++)
        {
            int c = col / options.scale;
            if (canvas.IsInside(r - options.indent, c - options.indent))
            {
                const Cell& cell = canvas.At(r - options.indent, c - options.indent);
                Stream() << "01"[cell.value];
            }
            else
            {
                Stream() << '0';
            }
        }
        Stream() << std::endl;
    }
}

// =============================================================================

void SvgOutputter::OutputImpl(const Canvas& canvas, const OutputOptions& options)
{
    int size = (static_cast<int>(canvas.Size()) + 2*options.indent);

    Stream() << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
             << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
             << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << std::endl
             << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 "
             << size << " " << size << "\" stroke=\"none\">" << std::endl
             << "<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>" << std::endl
             << "<path d=\"";
    for (size_t row = 0; row < canvas.Size(); row++)
    {
        for (size_t col = 0; col < canvas.Size(); col++)
        {
            const Cell& cell = canvas.At(row, col);
            if (cell.value == WHITE)
                continue;

            Stream() << "M" << row + options.indent << ',' << col + options.indent << "h1v1h-1z ";
        }
    }
    Stream() << "\" fill=\"#000000\"/></svg>";
}

// =============================================================================

void EpsOutputter::OutputImpl(const Canvas& canvas, const OutputOptions& options)
{
    int llx, lly, urx, ury;
    llx = lly = -options.indent;
    urx = ury = (static_cast<int>(canvas.Size()) + options.indent);

    Stream() << "%!PS-Adobe-3.0 EPSF-3.0" << std::endl
             << "%%BoundingBox: " << llx << ' ' << lly
                                  << ' ' << urx << ' ' << ury << std::endl
             << "%%Title: QR-code generated using myqro library" << std::endl
             << "%%EndComments" << std::endl;

    Stream() << "1.0 1.0 1.0 setrgbcolor" << std::endl
             << llx << ' ' << lly << ' ' << urx << ' ' << ury << " rectfill" << std::endl;

    Stream() << "0.0 0.0 0.0 setrgbcolor" << std::endl;
    for (size_t row = 0; row < canvas.Size(); row++)
    {
        for (size_t col = 0; col < canvas.Size(); col++)
        {
            const Cell& cell = canvas.At(row, col);
            if (cell.value == WHITE)
                continue;

            Stream() <<  col << ' ' << canvas.Size() - row << ' ' << "1 1 rectfill" << std::endl;
        }
    }

    Stream() << "%%EOF" << std::endl;
}

// =============================================================================

} // namespace myqro

// =============================================================================
