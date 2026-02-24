#pragma once

#include "../util/Params.h"
#include "EqBand.h"
#include <juce_dsp/juce_dsp.h>

namespace dsp {
class EqEngine {
  public:
    EqEngine() = default;
    ~EqEngine() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void updateParameters(const util::Params& params, util::Bank bank, int numSamples, double sampleRate);
    void setSoloBandIndex(int index) noexcept;

    template <typename ProcessContext> void process(const ProcessContext& context) {
        if (soloBandIndex_ >= 0 && soloBandIndex_ < util::Params::NumBands) {
            bands_[static_cast<std::size_t>(soloBandIndex_)].process(context);
            return;
        }

        for (auto& band : bands_)
            band.process(context);
    }

  private:
    std::array<EqBand, util::Params::NumBands> bands_;
    double sampleRate_ = 44100.0;
    int soloBandIndex_ = -1;
};
} // namespace dsp
