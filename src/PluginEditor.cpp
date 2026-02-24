#include "PluginEditor.h"
#include <cmath>

EQInfinityAudioProcessorEditor::EQInfinityAudioProcessorEditor(EQInfinityAudioProcessor& p)
    : AudioProcessorEditor(&p), processor_(p), spectrumAnalyzer_(p), eqPlot_(processor_.params(), spectrumAnalyzer_) {
    setSize(980, 430);
    setResizable(true, true);
    setResizeLimits(840, 360, 1400, 620);

    configureControls();

    eqPlot_.setBandSelectionCallback([this](int index) { selectBand(index); });
    eqPlot_.setBandSoloCallback([this](int index, bool active) {
        if (active)
            processor_.setSoloBandIndex(index);
        else
            processor_.clearSoloBand();
    });
    addAndMakeVisible(eqPlot_);

    outputGainAttachment_ =
        std::make_unique<SliderAttachment>(processor_.params().apvts, util::Params::IDs::outputGain, outputGainSlider_);
    stereoModeAttachment_ =
        std::make_unique<ComboBoxAttachment>(processor_.params().apvts, util::Params::IDs::stereoMode, stereoModeBox_);
    editTargetAttachment_ =
        std::make_unique<ComboBoxAttachment>(processor_.params().apvts, util::Params::IDs::editTarget, editTargetBox_);
    qualityModeAttachment_ =
        std::make_unique<ComboBoxAttachment>(processor_.params().apvts, util::Params::IDs::hqMode, qualityModeBox_);

    updateGlobalControlLabels();
    selectBand(0);
    startTimerHz(20);
}

EQInfinityAudioProcessorEditor::~EQInfinityAudioProcessorEditor() {
    processor_.clearSoloBand();
}

void EQInfinityAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(33, 35, 39));

    g.setColour(juce::Colour::fromRGB(45, 48, 53));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(6.0f), 7.0f);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(6.0f), 7.0f, 1.0f);
}

void EQInfinityAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds().reduced(14);
    auto bottomStrip = bounds.removeFromBottom(110);
    auto leftColumn = bounds.removeFromLeft(118);
    auto plotBounds = bounds.reduced(6, 0);

    titleLabel_.setBounds(leftColumn.removeFromTop(34));
    outputGainLabel_.setBounds(leftColumn.removeFromTop(24));
    outputGainSlider_.setBounds(leftColumn.reduced(8, 8));

    eqPlot_.setBounds(plotBounds);

    auto selectedControls = bottomStrip.removeFromTop(58);
    auto globalControls = selectedControls.removeFromRight(324);

    const int rowHeight = globalControls.getHeight() / 3;
    auto stereoArea = globalControls.removeFromTop(rowHeight);
    stereoModeLabel_.setBounds(stereoArea.removeFromLeft(56).reduced(2));
    stereoModeBox_.setBounds(stereoArea.reduced(2));

    auto targetArea = globalControls.removeFromTop(rowHeight);
    editTargetLabel_.setBounds(targetArea.removeFromLeft(56).reduced(2));
    editTargetBox_.setBounds(targetArea.reduced(2));

    auto qualityArea = globalControls;
    qualityLabel_.setBounds(qualityArea.removeFromLeft(56).reduced(2));
    qualityModeBox_.setBounds(qualityArea.reduced(2));

    selectedBandLabel_.setBounds(selectedControls.removeFromLeft(74).reduced(4));
    bandEnableToggle_.setBounds(selectedControls.removeFromLeft(80).reduced(4));
    bandTypeBox_.setBounds(selectedControls.removeFromLeft(154).reduced(4));
    bandSlopeBox_.setBounds(selectedControls.removeFromLeft(122).reduced(4));
    bandFreqSlider_.setBounds(selectedControls.removeFromLeft(152).reduced(4));
    bandGainSlider_.setBounds(selectedControls.removeFromLeft(122).reduced(4));
    bandQSlider_.setBounds(selectedControls.removeFromLeft(112).reduced(4));

    auto bandRow = bottomStrip.reduced(0, 8);
    const int buttonWidth = bandRow.getWidth() / util::Params::NumBands;
    for (int i = 0; i < util::Params::NumBands; ++i) {
        auto cell = bandRow.removeFromLeft(buttonWidth).reduced(4, 2);
        bandButtons_[static_cast<std::size_t>(i)].setBounds(cell);
    }
}

