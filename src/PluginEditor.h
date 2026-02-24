#pragma once

#include "PluginProcessor.h"
#include "ui/EqPlotComponent.h"
#include "ui/SpectrumAnalyzer.h"
#include <JuceHeader.h>

class EQInfinityAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit EQInfinityAudioProcessorEditor(EQInfinityAudioProcessor& processor);
    ~EQInfinityAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    EQInfinityAudioProcessor& processor_;
    ui::SpectrumAnalyzer spectrumAnalyzer_;
    ui::EqPlotComponent eqPlot_;

    // Left column
    juce::Slider outputGainSlider_;
    juce::Label titleLabel_;
    juce::Label outputGainLabel_;

    // Bottom strip controls
    std::array<juce::TextButton, util::Params::NumBands> bandButtons_;
    juce::Label selectedBandLabel_;
    juce::ToggleButton bandEnableToggle_;
    juce::ComboBox bandTypeBox_;
    juce::ComboBox bandSlopeBox_;
    juce::Slider bandFreqSlider_;
    juce::Slider bandGainSlider_;
    juce::Slider bandQSlider_;
    juce::Label stereoModeLabel_;
    juce::Label editTargetLabel_;
    juce::Label qualityLabel_;
    juce::ComboBox stereoModeBox_;
    juce::ComboBox editTargetBox_;
    juce::ComboBox qualityModeBox_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> outputGainAttachment_;
    std::unique_ptr<ComboBoxAttachment> stereoModeAttachment_;
    std::unique_ptr<ComboBoxAttachment> editTargetAttachment_;
    std::unique_ptr<ComboBoxAttachment> qualityModeAttachment_;
    std::unique_ptr<ButtonAttachment> bandEnableAttachment_;
    std::unique_ptr<ComboBoxAttachment> bandTypeAttachment_;
    std::unique_ptr<ComboBoxAttachment> bandSlopeAttachment_;
    std::unique_ptr<SliderAttachment> bandFreqAttachment_;
    std::unique_ptr<SliderAttachment> bandGainAttachment_;
    std::unique_ptr<SliderAttachment> bandQAttachment_;

    int selectedBandIndex_ = -1;
    util::EditTarget lastEditTarget_ = util::EditTarget::Link;
    util::StereoMode lastStereoMode_ = util::StereoMode::Stereo;

    void timerCallback() override;
    void configureControls();
    void selectBand(int bandIndex);
    void setBandControlsEnabled(bool enabled);
    void enforceStereoEditTargetPolicy();
    void rebuildSelectedBandAttachments();
    void updateSelectedBandControlState();
    void updateBandButtonStyles();
    void updateGlobalControlLabels();
    [[nodiscard]] util::Bank getAttachmentBank() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQInfinityAudioProcessorEditor)
};
