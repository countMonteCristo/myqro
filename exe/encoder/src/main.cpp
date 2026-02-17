#include <format>
#include <iostream>

#include "encoder.hpp"
#include "logger.hpp"
#include "outputter.hpp"


// =============================================================================

template<typename... Args>
void ExitWithErrorMessage(std::format_string<Args&&...> format, Args&&... args)
{
    std::cerr << std::format(format, std::forward<Args>(args)...) << std::endl;
    std::exit(1);
}

// =============================================================================

struct Args
{
    // int version;
    std::string msg;
    myqro::EncodingType encoding = myqro::EncodingType::BYTES;
    myqro::CorrectionLevel cl = myqro::CorrectionLevel::M;
    int mask_id = 0;
    int scale = 1;
    int indent = 4;
    std::string output = "out.ppm";
    std::string log_level_str = "info";

    void Init(int argc, char* argv[])
    {
        std::vector<std::string>args(argv, argv + argc);

        bool set_msg = false;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i][0] != '-')
            {
                if (set_msg)
                    ExitWithErrorMessage("Can't process multiple messages at once");

                msg = args[i];
                set_msg = true;
                continue;
            }

            if (args[i] == "-h" || args[i] == "--help")
            {
                Usage(args[0]);
                std::exit(0);
            }
            // else if (args[i] == "-v" || args[i] == "--version")
            // {
            //     if (i + 1 < args.size())
            //     {
            //         version = std::stoi(args[++i]);
            //         if (version < 0)
            //             throw std::runtime_error();
            //     } else
            //     {
            //         std::cerr << "Error: --output option requires an argument." << std::endl;
            //     }
            // }
            else if (args[i] == "-e" || args[i] == "--encoding")
            {
                if (i + 1 < args.size())
                    encoding = myqro::EncodingTypeFromString(args[++i]);
                else
                    ExitWithErrorMessage("--encoding option requires an argument. Possible values: num, alnum, bytes, kanji");
            }
            else if (args[i] == "-o" || args[i] == "--output")
            {
                if (i + 1 < args.size())
                    output = args[++i];
                else
                    ExitWithErrorMessage("--output option requires an argument.");
            }
            else if (args[i] == "-c" || args[i] == "--correction")
            {
                if (i + 1 < args.size())
                    cl = myqro::CorrectionLevelFromString(args[++i]);
                else
                    ExitWithErrorMessage("--correction option requires an argument. Possible values: L, M, Q, H");
            }
            else if (args[i] == "-m" || args[i] == "--mask")
            {
                if (i + 1 < args.size())
                    mask_id = std::stoi(args[++i]);
                else
                    ExitWithErrorMessage("--mask option requires an argument. Possible values are from int range [0-7]. "
                                         "Negative value means automatic choice.");
            }
            else if (args[i] == "-l" || args[i] == "--log-level")
            {
                if (i + 1 < args.size())
                    log_level_str = args[++i];
                else
                    ExitWithErrorMessage("--log-level option requires an argument.");
            }
            else if (args[i] == "-s" || args[i] == "--scale")
            {
                if (i + 1 < args.size())
                    scale = std::stoi(args[++i]);
                else
                    ExitWithErrorMessage("--scale option requires an argument.");
            }
            else if (args[i] == "-i" || args[i] == "--indent")
            {
                if (i + 1 < args.size())
                    indent = std::stoi(args[++i]);
                else
                    ExitWithErrorMessage("--indent option requires an argument.");
            }
            else
                ExitWithErrorMessage("Unknown argument: {}", args[i]);
        }

        if (!set_msg)
        {
            Usage(args[0]);
            ExitWithErrorMessage("`message` was not provided");
        }

        Validate();
    }

    void Validate() const
    {
        if (mask_id >= static_cast<int>(myqro::MAX_MASK_ID))
            ExitWithErrorMessage("`mask_id` shoud be negative or in range [{}, {}]",
                                 myqro::MIN_MASK_ID, myqro::MAX_MASK_ID);

        if (scale < 1)
            ExitWithErrorMessage("`scale` must be > 1");

        if (indent < 0)
            ExitWithErrorMessage("`indent` must be > 0");
    }

    void Usage(const std::string& program)
    {
        std::ostream& os = std::cout;
        os << "Usage: " << program << " [-h|--help]: show help and exit" << std::endl
           << "       " << program << " {flags} <message> {flags}: encode message into QR-code" << std::endl
           << std::endl
           << "Flags:" << std::endl
           << "  -e,--encoding <encoding> - type of encoding. Must be one of `num`, `alnum`, `bytes` or `kanji`." << std::endl
           << "  -c,--correction <cl>     - correction level. Defines how much errors can be fixed via decoding." << std::endl
           << "                             Must be one of `L` (7%), `M` (15%), `Q` (25%), `H` (30%)" << std::endl
           << "  -m,--mask <mask_id>      - identificator of mask function. Negative value means choosing the best mask." << std::endl
           << "                             Integer value from range [0; 7] identify specific function." << std::endl
           << "  -o,--output <filename>   - output image (supported formats: ppm, svg, console)." << std::endl
           << "  -s,--scale <int>         - scaling factor for output image (default 1)" << std::endl
           << "  -i,--indent <int>        - indentation for output QR code (default 4)"
           << "  -l,--log-level <level>   - set logging level. Must be one of `critical`, `error`, `warning`, `debug`, `info` or `void`" << std::endl
           << std::endl
           << "Required arguments:" << std::endl
           << "  message - message to encode" << std::endl;
    }
};

// =============================================================================

int main(int argc, char* argv[])
{
    Args args;

    args.Init(argc, argv);
    myqro::SetLogLevel(args.log_level_str);

    std::unique_ptr<myqro::Outputter> outputter;
    if (args.output == "console")
        outputter = std::make_unique<myqro::ConsoleOutputter>(std::cout);
    else
    {
        std::filesystem::path path(args.output);
        std::string ext = path.extension();
        if (ext == ".ppm" || ext == ".PPM")
            outputter = std::make_unique<myqro::PBMOutputter>(path);
        else if (ext == ".svg" || ext == ".SVG")
            outputter = std::make_unique<myqro::SvgOutputter>(path);
        else
            ExitWithErrorMessage("Unsupported output format: {}", ext);
    }

    myqro::Canvas canvas = myqro::Encoder::Encode(args.msg, args.cl, args.encoding, args.mask_id);
    LogDebug("Version: {}", canvas.Version());

    outputter->Output(canvas, myqro::OutputOptions(args.scale, args.indent));

    return 0;
}

// =============================================================================
