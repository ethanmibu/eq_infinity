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

void fillSine(juce::AudioBuffer<float>& buffer, double sampleRate, double frequencyHz, double& phase) {
    const double phaseIncrement = juce::MathConstants<double>::twoPi * (frequencyHz / sampleRate);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        const float value = static_cast<float>(std::sin(phase));
        phase += phaseIncrement;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

float computeRms(const juce::AudioBuffer<float>& buffer, int channel) {
    const float* data = buffer.getReadPointer(channel);
    double sumSquares = 0.0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        const double value = static_cast<double>(data[sample]);
        sumSquares += value * value;
    }

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(buffer.getNumSamples())));
}

float processToneAndMeasureRms(::dsp::EqBand& band, util::Params::BandParams params, double sampleRate, double frequencyHz,
                               int blocksToProcess) {
    constexpr int blockSize = 256;
    juce::AudioBuffer<float> buffer(2, blockSize);
    double phase = 0.0;
    float totalRms = 0.0f;

    for (int block = 0; block < blocksToProcess; ++block) {
        fillSine(buffer, sampleRate, frequencyHz, phase);
        band.updateCoefficients(params, sampleRate, blockSize);

        juce::dsp::AudioBlock<float> blockView(buffer);
        juce::dsp::ProcessContextReplacing<float> context(blockView);
        band.process(context);

        totalRms += computeRms(buffer, 0);
    }

    return totalRms / static_cast<float>(blocksToProcess);
}

bool testParamsIncludeMilestone2Ids() {
    DummyProcessor processor;
    util::Params params(processor);

    bool ok = expect(params.apvts.getParameter(util::Params::IDs::editTarget) != nullptr,
                     "Missing edit_target parameter");
    ok &= expect(params.apvts.getParameter(util::Params::IDs::stereoMode) != nullptr, "Missing stereo_mode parameter");
    ok &= expect(params.apvts.getParameter(util::Params::IDs::hqMode) != nullptr, "Missing hq_mode parameter");
    ok &= expect(params.apvts.getParameter(util::Params::IDs::outputGain) != nullptr, "Missing out_gain parameter");

    for (int bandNum = 1; bandNum <= util::Params::NumBands; ++bandNum) {
        for (const auto bank : {util::Bank::A, util::Bank::B}) {
            const auto bankLabel = bank == util::Bank::A ? "A" : "B";
            ok &= expect(params.apvts.getParameter(util::Params::IDs::enabled(bandNum, bank)) != nullptr,
                         "Missing enabled parameter for band " + std::to_string(bandNum) + " bank " + bankLabel);
            ok &= expect(params.apvts.getParameter(util::Params::IDs::type(bandNum, bank)) != nullptr,
                         "Missing type parameter for band " + std::to_string(bandNum) + " bank " + bankLabel);
            ok &= expect(params.apvts.getParameter(util::Params::IDs::freq(bandNum, bank)) != nullptr,
                         "Missing freq parameter for band " + std::to_string(bandNum) + " bank " + bankLabel);
            ok &= expect(params.apvts.getParameter(util::Params::IDs::gain(bandNum, bank)) != nullptr,
                         "Missing gain parameter for band " + std::to_string(bandNum) + " bank " + bankLabel);
            ok &= expect(params.apvts.getParameter(util::Params::IDs::q(bandNum, bank)) != nullptr,
                         "Missing q parameter for band " + std::to_string(bandNum) + " bank " + bankLabel);
            ok &= expect(params.apvts.getParameter(util::Params::IDs::slope(bandNum, bank)) != nullptr,
                         "Missing slope parameter for band " + std::to_string(bandNum) + " bank " + bankLabel);
        }
    }

    return ok;
}

