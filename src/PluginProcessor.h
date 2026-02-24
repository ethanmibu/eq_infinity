#pragma once

#include <JuceHeader.h>
#include "util/Params.h"

class GravityEQAudioProcessor final : public juce::AudioProcessor
{
public:
    GravityEQAudioProcessor();
    ~GravityEQAudioProcessor() override = default;

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

private:
    util::Params params_;

    juce::dsp::Gain<float> outputGain_;
    juce::dsp::ProcessSpec processSpec_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GravityEQAudioProcessor)
};
