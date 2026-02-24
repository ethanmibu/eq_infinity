#include "EqPlotComponent.h"
#include <cmath>

namespace ui {
namespace {

constexpr float MinFrequencyHz = 20.0f;
constexpr float MinDb = -24.0f;
constexpr float MaxDb = 24.0f;

} // namespace

EqPlotComponent::EqPlotComponent(util::Params& params, SpectrumAnalyzer& spectrumAnalyzer)
    : params_(params), spectrumAnalyzer_(spectrumAnalyzer) {
    startTimerHz(45);
    rebuildFrequencyAxis();
}

void EqPlotComponent::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void EqPlotComponent::setSelectedBand(int index) {
    selectedBandIndex_ = juce::jlimit(0, util::Params::NumBands - 1, index);
    repaint();
}

void EqPlotComponent::setBandSelectionCallback(std::function<void(int)> callback) {
    bandSelectionCallback_ = std::move(callback);
}

void EqPlotComponent::setBandSoloCallback(std::function<void(int, bool)> callback) {
    bandSoloCallback_ = std::move(callback);
}

void EqPlotComponent::paint(juce::Graphics& g) {
    const auto bounds = getPlotBounds();
    g.setColour(juce::Colour::fromRGB(30, 33, 37));
    g.fillRoundedRectangle(bounds, 6.0f);

    drawGrid(g, bounds);
    spectrumAnalyzer_.draw(g);

    if (!secondaryResponsePath_.isEmpty()) {
        g.setColour(juce::Colour::fromRGB(154, 179, 255).withAlpha(0.78f));
        g.strokePath(secondaryResponsePath_, juce::PathStrokeType(1.75f, juce::PathStrokeType::curved));
    }

    g.setColour(juce::Colour::fromRGB(118, 227, 255));
    g.strokePath(primaryResponsePath_, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved));
    drawResponseLegend(g, bounds);

    for (int i = 0; i < util::Params::NumBands; ++i) {
        const bool enabled = getBandFieldValueForDisplay(i, BandField::Enabled) > 0.5f;

        const float radius = (i == selectedBandIndex_) ? 11.0f : 9.0f;
        const auto node = nodePositions_[static_cast<std::size_t>(i)];
        const auto nodeBounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(node);

        g.setColour(enabled ? juce::Colour::fromRGB(255, 185, 67).withAlpha(0.9f)
                            : juce::Colour::fromRGB(120, 120, 120).withAlpha(0.8f));
        g.fillEllipse(nodeBounds);

        g.setColour(juce::Colour::fromRGB(28, 30, 34));
        g.fillEllipse(nodeBounds.reduced(2.2f));

        g.setColour(enabled ? juce::Colour::fromRGB(255, 185, 67) : juce::Colour::fromRGB(100, 100, 100));
        g.setFont(12.5f);
        g.drawFittedText(juce::String(i + 1), nodeBounds.toNearestInt(), juce::Justification::centred, 1);
    }
}

void EqPlotComponent::resized() {
    rebuildFrequencyAxis();
    rebuildPaths();
}

void EqPlotComponent::mouseDown(const juce::MouseEvent& event) {
    // Ensure node hit-testing uses the latest parameter-derived positions.
    rebuildPaths();

    draggingBandIndex_ = -1;
    const int bestIndex = findNearestNode(event.position, 16.0f);

    if (bestIndex >= 0) {
        selectedBandIndex_ = bestIndex;
        draggingBandIndex_ = bestIndex;
        dragStartPosition_ = event.position;

        dragStartFrequencyHz_ = getBandFieldValueForDisplay(bestIndex, BandField::Frequency);
        dragStartGainDb_ = getBandFieldValueForDisplay(bestIndex, BandField::Gain);
        dragStartQ_ = getBandFieldValueForDisplay(bestIndex, BandField::Q);

        if (getBandFieldValueForDisplay(bestIndex, BandField::Enabled) <= 0.5f)
            setBandFieldValueForEditTarget(bestIndex + 1, BandField::Enabled, 1.0f);

        updateSoloStateForModifier(bestIndex, event.mods);

        if (bandSelectionCallback_ != nullptr)
            bandSelectionCallback_(bestIndex);
    } else {
        if (soloActive_ && bandSoloCallback_ != nullptr)
            bandSoloCallback_(selectedBandIndex_, false);
        soloActive_ = false;
        draggingBandIndex_ = -1;
    }

    repaint();
}

