#pragma once

#include "dsp/EqEngine.h"
#include "util/Params.h"
#include <JuceHeader.h>
#include <array>

class EQInfinityAudioProcessor final : public juce::AudioProcessor {
  public:
    EQInfinityAudioProcessor();
    ~EQInfinityAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#if !JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    util::Params& params() noexcept { return params_; }
    const util::Params& params() const noexcept { return params_; }

    int popPreAnalyzerSamples(float* destination, int maxSamples) noexcept;
    int popPostAnalyzerSamples(float* destination, int maxSamples) noexcept;
    void setSoloBandIndex(int index) noexcept;
    void clearSoloBand() noexcept;

    util::Params params_;

  private:
    class AnalyzerFifo final {
      public:
        static constexpr int Capacity = 1 << 15;

        AnalyzerFifo() : fifo_(Capacity) {}

        void clear() noexcept { fifo_.reset(); }

        void push(const float* samples, int numSamples) noexcept {
            if (samples == nullptr || numSamples <= 0)
                return;

            const int writableSamples = juce::jmin(numSamples, fifo_.getFreeSpace());
            if (writableSamples <= 0)
                return;

            const auto write = fifo_.write(writableSamples);
            juce::FloatVectorOperations::copy(buffer_.data() + write.startIndex1, samples, write.blockSize1);
            if (write.blockSize2 > 0) {
                juce::FloatVectorOperations::copy(buffer_.data() + write.startIndex2, samples + write.blockSize1,
                                                  write.blockSize2);
            }
        }

        int pull(float* destination, int maxSamples) noexcept {
            if (destination == nullptr || maxSamples <= 0)
                return 0;

            const int readableSamples = juce::jmin(maxSamples, fifo_.getNumReady());
            if (readableSamples <= 0)
                return 0;

            const auto read = fifo_.read(readableSamples);
            juce::FloatVectorOperations::copy(destination, buffer_.data() + read.startIndex1, read.blockSize1);
            if (read.blockSize2 > 0) {
                juce::FloatVectorOperations::copy(destination + read.blockSize1, buffer_.data() + read.startIndex2,
                                                  read.blockSize2);
            }

            return readableSamples;
        }

      private:
        juce::AbstractFifo fifo_;
        std::array<float, Capacity> buffer_{};
    };

    ::dsp::EqEngine eqEngineA_;
    ::dsp::EqEngine eqEngineB_;
    juce::dsp::Gain<float> outputGain_;
    juce::dsp::ProcessSpec processSpec_{};
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2x_;
    AnalyzerFifo preAnalyzerFifo_;
    AnalyzerFifo postAnalyzerFifo_;
    std::atomic<int> soloBandIndex_{-1};

    static void encodeMidSide(juce::AudioBuffer<float>& buffer) noexcept;
    static void decodeMidSide(juce::AudioBuffer<float>& buffer) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQInfinityAudioProcessor)
};
