#include "Params.h"

namespace util {
namespace {

juce::String bandPrefixWithBank(int bandNum, Bank bank) {
    const char bankChar = bank == Bank::A ? 'a' : 'b';
    return "b" + juce::String(bandNum) + "_" + juce::String::charToString(bankChar) + "_";
}

} // namespace

juce::String Params::IDs::enabled(int bandNum, Bank bank) {
    return bandPrefixWithBank(bandNum, bank) + "enabled";
}

juce::String Params::IDs::type(int bandNum, Bank bank) {
    return bandPrefixWithBank(bandNum, bank) + "type";
}

juce::String Params::IDs::freq(int bandNum, Bank bank) {
    return bandPrefixWithBank(bandNum, bank) + "freq";
}

juce::String Params::IDs::gain(int bandNum, Bank bank) {
    return bandPrefixWithBank(bandNum, bank) + "gain";
}

juce::String Params::IDs::q(int bandNum, Bank bank) {
    return bandPrefixWithBank(bandNum, bank) + "q";
}

juce::String Params::IDs::slope(int bandNum, Bank bank) {
    return bandPrefixWithBank(bandNum, bank) + "slope";
}

Params::Params(juce::AudioProcessor& processor) : apvts(processor, nullptr, "PARAMS", createLayout()) {
    // Cache raw parameter pointers for RT access (no string lookups in processBlock)
    editTarget_ = apvts.getRawParameterValue(IDs::editTarget);
    stereoMode_ = apvts.getRawParameterValue(IDs::stereoMode);
    hqMode_ = apvts.getRawParameterValue(IDs::hqMode);
    outputGainDb_ = apvts.getRawParameterValue(IDs::outputGain);
    jassert(editTarget_ != nullptr);
    jassert(stereoMode_ != nullptr);
    jassert(hqMode_ != nullptr);
    jassert(outputGainDb_ != nullptr);

    auto cacheBandPointers = [this](std::array<BandParams, NumBands>& destination, Bank bank) {
        for (int i = 0; i < NumBands; ++i) {
            const int bandNum = i + 1;
            const auto index = static_cast<std::size_t>(i);
            destination[index].enabled = apvts.getRawParameterValue(IDs::enabled(bandNum, bank));
            destination[index].type = apvts.getRawParameterValue(IDs::type(bandNum, bank));
            destination[index].freq = apvts.getRawParameterValue(IDs::freq(bandNum, bank));
            destination[index].gain = apvts.getRawParameterValue(IDs::gain(bandNum, bank));
            destination[index].q = apvts.getRawParameterValue(IDs::q(bandNum, bank));
            destination[index].slope = apvts.getRawParameterValue(IDs::slope(bandNum, bank));

            jassert(destination[index].enabled != nullptr);
            jassert(destination[index].type != nullptr);
            jassert(destination[index].freq != nullptr);
            jassert(destination[index].gain != nullptr);
            jassert(destination[index].q != nullptr);
            jassert(destination[index].slope != nullptr);
        }
    };

    cacheBandPointers(bandsA_, Bank::A);
    cacheBandPointers(bandsB_, Bank::B);
}

float Params::getOutputGainDb() const noexcept {
    return outputGainDb_->load(std::memory_order_relaxed);
}

StereoMode Params::getStereoMode() const noexcept {
    return static_cast<StereoMode>(static_cast<int>(stereoMode_->load(std::memory_order_relaxed)));
}

HQMode Params::getHQMode() const noexcept {
    return static_cast<HQMode>(static_cast<int>(hqMode_->load(std::memory_order_relaxed)));
}

bool Params::isHQEnabled() const noexcept {
    return getHQMode() == HQMode::Oversampling2x;
}

EditTarget Params::getEditTarget() const noexcept {
    return static_cast<EditTarget>(static_cast<int>(editTarget_->load(std::memory_order_relaxed)));
}

const Params::BandParams& Params::getBand(int index, Bank bank) const noexcept {
    jassert(index >= 0 && index < NumBands);
    return (bank == Bank::A ? bandsA_ : bandsB_)[static_cast<std::size_t>(index)];
}

int Params::defaultTypeIndexForBand(int bandNum) noexcept {
    if (bandNum == 1)
        return static_cast<int>(FilterType::HighPass);

    if (bandNum == NumBands)
        return static_cast<int>(FilterType::LowPass);

    return static_cast<int>(FilterType::Peak);
}

float Params::defaultFrequencyHzForBand(int bandNum) noexcept {
    switch (bandNum) {
    case 1:
        return 50.0f;
    case 2:
        return 125.0f;
    case 3:
        return 250.0f;
    case 4:
        return 500.0f;
    case 5:
        return 1000.0f;
    case 6:
        return 2000.0f;
    case 7:
        return 4000.0f;
    case 8:
        return 10000.0f;
    default:
        return 1000.0f;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Params::createLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::editTarget, 1), "Edit Target",
                                                                  juce::StringArray{"Link", "A", "B"},
                                                                  static_cast<int>(EditTarget::Link)));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::stereoMode, 1), "Stereo Mode",
                                                                  juce::StringArray{"Stereo", "Mid/Side", "Left/Right"},
                                                                  static_cast<int>(StereoMode::Stereo)));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::hqMode, 1), "Quality",
                                                                  juce::StringArray{"Eco", "HQ (2x)"},
                                                                  static_cast<int>(HQMode::Off)));

    // Output Gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::outputGain, 1), "Output Gain",
                                                                 juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
                                                                 0.0f));

    // Filter type choices
    juce::StringArray typeChoices{"Peak", "Low Shelf", "High Shelf", "High Pass", "Low Pass"};
    // Slope choices
    juce::StringArray slopeChoices{"12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct"};

    auto addBandParametersForBank = [&](int bandNum, Bank bank) {
        const juce::String bankLabel = bank == Bank::A ? "A" : "B";
        const juce::String prefix = "Band " + juce::String(bandNum) + " " + bankLabel + " ";

        const bool defaultEnabled = false;
        const int defaultType = defaultTypeIndexForBand(bandNum);
        const float defaultFreq = defaultFrequencyHzForBand(bandNum);

        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(IDs::enabled(bandNum, bank), 1),
                                                                    prefix + "Enabled", defaultEnabled));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(IDs::type(bandNum, bank), 1),
                                                                      prefix + "Type", typeChoices, defaultType));

        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.1f);
        freqRange.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::freq(bandNum, bank), 1),
                                                                     prefix + "Freq", freqRange, defaultFreq));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(IDs::gain(bandNum, bank), 1), prefix + "Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), defaultGainDb()));

        juce::NormalisableRange<float> qRange(0.1f, 18.0f, 0.01f);
        qRange.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(IDs::q(bandNum, bank), 1),
                                                                     prefix + "Q", qRange, defaultQ()));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(IDs::slope(bandNum, bank), 1), prefix + "Slope", slopeChoices, defaultSlopeIndex()));
    };

    for (int i = 0; i < NumBands; ++i) {
        const int bandNum = i + 1;
        addBandParametersForBank(bandNum, Bank::A);
        addBandParametersForBank(bandNum, Bank::B);
    }

    return {params.begin(), params.end()};
}
} // namespace util
