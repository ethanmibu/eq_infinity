#include "EqEngine.h"

namespace dsp {
void EqEngine::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate_ = spec.sampleRate;

    for (auto& band : bands_)
        band.prepare(spec);
}

void EqEngine::reset() {
    for (auto& band : bands_)
        band.reset();
}

void EqEngine::updateParameters(const util::Params& params, util::Bank bank, int numSamples, double sampleRate) {
    sampleRate_ = sampleRate;

    for (int i = 0; i < util::Params::NumBands; ++i) {
        bands_[static_cast<std::size_t>(i)].updateCoefficients(params.getBand(i, bank), sampleRate_, numSamples);
    }
}

void EqEngine::setSoloBandIndex(int index) noexcept {
    soloBandIndex_ = index;
}
} // namespace dsp