bool testGlobalModesDefaultToLRAndEco() {
    DummyProcessor processor;
    util::Params params(processor);

    return expect(params.getStereoMode() == util::StereoMode::Stereo, "Stereo mode should default to Stereo") &&
           expect(params.getEditTarget() == util::EditTarget::Link, "Edit target should default to Link") &&
           expect(params.getHQMode() == util::HQMode::Off, "HQ mode should default to Eco/Off");
}

bool testCutBandsDisabledByDefault() {
    DummyProcessor processor;
    util::Params params(processor);

    const auto& band1 = params.getBand(0);
    const auto& band8 = params.getBand(util::Params::NumBands - 1);
    const auto& band1B = params.getBand(0, util::Bank::B);
    const auto& band8B = params.getBand(util::Params::NumBands - 1, util::Bank::B);

    const bool band1Enabled = band1.enabled->load(std::memory_order_relaxed) > 0.5f;
    const bool band8Enabled = band8.enabled->load(std::memory_order_relaxed) > 0.5f;
    const bool band1BEnabled = band1B.enabled->load(std::memory_order_relaxed) > 0.5f;
    const bool band8BEnabled = band8B.enabled->load(std::memory_order_relaxed) > 0.5f;

    return expect(!band1Enabled, "Band 1 (default High Pass) should be disabled by default") &&
           expect(!band8Enabled, "Band 8 (default Low Pass) should be disabled by default") &&
           expect(!band1BEnabled, "Band 1 bank B should be disabled by default") &&
           expect(!band8BEnabled, "Band 8 bank B should be disabled by default");
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
    band.updateCoefficients(params, spec.sampleRate, static_cast<int>(spec.maximumBlockSize));

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

bool testLowPassCutoffRespondsToFrequencyChanges() {
    ::dsp::EqBand band;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = 48000.0;
    spec.maximumBlockSize = 256;
    spec.numChannels = 2;
    band.prepare(spec);

    BandStorage storage;
    storage.type.store(4.0f); // LowPass
    storage.q.store(0.707f);
    storage.slope.store(0.0f);

    auto params = storage.asParams();
    constexpr double testToneHz = 8000.0;
    constexpr int blocks = 12;

    storage.freq.store(500.0f);
    const float lowCutoffRms = processToneAndMeasureRms(band, params, spec.sampleRate, testToneHz, blocks);

    storage.freq.store(10000.0f);
    const float highCutoffRms = processToneAndMeasureRms(band, params, spec.sampleRate, testToneHz, blocks);

    return expect(highCutoffRms > lowCutoffRms * 2.0f,
                  "Low-pass frequency changes should significantly alter 8 kHz attenuation");
}

bool testPeakBandRespondsToGainChanges() {
    ::dsp::EqBand band;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = 48000.0;
    spec.maximumBlockSize = 256;
    spec.numChannels = 2;
    band.prepare(spec);

    BandStorage storage;
    storage.type.store(0.0f); // Peak
    storage.freq.store(1000.0f);
    storage.q.store(1.2f);
    storage.slope.store(0.0f);

    auto params = storage.asParams();
    constexpr double testToneHz = 1000.0;
    constexpr int blocks = 12;

    storage.gain.store(0.0f);
    const float unityRms = processToneAndMeasureRms(band, params, spec.sampleRate, testToneHz, blocks);

    storage.gain.store(18.0f);
    const float boostedRms = processToneAndMeasureRms(band, params, spec.sampleRate, testToneHz, blocks);

    return expect(boostedRms > unityRms * 1.5f, "Peak gain changes should audibly boost a tone near center frequency");
}
} // namespace

int main() {
    bool ok = true;
    ok &= testParamsIncludeMilestone2Ids();
    ok &= testGlobalModesDefaultToLRAndEco();
    ok &= testCutBandsDisabledByDefault();
    ok &= testEqBandProcessesAllChannels();
    ok &= testLowPassCutoffRespondsToFrequencyChanges();
    ok &= testPeakBandRespondsToGainChanges();

    if (!ok)
        return 1;

    std::cout << "All Milestone 2/3 tests passed.\n";
    return 0;
}