void EQInfinityAudioProcessorEditor::timerCallback() {
    eqPlot_.setSampleRate(processor_.getSampleRate());

    const auto editTarget = processor_.params().getEditTarget();
    const auto stereoMode = processor_.params().getStereoMode();

    if (editTarget != lastEditTarget_ || stereoMode != lastStereoMode_) {
        lastEditTarget_ = editTarget;
        lastStereoMode_ = stereoMode;
        updateGlobalControlLabels();
        rebuildSelectedBandAttachments();
    }

    if (editTarget == util::EditTarget::Link) {
        const int bandNum = selectedBandIndex_ + 1;
        auto& apvts = processor_.params().apvts;
        auto mirrorField = [&](const juce::String& sourceID, const juce::String& destinationID) {
            auto* source = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(sourceID));
            auto* destination = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(destinationID));
            if (source == nullptr || destination == nullptr)
                return;

            const float sourceValue = source->convertFrom0to1(source->getValue());
            const float destinationValue = destination->convertFrom0to1(destination->getValue());
            if (std::abs(sourceValue - destinationValue) < 1.0e-6f)
                return;

            destination->setValueNotifyingHost(destination->convertTo0to1(sourceValue));
        };

        mirrorField(util::Params::IDs::enabled(bandNum, util::Bank::A), util::Params::IDs::enabled(bandNum, util::Bank::B));
        mirrorField(util::Params::IDs::type(bandNum, util::Bank::A), util::Params::IDs::type(bandNum, util::Bank::B));
        mirrorField(util::Params::IDs::freq(bandNum, util::Bank::A), util::Params::IDs::freq(bandNum, util::Bank::B));
        mirrorField(util::Params::IDs::gain(bandNum, util::Bank::A), util::Params::IDs::gain(bandNum, util::Bank::B));
        mirrorField(util::Params::IDs::q(bandNum, util::Bank::A), util::Params::IDs::q(bandNum, util::Bank::B));
        mirrorField(util::Params::IDs::slope(bandNum, util::Bank::A), util::Params::IDs::slope(bandNum, util::Bank::B));
    }

    updateBandButtonStyles();
}

void EQInfinityAudioProcessorEditor::configureControls() {
    titleLabel_.setText("EQ Infinity", juce::dontSendNotification);
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    titleLabel_.setFont(juce::FontOptions{23.0f, juce::Font::bold});
    titleLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 228, 231));
    addAndMakeVisible(titleLabel_);

    outputGainLabel_.setText("Output", juce::dontSendNotification);
    outputGainLabel_.setJustificationType(juce::Justification::centredLeft);
    outputGainLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(205, 208, 212));
    addAndMakeVisible(outputGainLabel_);

    outputGainSlider_.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGainSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 24);
    outputGainSlider_.setTextValueSuffix(" dB");
    outputGainSlider_.setDoubleClickReturnValue(true, 0.0);
    outputGainSlider_.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(118, 227, 255));
    outputGainSlider_.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(250, 183, 64));
    outputGainSlider_.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(59, 62, 67));
    addAndMakeVisible(outputGainSlider_);

    selectedBandLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(220, 223, 226));
    selectedBandLabel_.setJustificationType(juce::Justification::centredLeft);
    selectedBandLabel_.setFont(juce::FontOptions{15.0f, juce::Font::bold});
    addAndMakeVisible(selectedBandLabel_);

    bandEnableToggle_.setButtonText("Enabled");
    bandEnableToggle_.setColour(juce::ToggleButton::textColourId, juce::Colour::fromRGB(215, 218, 221));
    addAndMakeVisible(bandEnableToggle_);

    bandTypeBox_.addItemList({"Peak", "Low Shelf", "High Shelf", "High Pass", "Low Pass"}, 1);
    bandSlopeBox_.addItemList({"12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct"}, 1);
    addAndMakeVisible(bandTypeBox_);
    addAndMakeVisible(bandSlopeBox_);

    stereoModeLabel_.setText("Mode", juce::dontSendNotification);
    stereoModeLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(205, 208, 212));
    stereoModeLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(stereoModeLabel_);

    stereoModeBox_.addItemList({"Stereo", "Mid/Side", "Left/Right"}, 1);
    addAndMakeVisible(stereoModeBox_);

    editTargetLabel_.setText("Edit", juce::dontSendNotification);
    editTargetLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(205, 208, 212));
    editTargetLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(editTargetLabel_);

    editTargetBox_.addItemList({"Link", "A", "B"}, 1);
    addAndMakeVisible(editTargetBox_);

    qualityLabel_.setText("Qual", juce::dontSendNotification);
    qualityLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(205, 208, 212));
    qualityLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(qualityLabel_);

    qualityModeBox_.addItemList({"Eco", "HQ (2x)"}, 1);
    addAndMakeVisible(qualityModeBox_);

    bandFreqSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    bandFreqSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    bandFreqSlider_.setTextValueSuffix(" Hz");
    bandFreqSlider_.setDoubleClickReturnValue(true, util::Params::defaultFrequencyHzForBand(1));
    bandFreqSlider_.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(118, 227, 255));
    addAndMakeVisible(bandFreqSlider_);

    bandGainSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    bandGainSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 22);
    bandGainSlider_.setTextValueSuffix(" dB");
    bandGainSlider_.setDoubleClickReturnValue(true, util::Params::defaultGainDb());
    bandGainSlider_.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(118, 227, 255));
    addAndMakeVisible(bandGainSlider_);

    bandQSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    bandQSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 22);
    bandQSlider_.setDoubleClickReturnValue(true, util::Params::defaultQ());
    bandQSlider_.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(118, 227, 255));
    addAndMakeVisible(bandQSlider_);

    for (int i = 0; i < util::Params::NumBands; ++i) {
        auto& button = bandButtons_[static_cast<std::size_t>(i)];
        button.setButtonText("Band " + juce::String(i + 1));
        button.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(53, 56, 61));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(203, 206, 210));
        button.setColour(juce::TextButton::textColourOnId, juce::Colour::fromRGB(22, 24, 28));
        button.onClick = [this, i]() { selectBand(i); };
        addAndMakeVisible(button);
    }
}

