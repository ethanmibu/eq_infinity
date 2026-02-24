#include "EqBand.h"

namespace dsp {
EqBand::EqBand() {}

void EqBand::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate_ = spec.sampleRate;

    for (auto& filter : filters_)
        filter.prepare(spec);

    smoothedFreq_.reset(sampleRate_, 0.05);
    smoothedGain_.reset(sampleRate_, 0.05);
    smoothedQ_.reset(sampleRate_, 0.05);
}

void EqBand::reset() {
    for (auto& filter : filters_)
        filter.reset();
}

void EqBand::updateCoefficients(const util::Params::BandParams& params, double sampleRate, int numSamples) {
    sampleRate_ = sampleRate;

    // We expect params pointers to be valid.
    const bool isEnabled = params.enabled->load(std::memory_order_relaxed) > 0.5f;
    enabled_ = isEnabled;

    const auto type = static_cast<util::FilterType>(static_cast<int>(params.type->load(std::memory_order_relaxed)));
    const float freq = params.freq->load(std::memory_order_relaxed);
    const float gain = params.gain->load(std::memory_order_relaxed);
    const float q = params.q->load(std::memory_order_relaxed);
    const auto slope = static_cast<util::Slope>(static_cast<int>(params.slope->load(std::memory_order_relaxed)));

    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate_ * 0.495, 20000.0));
    const float clampedFreq = juce::jlimit(20.0f, maxFrequency, freq);
    const float clampedQ = juce::jlimit(0.1f, 18.0f, q);

    smoothedFreq_.setTargetValue(clampedFreq);
    smoothedGain_.setTargetValue(gain);
    smoothedQ_.setTargetValue(clampedQ);

    // Advance smoothing by the block length so behavior is block-size independent.
    const int samplesToAdvance = juce::jmax(numSamples, 0);
    const float currentFreq = samplesToAdvance > 0 ? smoothedFreq_.skip(samplesToAdvance) : smoothedFreq_.getCurrentValue();
    const float currentGain = juce::Decibels::decibelsToGain(samplesToAdvance > 0 ? smoothedGain_.skip(samplesToAdvance)
                                                                                   : smoothedGain_.getCurrentValue());
    const float currentQ = samplesToAdvance > 0 ? smoothedQ_.skip(samplesToAdvance) : smoothedQ_.getCurrentValue();

    if (!enabled_)
        return;

    std::array<float, 6> coeffs{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};

    switch (type) {
    case util::FilterType::Peak:
        coeffs = ArrayCoefficients::makePeakFilter(sampleRate_, currentFreq, currentQ, currentGain);
        break;
    case util::FilterType::LowShelf:
        coeffs = ArrayCoefficients::makeLowShelf(sampleRate_, currentFreq, currentQ, currentGain);
        break;
    case util::FilterType::HighShelf:
        coeffs = ArrayCoefficients::makeHighShelf(sampleRate_, currentFreq, currentQ, currentGain);
        break;
    case util::FilterType::HighPass:
        coeffs = ArrayCoefficients::makeHighPass(sampleRate_, currentFreq, currentQ);
        break;
    case util::FilterType::LowPass:
        coeffs = ArrayCoefficients::makeLowPass(sampleRate_, currentFreq, currentQ);
        break;
    }

    // For LowPass/HighPass, slope determines how many biquads we cascade
    // 12dB/oct = 1 biquad, 24dB/oct = 2 biquads, etc.
    int numFiltersToUse = 1;

    if (type == util::FilterType::LowPass || type == util::FilterType::HighPass) {
        switch (slope) {
        case util::Slope::Slope12dB:
            numFiltersToUse = 1;
            break;
        case util::Slope::Slope24dB:
            numFiltersToUse = 2;
            break;
        case util::Slope::Slope36dB:
            numFiltersToUse = 3;
            break;
        case util::Slope::Slope48dB:
            numFiltersToUse = 4;
            break;
        }
    }

    const std::array<float, 6> bypassCoeffs{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};

    for (int i = 0; i < numFiltersToUse; ++i) {
        *(filters_[static_cast<std::size_t>(i)].state) = coeffs;
    }

    for (int i = numFiltersToUse; i < 4; ++i) {
        *(filters_[static_cast<std::size_t>(i)].state) = bypassCoeffs;
    }
}
} // namespace dsp
