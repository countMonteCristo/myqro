#include <format>
#include <iostream>

#include "encoder.hpp"


// =============================================================================

struct Args
{
    // int version;
    std::string msg;
    myqro::EncodingType encoding = myqro::EncodingType::BYTES;
    myqro::CorrectionLevel cl = myqro::CorrectionLevel::M;
    int mask_id = 0;
    std::string output = "out.ppm";

    void Init(int argc, char* argv[])
    {
        std::vector<std::string>args(argv, argv + argc);

        bool set_msg = false;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i][0] != '-')
            {
                if (set_msg) throw std::runtime_error("Can't process multiple messages at once");

                msg = args[i];
                set_msg = true;
                continue;
            }

            if (args[i] == "-h" || args[i] == "--help")
            {
                Usage(args[0]);
                exit(0);
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
                    throw std::runtime_error("--encoding option requires an argument. Possible values: num, alnum, bytes, kanji");
            }
            else if (args[i] == "-o" || args[i] == "--output")
            {
                if (i + 1 < args.size())
                    output = args[++i];
                else
                    throw std::runtime_error("--output option requires an argument.");
            }
            else if (args[i] == "-c" || args[i] == "--correction")
            {
                if (i + 1 < args.size())
                    cl = myqro::CorrectionLevelFromString(args[++i]);
                else
                    throw std::runtime_error("--correction option requires an argument. Possible values: L, M, Q, H");
            }
            else if (args[i] == "-m" || args[i] == "--mask")
            {
                if (i + 1 < args.size())
                {
                    mask_id = std::stoi(args[++i]);
                    if (mask_id < 0)
                        throw std::runtime_error("Automatic mask choosing is not implemented yet");
                }
                else
                {
                    Usage(args[0]);
                    throw std::runtime_error("--mask option requires an argument. Possible values are from int range [0-7]. "
                                             "Negative value means automatic choice.");
                }
            }
            else
                throw std::runtime_error(std::format("Unknown argument: {}", args[i]));
        }

        if (!set_msg)
        {
            Usage(args[0]);
            throw std::runtime_error("`message` was not provided");
        }
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
           << "  -o,--output <filename>   - output image (only PPM supported for now)." << std::endl
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

    myqro::Canvas canvas = myqro::Encoder::Encode(args.msg, args.cl, args.encoding, args.mask_id);
    canvas.DebugPBM(args.output);

    std::cout << "[DEBUG] Version: " << canvas.Version() << std::endl;
    canvas.DebugOutput(std::cout);
    std::cout << std::endl;
    // std::cout << "imprint: <" << canvas.Imprint() << ">" << std::endl;
    std::cout << "Generated image: " << args.output << std::endl;

    return 0;
}

// =============================================================================