void EQInfinityAudioProcessorEditor::selectBand(int bandIndex) {
    selectedBandIndex_ = juce::jlimit(0, util::Params::NumBands - 1, bandIndex);
    eqPlot_.setSelectedBand(selectedBandIndex_);
    rebuildSelectedBandAttachments();
    updateBandButtonStyles();
}

void EQInfinityAudioProcessorEditor::rebuildSelectedBandAttachments() {
    const int bandNum = selectedBandIndex_ + 1;
    selectedBandLabel_.setText("Band " + juce::String(bandNum), juce::dontSendNotification);
    bandFreqSlider_.setDoubleClickReturnValue(true, util::Params::defaultFrequencyHzForBand(bandNum));
    bandGainSlider_.setDoubleClickReturnValue(true, util::Params::defaultGainDb());
    bandQSlider_.setDoubleClickReturnValue(true, util::Params::defaultQ());

    auto& apvts = processor_.params().apvts;
    const auto bank = getAttachmentBank();
    bandEnableAttachment_ =
        std::make_unique<ButtonAttachment>(apvts, util::Params::IDs::enabled(bandNum, bank), bandEnableToggle_);
    bandTypeAttachment_ = std::make_unique<ComboBoxAttachment>(apvts, util::Params::IDs::type(bandNum, bank), bandTypeBox_);
    bandSlopeAttachment_ =
        std::make_unique<ComboBoxAttachment>(apvts, util::Params::IDs::slope(bandNum, bank), bandSlopeBox_);
    bandFreqAttachment_ = std::make_unique<SliderAttachment>(apvts, util::Params::IDs::freq(bandNum, bank), bandFreqSlider_);
    bandGainAttachment_ = std::make_unique<SliderAttachment>(apvts, util::Params::IDs::gain(bandNum, bank), bandGainSlider_);
    bandQAttachment_ = std::make_unique<SliderAttachment>(apvts, util::Params::IDs::q(bandNum, bank), bandQSlider_);

    updateSelectedBandControlState();
}

void EQInfinityAudioProcessorEditor::updateSelectedBandControlState() {
    const auto typeIndex = bandTypeBox_.getSelectedItemIndex();
    if (typeIndex < 0)
        return;

    const auto type = static_cast<util::FilterType>(typeIndex);
    const bool usesGain =
        type == util::FilterType::Peak || type == util::FilterType::LowShelf || type == util::FilterType::HighShelf;
    const bool usesSlope = type == util::FilterType::HighPass || type == util::FilterType::LowPass;

    bandGainSlider_.setEnabled(usesGain);
    bandSlopeBox_.setEnabled(usesSlope);
}

void EQInfinityAudioProcessorEditor::updateBandButtonStyles() {
    const auto bank = getAttachmentBank();
    for (int i = 0; i < util::Params::NumBands; ++i) {
        auto& button = bandButtons_[static_cast<std::size_t>(i)];
        const auto& band = processor_.params().getBand(i, bank);
        const bool enabled = band.enabled->load(std::memory_order_relaxed) > 0.5f;
        const bool selected = i == selectedBandIndex_;

        auto base = enabled ? juce::Colour::fromRGB(74, 122, 138) : juce::Colour::fromRGB(56, 58, 62);
        if (selected)
            base = juce::Colour::fromRGB(250, 183, 64);
        button.setColour(juce::TextButton::buttonColourId, base);
    }

    updateSelectedBandControlState();
}

void EQInfinityAudioProcessorEditor::updateGlobalControlLabels() {
    const auto mode = processor_.params().getStereoMode();
    juce::StringArray labels{"Link", "A", "B"};

    if (mode == util::StereoMode::LeftRight)
        labels = {"Link", "L", "R"};
    else if (mode == util::StereoMode::MidSide)
        labels = {"Link", "M", "S"};

    const int currentIndex = juce::jmax(0, editTargetBox_.getSelectedItemIndex());
    editTargetBox_.clear(juce::dontSendNotification);
    editTargetBox_.addItemList(labels, 1);
    editTargetBox_.setSelectedItemIndex(juce::jlimit(0, labels.size() - 1, currentIndex), juce::dontSendNotification);
}

util::Bank EQInfinityAudioProcessorEditor::getAttachmentBank() const noexcept {
    return processor_.params().getEditTarget() == util::EditTarget::B ? util::Bank::B : util::Bank::A;
}
