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

    void updateParameters(const util::Params& params);

    template <typename ProcessContext> void process(const ProcessContext& context) {
        for (auto& band : bands_)
            band.process(context);
    }

  private:
    std::array<EqBand, util::Params::NumBands> bands_;
    double sampleRate_ = 44100.0;
};
} // namespace dsp
