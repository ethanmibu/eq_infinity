#include "PluginProcessor.h"
#include "PluginEditor.h"

EQInfinityAudioProcessor::EQInfinityAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      params_(*this) {
}

const juce::String EQInfinityAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool EQInfinityAudioProcessor::acceptsMidi() const {
    return false;
}
bool EQInfinityAudioProcessor::producesMidi() const {
    return false;
}
bool EQInfinityAudioProcessor::isMidiEffect() const {
    return false;
}
double EQInfinityAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int EQInfinityAudioProcessor::getNumPrograms() {
    return 1;
}
int EQInfinityAudioProcessor::getCurrentProgram() {
    return 0;
}
void EQInfinityAudioProcessor::setCurrentProgram(int) {}
const juce::String EQInfinityAudioProcessor::getProgramName(int) {
    return {};
}
void EQInfinityAudioProcessor::changeProgramName(int, const juce::String&) {}

void EQInfinityAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    processSpec_.sampleRate = sampleRate;
    processSpec_.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    processSpec_.numChannels =
        static_cast<juce::uint32>(juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels()));

    eqEngineA_.prepare(processSpec_);
    eqEngineB_.prepare(processSpec_);

    const auto numProcessingChannels =
        static_cast<std::size_t>(juce::jmax(1, static_cast<int>(processSpec_.numChannels)));
    oversampling2x_ = std::make_unique<juce::dsp::Oversampling<float>>(
        numProcessingChannels, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling2x_->reset();
    oversampling2x_->initProcessing(static_cast<std::size_t>(processSpec_.maximumBlockSize));

    outputGain_.reset();
    outputGain_.prepare(processSpec_);
    outputGain_.setRampDurationSeconds(0.02); // smoothing

    // Initialize from current parameter value (no allocations)
    const auto gainDb = params_.getOutputGainDb();
    outputGain_.setGainDecibels(gainDb);

    preAnalyzerFifo_.clear();
    postAnalyzerFifo_.clear();
}

void EQInfinityAudioProcessor::releaseResources() {
    if (oversampling2x_ != nullptr)
        oversampling2x_->reset();

    eqEngineA_.reset();
    eqEngineB_.reset();

    preAnalyzerFifo_.clear();
    postAnalyzerFifo_.clear();
}

#if !JucePlugin_PreferredChannelConfigurations
bool EQInfinityAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (mainIn != mainOut)
        return false;
#endif

    return true;
}
#endif

void EQInfinityAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    if (totalNumInputChannels > 0)
        preAnalyzerFifo_.push(buffer.getReadPointer(0), buffer.getNumSamples());

    const bool canUseMidSide = totalNumInputChannels >= 2 && totalNumOutputChannels >= 2;
    const bool useMidSide = canUseMidSide && params_.getStereoMode() == util::StereoMode::MidSide;
    const bool useLeftRight = canUseMidSide && params_.getStereoMode() == util::StereoMode::LeftRight;
    const bool useHQ = oversampling2x_ != nullptr && params_.isHQEnabled();
    const int soloBandIndex = soloBandIndex_.load(std::memory_order_relaxed);

    eqEngineA_.setSoloBandIndex(soloBandIndex);
    eqEngineB_.setSoloBandIndex(soloBandIndex);

    if (useMidSide)
        encodeMidSide(buffer);

    auto processMode = [&](juce::dsp::AudioBlock<float> block, int numSamples, double effectiveSampleRate) {
        // No locks, no allocations: read atomic parameters and apply.
        if (useLeftRight || useMidSide) {
            eqEngineA_.updateParameters(params_, util::Bank::A, numSamples, effectiveSampleRate);
            eqEngineB_.updateParameters(params_, util::Bank::B, numSamples, effectiveSampleRate);

            auto leftBlock = block.getSingleChannelBlock(0);
            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
            juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
            eqEngineA_.process(leftContext);
            eqEngineB_.process(rightContext);
            return;
        }

        eqEngineA_.updateParameters(params_, util::Bank::A, numSamples, effectiveSampleRate);
        juce::dsp::ProcessContextReplacing<float> context(block);
        eqEngineA_.process(context);
    };

    if (useHQ) {
        juce::dsp::AudioBlock<float> block(buffer);
        auto upsampledBlock = oversampling2x_->processSamplesUp(block);
        processMode(upsampledBlock, static_cast<int>(upsampledBlock.getNumSamples()), processSpec_.sampleRate * 2.0);
        oversampling2x_->processSamplesDown(block);
    } else {
        juce::dsp::AudioBlock<float> block(buffer);
        processMode(block, buffer.getNumSamples(), processSpec_.sampleRate);
    }

    outputGain_.setGainDecibels(params_.getOutputGainDb());
    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    outputGain_.process(outputContext);

    if (useMidSide)
        decodeMidSide(buffer);

    if (totalNumOutputChannels > 0)
        postAnalyzerFifo_.push(buffer.getReadPointer(0), buffer.getNumSamples());
}

bool EQInfinityAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* EQInfinityAudioProcessor::createEditor() {
    return new EQInfinityAudioProcessorEditor(*this);
}

void EQInfinityAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // APVTS -> XML -> binary
    auto state = params_.apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EQInfinityAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    if (!xmlState->hasTagName(params_.apvts.state.getType()))
        return;

    params_.apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

int EQInfinityAudioProcessor::popPreAnalyzerSamples(float* destination, int maxSamples) noexcept {
    return preAnalyzerFifo_.pull(destination, maxSamples);
}

int EQInfinityAudioProcessor::popPostAnalyzerSamples(float* destination, int maxSamples) noexcept {
    return postAnalyzerFifo_.pull(destination, maxSamples);
}

void EQInfinityAudioProcessor::setSoloBandIndex(int index) noexcept {
    soloBandIndex_.store(juce::jlimit(-1, util::Params::NumBands - 1, index), std::memory_order_relaxed);
}

void EQInfinityAudioProcessor::clearSoloBand() noexcept {
    soloBandIndex_.store(-1, std::memory_order_relaxed);
}

void EQInfinityAudioProcessor::encodeMidSide(juce::AudioBuffer<float>& buffer) noexcept {
    if (buffer.getNumChannels() < 2)
        return;

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const float l = left[i];
        const float r = right[i];
        left[i] = 0.5f * (l + r);
        right[i] = 0.5f * (l - r);
    }
}

void EQInfinityAudioProcessor::decodeMidSide(juce::AudioBuffer<float>& buffer) noexcept {
    if (buffer.getNumChannels() < 2)
        return;

    float* mid = buffer.getWritePointer(0);
    float* side = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const float m = mid[i];
        const float s = side[i];
        mid[i] = m + s;
        side[i] = m - s;
    }
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new EQInfinityAudioProcessor();
}
