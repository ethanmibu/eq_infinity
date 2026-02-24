#include "ResponseCurve.h"
#include <cmath>
#include <complex>
#include <juce_dsp/juce_dsp.h>

namespace dsp {
namespace {

using ArrayCoefficients = juce::dsp::IIR::ArrayCoefficients<float>;

std::array<float, 6> makeCoefficients(util::FilterType type, double sampleRate, float frequencyHz, float q,
                                      float gainLinear) {
    switch (type) {
    case util::FilterType::Peak:
        return ArrayCoefficients::makePeakFilter(sampleRate, frequencyHz, q, gainLinear);
    case util::FilterType::LowShelf:
        return ArrayCoefficients::makeLowShelf(sampleRate, frequencyHz, q, gainLinear);
    case util::FilterType::HighShelf:
        return ArrayCoefficients::makeHighShelf(sampleRate, frequencyHz, q, gainLinear);
    case util::FilterType::HighPass:
        return ArrayCoefficients::makeHighPass(sampleRate, frequencyHz, q);
    case util::FilterType::LowPass:
        return ArrayCoefficients::makeLowPass(sampleRate, frequencyHz, q);
    }

    return {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
}

int slopeStageCount(util::Slope slope) noexcept {
    switch (slope) {
    case util::Slope::Slope12dB:
        return 1;
    case util::Slope::Slope24dB:
        return 2;
    case util::Slope::Slope36dB:
        return 3;
    case util::Slope::Slope48dB:
        return 4;
    }

    return 1;
}

double biquadMagnitude(const std::array<float, 6>& coeffs, double omega) {
    const std::complex<double> z1 = std::exp(std::complex<double>(0.0, -omega));
    const std::complex<double> z2 = z1 * z1;
    const std::complex<double> numerator =
        static_cast<double>(coeffs[0]) + static_cast<double>(coeffs[1]) * z1 + static_cast<double>(coeffs[2]) * z2;
    const std::complex<double> denominator =
        static_cast<double>(coeffs[3]) + static_cast<double>(coeffs[4]) * z1 + static_cast<double>(coeffs[5]) * z2;
    const double denominatorMagnitude = std::max(std::abs(denominator), 1.0e-12);
    return std::abs(numerator) / denominatorMagnitude;
}

} // namespace

ResponseCurve::State ResponseCurve::capture(const util::Params& params, double sampleRate, util::Bank bank) noexcept {
    State state;
    state.sampleRate = sampleRate;
    state.outputGainDb = params.getOutputGainDb();

    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate * 0.495, 20000.0));

    for (int i = 0; i < util::Params::NumBands; ++i) {
        const auto& band = params.getBand(i, bank);
        auto& bandState = state.bands[static_cast<std::size_t>(i)];
        bandState.enabled = band.enabled->load(std::memory_order_relaxed) > 0.5f;
        bandState.type = static_cast<util::FilterType>(static_cast<int>(band.type->load(std::memory_order_relaxed)));
        bandState.frequencyHz = juce::jlimit(20.0f, maxFrequency, band.freq->load(std::memory_order_relaxed));
        bandState.gainDb = juce::jlimit(-24.0f, 24.0f, band.gain->load(std::memory_order_relaxed));
        bandState.q = juce::jlimit(0.1f, 18.0f, band.q->load(std::memory_order_relaxed));
        bandState.slope = static_cast<util::Slope>(static_cast<int>(band.slope->load(std::memory_order_relaxed)));
    }

    return state;
}

std::vector<float> ResponseCurve::computeMagnitudeDb(const State& state, const std::vector<double>& frequencies) {
    std::vector<float> magnitudeDb(frequencies.size(), 0.0f);

    if (state.sampleRate <= 0.0)
        return magnitudeDb;

    const double twoPiOverSampleRate = juce::MathConstants<double>::twoPi / state.sampleRate;

    for (std::size_t i = 0; i < frequencies.size(); ++i) {
        const double frequency = frequencies[i];
        const double omega = frequency * twoPiOverSampleRate;
        double totalMagnitude = juce::Decibels::decibelsToGain(static_cast<double>(state.outputGainDb));

        for (const auto& band : state.bands) {
            if (!band.enabled)
                continue;

            const float gainLinear = juce::Decibels::decibelsToGain(band.gainDb);
            const auto coeffs = makeCoefficients(band.type, state.sampleRate, band.frequencyHz, band.q, gainLinear);

            const bool isCutFilter = band.type == util::FilterType::HighPass || band.type == util::FilterType::LowPass;
            const int stages = isCutFilter ? slopeStageCount(band.slope) : 1;
            const double stageMagnitude = biquadMagnitude(coeffs, omega);

            totalMagnitude *= std::pow(stageMagnitude, static_cast<double>(stages));
        }

        const float db = juce::Decibels::gainToDecibels(static_cast<float>(totalMagnitude), -48.0f);
        magnitudeDb[i] = juce::jlimit(-48.0f, 24.0f, db);
    }

    return magnitudeDb;
}

} // namespace dsp
