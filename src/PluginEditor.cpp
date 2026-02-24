#include "PluginEditor.h"

EQInfinityAudioProcessorEditor::EQInfinityAudioProcessorEditor(EQInfinityAudioProcessor& p)
    : AudioProcessorEditor(&p), processor_(p) {
    setSize(420, 220);

    outputGainSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 22);
    outputGainSlider_.setTextValueSuffix(" dB");
    outputGainSlider_.setSkewFactorFromMidPoint(0.0);

    outputGainLabel_.setText("Output Gain", juce::dontSendNotification);
    outputGainLabel_.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(outputGainSlider_);
    addAndMakeVisible(outputGainLabel_);

    outputGainAttachment_ =
        std::make_unique<SliderAttachment>(processor_.params().apvts, util::Params::IDs::outputGain, outputGainSlider_);
}

void EQInfinityAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("EQInfinity", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void EQInfinityAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(50);

    auto knobArea = area.removeFromTop(150);
    outputGainLabel_.setBounds(knobArea.removeFromTop(22));
    outputGainSlider_.setBounds(knobArea.reduced(40, 10));
}
