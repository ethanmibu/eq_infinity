#include "Params.h"

namespace util {
Params::Params(juce::AudioProcessor& processor) : apvts(processor, nullptr, "PARAMS", createLayout()) {
    // Cache raw parameter pointers for RT access (no string lookups in processBlock)
    outputGainDb_ = apvts.getRawParameterValue(IDs::outputGain);
    jassert(outputGainDb_ != nullptr);

    for (int i = 0; i < NumBands; ++i) {
        const int bandNum = i + 1;
        bands_[i].enabled = apvts.getRawParameterValue(IDs::enabled(bandNum));
        bands_[i].type = apvts.getRawParameterValue(IDs::type(bandNum));
        bands_[i].freq = apvts.getRawParameterValue(IDs::freq(bandNum));
        bands_[i].gain = apvts.getRawParameterValue(IDs::gain(bandNum));
        bands_[i].q = apvts.getRawParameterValue(IDs::q(bandNum));
        bands_[i].slope = apvts.getRawParameterValue(IDs::slope(bandNum));

        jassert(bands_[i].enabled != nullptr);
        jassert(bands_[i].type != nullptr);
        jassert(bands_[i].freq != nullptr);
        jassert(bands_[i].gain != nullptr);
        jassert(bands_[i].q != nullptr);
        jassert(bands_[i].slope != nullptr);
    }
}

float Params::getOutputGainDb() const noexcept {
    return outputGainDb_->load(std::memory_order_relaxed);
}

const Params::BandParams& Params::getBand(int index) const noexcept {
    jassert(index >= 0 && index < NumBands);
    return bands_[static_cast<std::size_t>(index)];
}

juce::AudioProcessorValueTreeState::ParameterLayout Params::createLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Output Gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::outputGain, 1), "Output Gain",
                                                                 juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
                                                                 0.0f));

    // Filter type choices
    juce::StringArray typeChoices{"Peak", "Low Shelf", "High Shelf", "High Pass", "Low Pass"};
    // Slope choices
    juce::StringArray slopeChoices{"12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct"};

    for (int i = 0; i < NumBands; ++i) {
        const int bandNum = i + 1;
        const juce::String prefix = "Band " + juce::String(bandNum) + " ";

        // Default to a sane state: only a couple enabled initially, or spaced out
        const bool defaultEnabled = (i == 0 || i == NumBands - 1);
        int defaultType = (i == 0) ? 3 : (i == NumBands - 1) ? 4 : 0; // HighPass, LowPass, Peak

        // Distribute default frequencies
        float defaultFreq = 1000.0f;
        if (i == 0)
            defaultFreq = 50.0f;
        else if (i == 1)
            defaultFreq = 125.0f;
        else if (i == 2)
            defaultFreq = 250.0f;
        else if (i == 3)
            defaultFreq = 500.0f;
        else if (i == 4)
            defaultFreq = 1000.0f;
        else if (i == 5)
            defaultFreq = 2000.0f;
        else if (i == 6)
            defaultFreq = 4000.0f;
        else if (i == 7)
            defaultFreq = 10000.0f;

        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(IDs::enabled(bandNum), 1),
                                                                    prefix + "Enabled", defaultEnabled));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::type(bandNum), 1),
                                                                      prefix + "Type", typeChoices, defaultType));

        // Frequency (logarithmic scale)
        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.1f);
        freqRange.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::freq(bandNum), 1),
                                                                     prefix + "Freq", freqRange, defaultFreq));

        // Gain
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::gain(bandNum), 1), prefix + "Gain",
                                                        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

        // Q (logarithmic scale)
        juce::NormalisableRange<float> qRange(0.1f, 18.0f, 0.01f);
        qRange.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::q(bandNum), 1),
                                                                     prefix + "Q", qRange,
                                                                     1.0f)); // Default Q: 1.0

        // Slope choice (0 = 12dB, 1 = 24dB, etc.)
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::slope(bandNum), 1),
                                                                      prefix + "Slope", slopeChoices,
                                                                      0)); // Default 12 dB/oct
    }

    return {params.begin(), params.end()};
}
} // namespace util
