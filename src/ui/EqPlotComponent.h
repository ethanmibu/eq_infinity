#pragma once

#include "../dsp/ResponseCurve.h"
#include "../util/Params.h"
#include "SpectrumAnalyzer.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

class EqPlotComponent final : public juce::Component, private juce::Timer {
  public:
    EqPlotComponent(util::Params& params, SpectrumAnalyzer& spectrumAnalyzer);

    void setSampleRate(double sampleRate);
    void setSelectedBand(int index);
    [[nodiscard]] int getSelectedBand() const noexcept { return selectedBandIndex_; }
    void setBandSelectionCallback(std::function<void(int)> callback);
    void setBandSoloCallback(std::function<void(int, bool)> callback);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

  private:
    enum class BandField { Enabled, Type, Frequency, Gain, Q, Slope };

    util::Params& params_;
    SpectrumAnalyzer& spectrumAnalyzer_;
    std::function<void(int)> bandSelectionCallback_;
    std::function<void(int, bool)> bandSoloCallback_;

    std::vector<double> frequenciesHz_;
    std::vector<float> magnitudeDb_;
    std::vector<float> secondaryMagnitudeDb_;
    juce::Path primaryResponsePath_;
    juce::Path secondaryResponsePath_;
    std::array<juce::Point<float>, util::Params::NumBands> nodePositions_{};

    double sampleRate_ = 44100.0;
    int selectedBandIndex_ = 0;
    int draggingBandIndex_ = -1;
    bool soloActive_ = false;
    juce::Point<float> dragStartPosition_{};
    float dragStartFrequencyHz_ = 1000.0f;
    float dragStartGainDb_ = 0.0f;
    float dragStartQ_ = 1.0f;

    void timerCallback() override;

    [[nodiscard]] juce::Rectangle<float> getPlotBounds() const;
    void rebuildFrequencyAxis();
    void rebuildPaths();

    [[nodiscard]] float frequencyToX(float frequency, juce::Rectangle<float> bounds) const;
    [[nodiscard]] float xToFrequency(float x, juce::Rectangle<float> bounds) const;
    [[nodiscard]] static float dbToY(float db, juce::Rectangle<float> bounds);
    [[nodiscard]] static float yToDb(float y, juce::Rectangle<float> bounds);

    void setParameterValue(const juce::String& parameterID, float value);
    void setBandFieldValueForEditTarget(int bandNum, BandField field, float value);
    [[nodiscard]] float getBandFieldValueForDisplay(int bandIndex, BandField field) const;
    [[nodiscard]] util::Bank getDisplayBank() const noexcept;
    [[nodiscard]] bool shouldDrawSecondaryResponse() const noexcept;
    [[nodiscard]] util::Bank getSecondaryDisplayBank() const noexcept;
    void drawResponseLegend(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void resetBandToDefaults(int bandIndex);
    void updateSoloStateForModifier(int bandIndex, const juce::ModifierKeys& modifiers);
    [[nodiscard]] int findNearestNode(juce::Point<float> position, float threshold) const;
    [[nodiscard]] static bool usesGainAxis(util::FilterType type) noexcept;
    [[nodiscard]] util::FilterType getBandType(int bandIndex) const noexcept;
    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds);
};

} // namespace ui
