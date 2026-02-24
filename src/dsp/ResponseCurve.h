#pragma once

#include "../util/Params.h"
#include <juce_core/juce_core.h>
#include <vector>

namespace dsp {

class ResponseCurve {
  public:
    struct BandState {
        bool enabled = false;
        util::FilterType type = util::FilterType::Peak;
        float frequencyHz = 1000.0f;
        float gainDb = 0.0f;
        float q = 1.0f;
        util::Slope slope = util::Slope::Slope12dB;
    };

    struct State {
        std::array<BandState, util::Params::NumBands> bands{};
        float outputGainDb = 0.0f;
        double sampleRate = 44100.0;
    };

    [[nodiscard]] static State capture(const util::Params& params, double sampleRate,
                                       util::Bank bank = util::Bank::A) noexcept;
    [[nodiscard]] static std::vector<float> computeMagnitudeDb(const State& state,
                                                               const std::vector<double>& frequencies);
};

} // namespace dsp
