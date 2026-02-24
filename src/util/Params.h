#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>

namespace util {

enum class FilterType { Peak, LowShelf, HighShelf, HighPass, LowPass };

enum class Slope { Slope12dB, Slope24dB, Slope36dB, Slope48dB };

enum class Bank { A, B };

enum class StereoMode { Stereo, MidSide, LeftRight };

enum class HQMode { Off, Oversampling2x };

enum class EditTarget { Link, A, B };

class Params final {
  public:
    static constexpr int NumBands = 8;

    struct IDs {
        static constexpr const char* stereoMode = "stereo_mode";
        static constexpr const char* hqMode = "hq_mode";
        static constexpr const char* editTarget = "edit_target";
        static constexpr const char* outputGain = "out_gain";

        static juce::String enabled(int bandNum, Bank bank = Bank::A);
        static juce::String type(int bandNum, Bank bank = Bank::A);
        static juce::String freq(int bandNum, Bank bank = Bank::A);
        static juce::String gain(int bandNum, Bank bank = Bank::A);
        static juce::String q(int bandNum, Bank bank = Bank::A);
        static juce::String slope(int bandNum, Bank bank = Bank::A);
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
    StereoMode getStereoMode() const noexcept;
    HQMode getHQMode() const noexcept;
    bool isHQEnabled() const noexcept;
    EditTarget getEditTarget() const noexcept;
    const BandParams& getBand(int index, Bank bank = Bank::A) const noexcept;

    static int defaultTypeIndexForBand(int bandNum) noexcept;
    static float defaultFrequencyHzForBand(int bandNum) noexcept;
    static constexpr float defaultGainDb() noexcept { return 0.0f; }
    static constexpr float defaultQ() noexcept { return 1.0f; }
    static constexpr int defaultSlopeIndex() noexcept { return 0; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

  private:
    std::atomic<float>* editTarget_ = nullptr;
    std::atomic<float>* stereoMode_ = nullptr;
    std::atomic<float>* hqMode_ = nullptr;
    std::atomic<float>* outputGainDb_ = nullptr;
    std::array<BandParams, NumBands> bandsA_;
    std::array<BandParams, NumBands> bandsB_;
};
} // namespace util
