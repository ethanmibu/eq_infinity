#pragma once

#include "../util/Params.h"
#include <JuceHeader.h>

namespace dsp {
class EqBand {
  public:
    EqBand();
    ~EqBand() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Call this before processing a block to apply updated parameters.
    void updateCoefficients(const util::Params::BandParams& params, double sampleRate);

    // Bypasses internal processing if `enabled` is false or if unity gain.
    template <typename ProcessContext> void process(const ProcessContext& context) {
        if (!enabled_)
            return;

        for (auto& filter : filters_)
            filter.process(context);
    }

  private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using FilterCoefficients = juce::dsp::IIR::Coefficients<float>;

    void updateFilterState(juce::dsp::IIR::Coefficients<float>::Ptr newCoefficients);

    std::array<Filter, 4> filters_;

    bool enabled_ = false;
    util::FilterType lastType_;

    // Smoothing interpolators
    juce::LinearSmoothedValue<float> smoothedFreq_{1000.0f};
    juce::LinearSmoothedValue<float> smoothedGain_{0.0f};
    juce::LinearSmoothedValue<float> smoothedQ_{1.0f};

    double sampleRate_ = 44100.0;
};
} // namespace dsp