void EqPlotComponent::mouseDoubleClick(const juce::MouseEvent& event) {
    const int bandIndex = findNearestNode(event.position, 18.0f);
    if (bandIndex < 0)
        return;

    selectedBandIndex_ = bandIndex;
    if (bandSelectionCallback_ != nullptr)
        bandSelectionCallback_(bandIndex);

    resetBandToDefaults(bandIndex);
    repaint();
}

void EqPlotComponent::mouseDrag(const juce::MouseEvent& event) {
    if (draggingBandIndex_ < 0)
        return;

    const auto plotBounds = getPlotBounds();
    const juce::Point<float> constrainedPosition{
        juce::jlimit(plotBounds.getX(), plotBounds.getRight(), event.position.x),
        juce::jlimit(plotBounds.getY(), plotBounds.getBottom(), event.position.y)};
    const int bandNum = draggingBandIndex_ + 1;
    const auto type = getBandType(draggingBandIndex_);

    updateSoloStateForModifier(draggingBandIndex_, event.mods);

    if (event.mods.isAltDown()) {
        float qSensitivity = 0.010f;
        if (event.mods.isShiftDown())
            qSensitivity *= 0.35f;

        const float qScale = std::exp((dragStartPosition_.y - constrainedPosition.y) * qSensitivity);
        const float nextQ = juce::jlimit(0.1f, 18.0f, dragStartQ_ * qScale);
        setBandFieldValueForEditTarget(bandNum, BandField::Q, nextQ);
        return;
    }

    float nextFreq = xToFrequency(constrainedPosition.x, plotBounds);
    if (event.mods.isShiftDown())
        nextFreq = dragStartFrequencyHz_ + (nextFreq - dragStartFrequencyHz_) * 0.2f;
    setBandFieldValueForEditTarget(bandNum, BandField::Frequency, nextFreq);

    if (usesGainAxis(type)) {
        float nextGain = yToDb(constrainedPosition.y, plotBounds);
        if (event.mods.isShiftDown())
            nextGain = dragStartGainDb_ + (nextGain - dragStartGainDb_) * 0.2f;
        setBandFieldValueForEditTarget(bandNum, BandField::Gain, nextGain);
    }
}

void EqPlotComponent::mouseUp(const juce::MouseEvent&) {
    if (soloActive_ && bandSoloCallback_ != nullptr)
        bandSoloCallback_(selectedBandIndex_, false);
    soloActive_ = false;
    draggingBandIndex_ = -1;
}

void EqPlotComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) {
    if (selectedBandIndex_ < 0 || selectedBandIndex_ >= util::Params::NumBands)
        return;

    const float currentQ = getBandFieldValueForDisplay(selectedBandIndex_, BandField::Q);
    const float scaledDelta = static_cast<float>(wheel.deltaY) * 0.35f;
    const float nextQ = juce::jlimit(0.1f, 18.0f, currentQ * std::exp(scaledDelta));
    setBandFieldValueForEditTarget(selectedBandIndex_ + 1, BandField::Q, nextQ);
}

void EqPlotComponent::timerCallback() {
    setSampleRate(sampleRate_ > 1.0 ? sampleRate_ : 44100.0);
    rebuildPaths();
    repaint();
}

juce::Rectangle<float> EqPlotComponent::getPlotBounds() const {
    auto bounds = getLocalBounds().toFloat().reduced(10.0f, 8.0f);
    bounds.removeFromLeft(42.0f);
    bounds.removeFromBottom(20.0f);
    return bounds;
}

void EqPlotComponent::rebuildFrequencyAxis() {
    const auto plotBounds = getPlotBounds();
    const auto pointCount = juce::jmax(static_cast<int>(plotBounds.getWidth()), 2);
    frequenciesHz_.resize(static_cast<std::size_t>(pointCount));

    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate_ * 0.495, 20000.0));
    const double ratio = maxFrequency / MinFrequencyHz;
    const double denominator = static_cast<double>(pointCount - 1);

    for (int i = 0; i < pointCount; ++i) {
        const double normalized = static_cast<double>(i) / denominator;
        frequenciesHz_[static_cast<std::size_t>(i)] = MinFrequencyHz * std::pow(ratio, normalized);
    }
}

