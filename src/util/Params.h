#pragma once

#include <JuceHeader.h>
#include <atomic>

namespace util
{
class Params final
{
public:
    struct IDs
    {
        static constexpr const char* outputGain = "outputGain";
    };

    explicit Params(juce::AudioProcessor& processor);

    juce::AudioProcessorValueTreeState apvts;

    float getOutputGainDb() const noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    std::atomic<float>* outputGainDb_ = nullptr;
};
} // namespace util
