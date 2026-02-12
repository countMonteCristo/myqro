#include "encoder.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

Canvas Encoder::Encode(const std::string& msg, CorrectionLevel cl,
                       EncodingType encoding, int mask_id)
{
    if (mask_id < 0)
        throw std::runtime_error("Automatic mask choice is not implemented yet");

    EncodeProviderPtr provider = myqro::EncodeProviderFactory::GetProvider(encoding);

    Context ctx = provider->Encode(msg, cl);
    Canvas canvas = myqro::Canvas(ctx.version);

    canvas.SetupSearchPatterns();
    canvas.SetupLevelingPatterns();
    canvas.SetupSyncLines();
    canvas.SetupVersionCode();

    canvas.FillData(ctx.cl, mask_id, myqro::DataStream(std::move(ctx.output)));

    return canvas;
}

// =============================================================================

} // namespace myqro

// =============================================================================
