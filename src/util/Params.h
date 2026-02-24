#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

namespace util {

enum class FilterType { Peak, LowShelf, HighShelf, HighPass, LowPass };

enum class Slope { Slope12dB, Slope24dB, Slope36dB, Slope48dB };

class Params final {
  public:
    static constexpr int NumBands = 8;

    struct IDs {
        static constexpr const char* outputGain = "out_gain";

        static juce::String enabled(int bandNum) { return "b" + juce::String(bandNum) + "_enabled"; }
        static juce::String type(int bandNum) { return "b" + juce::String(bandNum) + "_type"; }
        static juce::String freq(int bandNum) { return "b" + juce::String(bandNum) + "_freq"; }
        static juce::String gain(int bandNum) { return "b" + juce::String(bandNum) + "_gain"; }
        static juce::String q(int bandNum) { return "b" + juce::String(bandNum) + "_q"; }
        static juce::String slope(int bandNum) { return "b" + juce::String(bandNum) + "_slope"; }
    };

    struct BandParams {
        std::atomic<float>* enabled = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* slope = nullptr;
    };

    explicit Params(juce::AudioProcessor& processor);

    juce::AudioProcessorValueTreeState apvts;

    float getOutputGainDb() const noexcept;
    const BandParams& getBand(int index) const noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

  private:
    std::atomic<float>* outputGainDb_ = nullptr;
    std::array<BandParams, NumBands> bands_;
};
} // namespace util
