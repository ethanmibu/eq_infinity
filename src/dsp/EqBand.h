#pragma once

#include "../util/Params.h"
#include <juce_dsp/juce_dsp.h>

namespace dsp {
class EqBand {
  public:
    EqBand();
    ~EqBand() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Call this before processing a block to apply updated parameters.
    void updateCoefficients(const util::Params::BandParams& params, double sampleRate);

    // Bypasses internal processing when the band is disabled.
    template <typename ProcessContext> void process(const ProcessContext& context) {
        if (!enabled_)
            return;

        for (auto& filter : filters_)
            filter.process(context);
    }

  private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using ArrayCoefficients = juce::dsp::IIR::ArrayCoefficients<float>;

    std::array<Filter, 4> filters_;

    bool enabled_ = false;

    // Smoothing interpolators
    juce::LinearSmoothedValue<float> smoothedFreq_{1000.0f};
    juce::LinearSmoothedValue<float> smoothedGain_{0.0f};
    juce::LinearSmoothedValue<float> smoothedQ_{1.0f};

    double sampleRate_ = 44100.0;
};
} // namespace dsp
