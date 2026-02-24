#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GravityEQAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GravityEQAudioProcessorEditor(GravityEQAudioProcessor&);
    ~GravityEQAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    GravityEQAudioProcessor& processor_;

    juce::Slider outputGainSlider_;
    juce::Label outputGainLabel_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GravityEQAudioProcessorEditor)
};
