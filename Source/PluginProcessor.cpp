#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout RekordisMasterLimiterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>("drive", "Drive", 0.0f, 18.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("threshold", "Threshold", -24.0f, 0.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.1f, 30.0f, 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("release", "Release", 50.0f, 2000.0f, 250.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("warmth", "Warmth", 0.0f, 100.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("ceiling", "Ceiling", -2.0f, -0.1f, -0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("makeup_gain", "Makeup Gain", -12.0f, 12.0f, 0.0f));
    return layout;
}

RekordisMasterLimiterAudioProcessor::RekordisMasterLimiterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Setup input saturator (tanh)
    inputSaturator.functionToUse = [](float x) {
        return std::tanh(x);
    };
}

RekordisMasterLimiterAudioProcessor::~RekordisMasterLimiterAudioProcessor()
{
}

const juce::String RekordisMasterLimiterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RekordisMasterLimiterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RekordisMasterLimiterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RekordisMasterLimiterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RekordisMasterLimiterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RekordisMasterLimiterAudioProcessor::getNumPrograms()
{
    return 1;
}

int RekordisMasterLimiterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RekordisMasterLimiterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RekordisMasterLimiterAudioProcessor::getProgramName (int index)
{
    return {};
}

void RekordisMasterLimiterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void RekordisMasterLimiterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    inputSaturator.prepare(spec);
    
    optoCompressor.prepare(spec);
    
    softClipper.prepare(spec);
    softClipper.setRatio(100.0f); // Limit
    softClipper.setAttack(0.1f);
    softClipper.setRelease(10.0f);
}

void RekordisMasterLimiterAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RekordisMasterLimiterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RekordisMasterLimiterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Load Parameters
    float drive = apvts.getRawParameterValue("drive")->load();
    float threshold = apvts.getRawParameterValue("threshold")->load();
    float attack = apvts.getRawParameterValue("attack")->load();
    float release = apvts.getRawParameterValue("release")->load();
    float ceiling = apvts.getRawParameterValue("ceiling")->load();
    float makeup = apvts.getRawParameterValue("makeup_gain")->load();

    float driveLinear = juce::Decibels::decibelsToGain(drive);
    float makeupLinear = juce::Decibels::decibelsToGain(makeup);

    optoCompressor.setThreshold(threshold);
    optoCompressor.setAttack(attack);
    optoCompressor.setRelease(release);
    optoCompressor.setRatio(4.0f); // Gentle glue ratio

    softClipper.setThreshold(ceiling - 0.2f); // close to ceiling

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Apply Drive
    buffer.applyGain(driveLinear);

    // Saturation Stage 
    inputSaturator.process(context);

    // Opto Compressor Stage
    optoCompressor.process(context);

    // Soft Clipper / Limiter
    softClipper.process(context);

    // Apply Makeup
    buffer.applyGain(makeupLinear);

    // Calculate Gain Reduction for VU Meter (approx)
    // In a real scenario you would get this from the compressor state, but for now we'll calculate RMS
    float curRms = 0.0f;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        curRms += buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
    }
    curRms /= (float)totalNumInputChannels;
    
    // Simulate simple reduction visual
    float currentLevel = juce::Decibels::gainToDecibels(curRms);
    float reduction = std::min(0.0f, threshold - currentLevel);
    
    // Smooth the meter value
    float meterValue = gainReductionLevel.load();
    meterValue = meterValue * 0.8f + reduction * 0.2f;
    gainReductionLevel.store(meterValue);
}

bool RekordisMasterLimiterAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* RekordisMasterLimiterAudioProcessor::createEditor()
{
    return new RekordisMasterLimiterAudioProcessorEditor (*this);
}

void RekordisMasterLimiterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RekordisMasterLimiterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RekordisMasterLimiterAudioProcessor();
}