void EqPlotComponent::rebuildPaths() {
    if (frequenciesHz_.empty())
        rebuildFrequencyAxis();

    const auto plotBounds = getPlotBounds();
    const auto primaryBank = getDisplayBank();
    const auto state = dsp::ResponseCurve::capture(params_, sampleRate_, primaryBank);
    magnitudeDb_ = dsp::ResponseCurve::computeMagnitudeDb(state, frequenciesHz_);

    primaryResponsePath_.clear();
    for (std::size_t i = 0; i < magnitudeDb_.size(); ++i) {
        const auto x = plotBounds.getX() + static_cast<float>(i);
        const auto y = dbToY(magnitudeDb_[i], plotBounds);

        if (i == 0)
            primaryResponsePath_.startNewSubPath(x, y);
        else
            primaryResponsePath_.lineTo(x, y);
    }

    secondaryResponsePath_.clear();
    secondaryMagnitudeDb_.clear();
    if (shouldDrawSecondaryResponse()) {
        const auto secondaryState = dsp::ResponseCurve::capture(params_, sampleRate_, getSecondaryDisplayBank());
        secondaryMagnitudeDb_ = dsp::ResponseCurve::computeMagnitudeDb(secondaryState, frequenciesHz_);
        for (std::size_t i = 0; i < secondaryMagnitudeDb_.size(); ++i) {
            const auto x = plotBounds.getX() + static_cast<float>(i);
            const auto y = dbToY(secondaryMagnitudeDb_[i], plotBounds);

            if (i == 0)
                secondaryResponsePath_.startNewSubPath(x, y);
            else
                secondaryResponsePath_.lineTo(x, y);
        }
    }

    for (int i = 0; i < util::Params::NumBands; ++i) {
        const auto& band = state.bands[static_cast<std::size_t>(i)];
        const float x = frequencyToX(band.frequencyHz, plotBounds);
        const float y = usesGainAxis(band.type) ? dbToY(band.gainDb, plotBounds) : dbToY(0.0f, plotBounds);
        nodePositions_[static_cast<std::size_t>(i)] = {x, y};
    }

    spectrumAnalyzer_.update(sampleRate_, plotBounds);
}

float EqPlotComponent::frequencyToX(float frequency, juce::Rectangle<float> bounds) const {
    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate_ * 0.495, 20000.0));
    const float logMin = std::log10(MinFrequencyHz);
    const float logMax = std::log10(maxFrequency);
    const float normalized = (std::log10(frequency) - logMin) / juce::jmax(logMax - logMin, 1.0e-6f);
    return bounds.getX() + normalized * bounds.getWidth();
}

float EqPlotComponent::xToFrequency(float x, juce::Rectangle<float> bounds) const {
    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate_ * 0.495, 20000.0));
    const float normalized = juce::jlimit(0.0f, 1.0f, (x - bounds.getX()) / bounds.getWidth());
    const float ratio = maxFrequency / MinFrequencyHz;
    return MinFrequencyHz * std::pow(ratio, normalized);
}

float EqPlotComponent::dbToY(float db, juce::Rectangle<float> bounds) {
    const float normalized = juce::jmap(db, MaxDb, MinDb, 0.0f, 1.0f);
    return bounds.getY() + normalized * bounds.getHeight();
}

float EqPlotComponent::yToDb(float y, juce::Rectangle<float> bounds) {
    const float normalized = juce::jlimit(0.0f, 1.0f, (y - bounds.getY()) / bounds.getHeight());
    return juce::jmap(normalized, 0.0f, 1.0f, MaxDb, MinDb);
}

