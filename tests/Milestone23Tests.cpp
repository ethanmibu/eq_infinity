#include "../src/dsp/EqBand.h"
#include "../src/util/Params.h"
#include <array>
#include <atomic>
#include <cmath>
#include <iostream>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <string>

namespace {
class DummyProcessor final : public juce::AudioProcessor {
  public:
    DummyProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("In", juce::AudioChannelSet::stereo(), true)
                             .withOutput("Out", juce::AudioChannelSet::stereo(), true)) {}

    const juce::String getName() const override { return "DummyProcessor"; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    bool hasEditor() const override { return false; }

    bool acceptsMidi() const override { return false; }

    bool producesMidi() const override { return false; }

    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }

    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int) override {}

    const juce::String getProgramName(int) override { return {}; }

    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

struct BandStorage {
    std::atomic<float> enabled{1.0f};
    std::atomic<float> type{3.0f}; // HighPass
    std::atomic<float> freq{1000.0f};
    std::atomic<float> gain{0.0f};
    std::atomic<float> q{0.707f};
    std::atomic<float> slope{0.0f};

    util::Params::BandParams asParams() noexcept {
        util::Params::BandParams params;
        params.enabled = &enabled;
        params.type = &type;
        params.freq = &freq;
        params.gain = &gain;
        params.q = &q;
        params.slope = &slope;
        return params;
    }
};

bool expect(bool condition, const std::string& message) {
    if (condition)
        return true;

    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool testParamsIncludeMilestone2Ids() {
    DummyProcessor processor;
    util::Params params(processor);

    bool ok = expect(params.apvts.getParameter(util::Params::IDs::outputGain) != nullptr, "Missing out_gain parameter");

    for (int bandNum = 1; bandNum <= util::Params::NumBands; ++bandNum) {
        ok &= expect(params.apvts.getParameter(util::Params::IDs::enabled(bandNum)) != nullptr,
                     "Missing enabled parameter for band " + std::to_string(bandNum));
        ok &= expect(params.apvts.getParameter(util::Params::IDs::type(bandNum)) != nullptr,
                     "Missing type parameter for band " + std::to_string(bandNum));
        ok &= expect(params.apvts.getParameter(util::Params::IDs::freq(bandNum)) != nullptr,
                     "Missing freq parameter for band " + std::to_string(bandNum));
        ok &= expect(params.apvts.getParameter(util::Params::IDs::gain(bandNum)) != nullptr,
                     "Missing gain parameter for band " + std::to_string(bandNum));
        ok &= expect(params.apvts.getParameter(util::Params::IDs::q(bandNum)) != nullptr,
                     "Missing q parameter for band " + std::to_string(bandNum));
        ok &= expect(params.apvts.getParameter(util::Params::IDs::slope(bandNum)) != nullptr,
                     "Missing slope parameter for band " + std::to_string(bandNum));
    }

    return ok;
}

bool testEqBandProcessesAllChannels() {
    ::dsp::EqBand band;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = 48000.0;
    spec.maximumBlockSize = 64;
    spec.numChannels = 2;
    band.prepare(spec);

    BandStorage storage;
    auto params = storage.asParams();
    band.updateCoefficients(params, spec.sampleRate);

    juce::AudioBuffer<float> buffer(2, 64);
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, 1.0f);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    band.process(context);

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    bool channelsMatch = true;
    bool signalChanged = false;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        const float leftValue = left[sample];
        const float rightValue = right[sample];

        if (std::abs(leftValue - rightValue) > 1.0e-4f)
            channelsMatch = false;

        if (std::abs(leftValue - 1.0f) > 1.0e-4f)
            signalChanged = true;
    }

    return expect(channelsMatch, "EqBand should process all channels identically for identical input") &&
           expect(signalChanged, "High-pass filter should alter a DC block");
}
} // namespace

int main() {
    bool ok = true;
    ok &= testParamsIncludeMilestone2Ids();
    ok &= testEqBandProcessesAllChannels();

    if (!ok)
        return 1;

    std::cout << "All Milestone 2/3 tests passed.\n";
    return 0;
}
