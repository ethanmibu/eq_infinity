#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class EQInfinityAudioProcessorEditor final : public juce::AudioProcessorEditor {
  public:
    explicit EQInfinityAudioProcessorEditor(EQInfinityAudioProcessor&);
    ~EQInfinityAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

  private:
    EQInfinityAudioProcessor& processor_;

    juce::Slider outputGainSlider_;
    juce::Label outputGainLabel_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQInfinityAudioProcessorEditor)
};
