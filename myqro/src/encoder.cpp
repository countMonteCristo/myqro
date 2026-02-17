#include "encoder.hpp"

#include <limits>

#include "logger.hpp"


// =============================================================================

namespace myqro
{

// =============================================================================

Canvas Encoder::Encode(const std::string& msg, CorrectionLevel cl,
                       EncodingType encoding, int mask_id)
{
    EncodeProviderPtr provider = EncodeProviderFactory::GetProvider(encoding);

    Context ctx = provider->Encode(msg, cl);
    Canvas canvas = Canvas(ctx.version);

    canvas.SetupSearchPatterns();
    canvas.SetupLevelingPatterns();
    canvas.SetupSyncLines();
    canvas.SetupVersionCode();

    if (mask_id < 0)
    {
        LogDebug("Choosing best mask");
        return FindBestMask(canvas, ctx);
    }

    canvas.FillData(ctx.cl, mask_id, DataStream(std::move(ctx.output)));
    return canvas;
}

Canvas Encoder::FindBestMask(Canvas& c, const Context& ctx)
{
    std::vector<Canvas> tries(MASK_ARRAY_SIZE, c);
    size_t max_penalty = std::numeric_limits<size_t>::max();
    size_t idx = MIN_MASK_ID;
    for (size_t mask_id = MIN_MASK_ID; mask_id <= MAX_MASK_ID; ++mask_id)
    {
        tries[mask_id].FillData(ctx.cl, mask_id, DataStream(ctx.output));
        size_t penalty = tries[mask_id].Penalty(mask_id);
        if (penalty < max_penalty)
        {
            max_penalty = penalty;
            idx = mask_id;
        }
    }
    LogDebug("Choose best mask: {}, penalty={}", idx, max_penalty);
    return tries[idx];
}

// =============================================================================

} // namespace myqro

// =============================================================================
