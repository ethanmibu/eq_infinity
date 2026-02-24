#include "Params.h"

namespace util
{
Params::Params(juce::AudioProcessor& processor)
    : apvts(processor, nullptr, "PARAMS", createLayout())
{
    // Cache raw parameter pointers for RT access (no string lookups in processBlock)
    outputGainDb_ = apvts.getRawParameterValue(IDs::outputGain);
    jassert(outputGainDb_ != nullptr);
}

float Params::getOutputGainDb() const noexcept
{
    // getRawParameterValue returns atomic<float>*
    return outputGainDb_->load(std::memory_order_relaxed);
}

juce::AudioProcessorValueTreeState::ParameterLayout Params::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::outputGain,
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
        0.0f));

    return { params.begin(), params.end() };
}
} // namespace util
