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

void EqBand::updateCoefficients(const util::Params::BandParams& params, double sampleRate) {
    sampleRate_ = sampleRate;

    // We expect params pointers to be valid.
    const bool isEnabled = params.enabled->load(std::memory_order_relaxed) > 0.5f;
    enabled_ = isEnabled;

    if (!enabled_)
        return;

    const auto type = static_cast<util::FilterType>(static_cast<int>(params.type->load(std::memory_order_relaxed)));
    const float freq = params.freq->load(std::memory_order_relaxed);
    const float gain = params.gain->load(std::memory_order_relaxed);
    const float q = params.q->load(std::memory_order_relaxed);
    const auto slope = static_cast<util::Slope>(static_cast<int>(params.slope->load(std::memory_order_relaxed)));

    smoothedFreq_.setTargetValue(freq);
    smoothedGain_.setTargetValue(gain);
    smoothedQ_.setTargetValue(q);

    // Get current interpolated values
    const float currentFreq = smoothedFreq_.getNextValue();
    const float currentGain = juce::Decibels::decibelsToGain(smoothedGain_.getNextValue());
    const float currentQ = smoothedQ_.getNextValue();

    FilterCoefficients::Ptr coeffs;

    switch (type) {
    case util::FilterType::Peak:
        coeffs = FilterCoefficients::makePeakFilter(sampleRate_, currentFreq, currentQ, currentGain);
        break;
    case util::FilterType::LowShelf:
        coeffs = FilterCoefficients::makeLowShelf(sampleRate_, currentFreq, currentQ, currentGain);
        break;
    case util::FilterType::HighShelf:
        coeffs = FilterCoefficients::makeHighShelf(sampleRate_, currentFreq, currentQ, currentGain);
        break;
    case util::FilterType::HighPass:
        coeffs = FilterCoefficients::makeHighPass(sampleRate_, currentFreq, currentQ);
        break;
    case util::FilterType::LowPass:
        coeffs = FilterCoefficients::makeLowPass(sampleRate_, currentFreq, currentQ);
        break;
    }

    if (coeffs != nullptr) {
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

        // Apply coefficient to required number of filters
        for (int i = 0; i < numFiltersToUse; ++i) {
            *(filters_[static_cast<std::size_t>(i)].coefficients) = *coeffs;
        }

        // Bypass remaining unused filters by setting unity gain coefficients
        FilterCoefficients::Ptr bypassCoeffs =
            FilterCoefficients::makeFirstOrderLowPass(sampleRate_, sampleRate_ / 2.0f);
        bypassCoeffs->coefficients.clear();
        bypassCoeffs->coefficients.set(0, 1.0f); // b0
        bypassCoeffs->coefficients.set(1, 0.0f); // b1
        bypassCoeffs->coefficients.set(2, 0.0f); // b2
        bypassCoeffs->coefficients.set(3, 1.0f); // a0
        bypassCoeffs->coefficients.set(4, 0.0f); // a1
        bypassCoeffs->coefficients.set(5, 0.0f); // a2

        for (int i = numFiltersToUse; i < 4; ++i) {
            *(filters_[static_cast<std::size_t>(i)].coefficients) = *bypassCoeffs;
        }
    }
}
} // namespace dsp
