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

void EqEngine::updateParameters(const util::Params& params) {
    for (int i = 0; i < util::Params::NumBands; ++i) {
        bands_[static_cast<std::size_t>(i)].updateCoefficients(params.getBand(i), sampleRate_);
    }
}
} // namespace dsp