void EqPlotComponent::setParameterValue(const juce::String& parameterID, float value) {
    auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(params_.apvts.getParameter(parameterID));
    if (parameter == nullptr)
        return;

    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void EqPlotComponent::setBandFieldValueForEditTarget(int bandNum, BandField field, float value) {
    auto writeField = [&](util::Bank bank) {
        switch (field) {
        case BandField::Enabled:
            setParameterValue(util::Params::IDs::enabled(bandNum, bank), value);
            break;
        case BandField::Type:
            setParameterValue(util::Params::IDs::type(bandNum, bank), value);
            break;
        case BandField::Frequency:
            setParameterValue(util::Params::IDs::freq(bandNum, bank), value);
            break;
        case BandField::Gain:
            setParameterValue(util::Params::IDs::gain(bandNum, bank), value);
            break;
        case BandField::Q:
            setParameterValue(util::Params::IDs::q(bandNum, bank), value);
            break;
        case BandField::Slope:
            setParameterValue(util::Params::IDs::slope(bandNum, bank), value);
            break;
        }
    };

    const auto editTarget = params_.getEditTarget();
    if (editTarget == util::EditTarget::Link) {
        writeField(util::Bank::A);
        writeField(util::Bank::B);
    } else if (editTarget == util::EditTarget::B) {
        writeField(util::Bank::B);
    } else {
        writeField(util::Bank::A);
    }
}

float EqPlotComponent::getBandFieldValueForDisplay(int bandIndex, BandField field) const {
    const auto& band = params_.getBand(bandIndex, getDisplayBank());
    switch (field) {
    case BandField::Enabled:
        return band.enabled->load(std::memory_order_relaxed);
    case BandField::Type:
        return band.type->load(std::memory_order_relaxed);
    case BandField::Frequency:
        return band.freq->load(std::memory_order_relaxed);
    case BandField::Gain:
        return band.gain->load(std::memory_order_relaxed);
    case BandField::Q:
        return band.q->load(std::memory_order_relaxed);
    case BandField::Slope:
        return band.slope->load(std::memory_order_relaxed);
    }

    return 0.0f;
}

util::Bank EqPlotComponent::getDisplayBank() const noexcept {
    return params_.getEditTarget() == util::EditTarget::B ? util::Bank::B : util::Bank::A;
}

bool EqPlotComponent::shouldDrawSecondaryResponse() const noexcept {
    return params_.getStereoMode() != util::StereoMode::Stereo;
}

util::Bank EqPlotComponent::getSecondaryDisplayBank() const noexcept {
    return getDisplayBank() == util::Bank::A ? util::Bank::B : util::Bank::A;
}

void EqPlotComponent::drawResponseLegend(juce::Graphics& g, juce::Rectangle<float> bounds) const {
    if (!shouldDrawSecondaryResponse())
        return;

    juce::String primaryLabel = "A";
    juce::String secondaryLabel = "B";
    const auto mode = params_.getStereoMode();

    if (mode == util::StereoMode::LeftRight) {
        primaryLabel = getDisplayBank() == util::Bank::A ? "L" : "R";
        secondaryLabel = getDisplayBank() == util::Bank::A ? "R" : "L";
    } else if (mode == util::StereoMode::MidSide) {
        primaryLabel = getDisplayBank() == util::Bank::A ? "M" : "S";
        secondaryLabel = getDisplayBank() == util::Bank::A ? "S" : "M";
    } else {
        primaryLabel = getDisplayBank() == util::Bank::A ? "A" : "B";
        secondaryLabel = getDisplayBank() == util::Bank::A ? "B" : "A";
    }

    auto legendBounds = bounds.removeFromTop(20.0f).removeFromRight(86.0f);
    g.setFont(11.0f);

    g.setColour(juce::Colour::fromRGB(118, 227, 255));
    g.drawText(primaryLabel, legendBounds.removeFromLeft(26).reduced(2, 0), juce::Justification::centredRight);

    g.setColour(juce::Colour::fromRGB(154, 179, 255).withAlpha(0.85f));
    g.drawText(secondaryLabel, legendBounds.removeFromLeft(26).reduced(2, 0), juce::Justification::centredRight);
}

void EqPlotComponent::resetBandToDefaults(int bandIndex) {
    if (bandIndex < 0 || bandIndex >= util::Params::NumBands)
        return;

    const int bandNum = bandIndex + 1;
    setBandFieldValueForEditTarget(bandNum, BandField::Enabled, 1.0f);
    setBandFieldValueForEditTarget(bandNum, BandField::Type, static_cast<float>(util::Params::defaultTypeIndexForBand(bandNum)));
    setBandFieldValueForEditTarget(bandNum, BandField::Frequency, util::Params::defaultFrequencyHzForBand(bandNum));
    setBandFieldValueForEditTarget(bandNum, BandField::Gain, util::Params::defaultGainDb());
    setBandFieldValueForEditTarget(bandNum, BandField::Q, util::Params::defaultQ());
    setBandFieldValueForEditTarget(bandNum, BandField::Slope, static_cast<float>(util::Params::defaultSlopeIndex()));
}

void EqPlotComponent::updateSoloStateForModifier(int bandIndex, const juce::ModifierKeys& modifiers) {
    if (bandIndex < 0)
        return;

    const bool wantsSolo = modifiers.isCommandDown();
    if (wantsSolo && !soloActive_) {
        if (bandSoloCallback_ != nullptr)
            bandSoloCallback_(bandIndex, true);
        soloActive_ = true;
        return;
    }

    if (!wantsSolo && soloActive_) {
        if (bandSoloCallback_ != nullptr)
            bandSoloCallback_(bandIndex, false);
        soloActive_ = false;
    }
}

int EqPlotComponent::findNearestNode(juce::Point<float> position, float threshold) const {
    float bestDistance = threshold;
    int bestIndex = -1;

    for (int i = 0; i < util::Params::NumBands; ++i) {
        const auto distance = position.getDistanceFrom(nodePositions_[static_cast<std::size_t>(i)]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool EqPlotComponent::usesGainAxis(util::FilterType type) noexcept {
    return type == util::FilterType::Peak || type == util::FilterType::LowShelf || type == util::FilterType::HighShelf;
}

util::FilterType EqPlotComponent::getBandType(int bandIndex) const noexcept {
    const auto& band = params_.getBand(bandIndex, getDisplayBank());
    return static_cast<util::FilterType>(static_cast<int>(band.type->load(std::memory_order_relaxed)));
}

void EqPlotComponent::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    constexpr std::array<float, 9> dbMarks{MinDb, -18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f, MaxDb};
    for (const auto db : dbMarks) {
        const float y = dbToY(db, bounds);
        g.setColour(db == 0.0f ? juce::Colours::white.withAlpha(0.35f) : juce::Colours::white.withAlpha(0.14f));
        g.drawHorizontalLine(static_cast<int>(std::round(y)), bounds.getX(), bounds.getRight());
    }

    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate_ * 0.495, 20000.0));
    const std::array<float, 25> freqMarks{20.0f,   30.0f,   40.0f,    50.0f,   60.0f,   80.0f,   100.0f,
                                          125.0f,  160.0f,  200.0f,   250.0f,  315.0f,  400.0f,  500.0f,
                                          630.0f,  800.0f,  1000.0f,  2000.0f, 3000.0f, 4000.0f, 5000.0f,
                                          6300.0f, 8000.0f, 10000.0f, 20000.0f};

    for (const auto freq : freqMarks) {
        if (freq > maxFrequency)
            continue;

        const float x = frequencyToX(freq, bounds);
        const bool major = freq == 100.0f || freq == 1000.0f || freq == 10000.0f;
        g.setColour(major ? juce::Colours::white.withAlpha(0.25f) : juce::Colours::white.withAlpha(0.1f));
        g.drawVerticalLine(static_cast<int>(std::round(x)), bounds.getY(), bounds.getBottom());
    }

    g.setColour(juce::Colours::white.withAlpha(0.55f));
    g.setFont(11.0f);

    const std::array<float, 5> labeledDbMarks{MaxDb, 12.0f, 0.0f, -12.0f, MinDb};
    for (const float db : labeledDbMarks) {
        const int y = static_cast<int>(std::round(dbToY(db, bounds))) - 7;
        juce::String label = juce::String(db, db == 0.0f ? 0 : 0);
        if (db > 0.0f)
            label = "+" + label;

        g.drawText(label, juce::Rectangle<int>(static_cast<int>(bounds.getX()) - 38, y, 32, 14),
                   juce::Justification::centredRight);
    }

    struct FreqLabel {
        float hz;
        const char* text;
    };

    const std::array<FreqLabel, 10> freqLabels{{
        {20.0f, "20"},
        {50.0f, "50"},
        {100.0f, "100"},
        {200.0f, "200"},
        {500.0f, "500"},
        {1000.0f, "1k"},
        {2000.0f, "2k"},
        {5000.0f, "5k"},
        {10000.0f, "10k"},
        {20000.0f, "20k"},
    }};

    for (const auto& label : freqLabels) {
        if (label.hz > maxFrequency)
            continue;

        const int x = static_cast<int>(std::round(frequencyToX(label.hz, bounds)));
        g.drawText(label.text, juce::Rectangle<int>(x - 14, static_cast<int>(bounds.getBottom()) + 2, 28, 14),
                   juce::Justification::centred);
    }
}

} // namespace ui
