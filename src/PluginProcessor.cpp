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
     ), params_(*this)
 {
 }

 const juce::String EQInfinityAudioProcessor::getName() const
 {
     return JucePlugin_Name;
 }

 bool EQInfinityAudioProcessor::acceptsMidi() const { return false; }
 bool EQInfinityAudioProcessor::producesMidi() const { return false; }
 bool EQInfinityAudioProcessor::isMidiEffect() const { return false; }
 double EQInfinityAudioProcessor::getTailLengthSeconds() const { return 0.0; }

 int EQInfinityAudioProcessor::getNumPrograms() { return 1; }
 int EQInfinityAudioProcessor::getCurrentProgram() { return 0; }
 void EQInfinityAudioProcessor::setCurrentProgram(int) {}
 const juce::String EQInfinityAudioProcessor::getProgramName(int) { return {}; }
 void EQInfinityAudioProcessor::changeProgramName(int, const juce::String&) {}

 void EQInfinityAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
 {
     processSpec_.sampleRate = sampleRate;
     processSpec_.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
     processSpec_.numChannels = static_cast<juce::uint32>(juce::jmax(getTotalNumInputChannels(),
                                                                     getTotalNumOutputChannels()));

     outputGain_.reset();
     outputGain_.prepare(processSpec_);
     outputGain_.setRampDurationSeconds(0.02); // smoothing

     // Initialize from current parameter value (no allocations)
     const auto gainDb = params_.getOutputGainDb();
     outputGain_.setGainDecibels(gainDb);
 }

 void EQInfinityAudioProcessor::releaseResources()
 {
 }

#if !JucePlugin_PreferredChannelConfigurations
 bool EQInfinityAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
 {
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

 void EQInfinityAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
 {
     juce::ignoreUnused(midiMessages);

     juce::ScopedNoDenormals noDenormals;

     const int totalNumInputChannels = getTotalNumInputChannels();
     const int totalNumOutputChannels = getTotalNumOutputChannels();

     for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
         buffer.clear(ch, 0, buffer.getNumSamples());

     // No locks, no allocations: read atomic parameter and apply
     outputGain_.setGainDecibels(params_.getOutputGainDb());

     juce::dsp::AudioBlock<float> block(buffer);
     juce::dsp::ProcessContextReplacing<float> ctx(block);
     outputGain_.process(ctx);
 }

 bool EQInfinityAudioProcessor::hasEditor() const { return true; }

 juce::AudioProcessorEditor* EQInfinityAudioProcessor::createEditor()
 {
     return new EQInfinityAudioProcessorEditor(*this);
 }

 void EQInfinityAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
 {
     // APVTS -> XML -> binary
     auto state = params_.apvts.copyState();
     std::unique_ptr<juce::XmlElement> xml(state.createXml());
     copyXmlToBinary(*xml, destData);
 }

 void EQInfinityAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
 {
     std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
     if (xmlState == nullptr)
         return;

     if (!xmlState->hasTagName(params_.apvts.state.getType()))
         return;

     params_.apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
 }

 // This creates new instances of the plugin.
 juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
 {
     return new EQInfinityAudioProcessor();
 }
