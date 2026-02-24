#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

class EQInfinityAudioProcessor;

namespace ui {

class SpectrumAnalyzer {
  public:
    explicit SpectrumAnalyzer(EQInfinityAudioProcessor& processor);

    void update(double sampleRate, juce::Rectangle<float> plotBounds);
    void draw(juce::Graphics& g) const;

  private:
    static constexpr int FFTOrder = 11;
    static constexpr int FFTSize = 1 << FFTOrder;
    static constexpr int TempBlockSize = 512;

    EQInfinityAudioProcessor& processor_;
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;

    std::array<float, FFTSize> preHistory_{};
    std::array<float, FFTSize> postHistory_{};
    int preWritePosition_ = 0;
    int postWritePosition_ = 0;

    std::array<float, FFTSize * 2> preFftBuffer_{};
    std::array<float, FFTSize * 2> postFftBuffer_{};

    juce::Path prePath_;
    juce::Path postPath_;
    juce::Rectangle<float> bounds_;

    float minDb_ = -96.0f;
    float maxDb_ = 12.0f;

    void pullFromProcessor();
    void buildPathFromHistory(const std::array<float, FFTSize>& history, int writePosition,
                              std::array<float, FFTSize * 2>& fftBuffer, juce::Path& path, double sampleRate);
};

} // namespace ui
