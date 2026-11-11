// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#include "PluginProcessor.h"
#include "PluginEditor.h"

RangeMasterProcessor::RangeMasterProcessor()
 : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
     .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 parameters(*this,nullptr,"RANGEMASTER_STATE",makeParameters()) {}

juce::AudioProcessorValueTreeState::ParameterLayout RangeMasterProcessor::makeParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"boost",1},"Boost",
        juce::NormalisableRange<float>(0,1),0.75f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"master",1},"Master",
        juce::NormalisableRange<float>(-60,6,0.01f),-18.0f,juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"material",1},"Transistor",
        juce::StringArray{"Germanium OC44","Silicon"},0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"enabled",1},"On",true));
    return p;
}
bool RangeMasterProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    const auto n=b.getMainOutputChannelSet();
    return (n==juce::AudioChannelSet::mono()||n==juce::AudioChannelSet::stereo())&&n==b.getMainInputChannelSet();
}
void RangeMasterProcessor::prepareToPlay(double fs,int maxBlock) {
    chunkSize=juce::jmax(1,maxBlock);
    oversampling=std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(getTotalNumInputChannels()),2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true);
    oversampling->initProcessing(static_cast<size_t>(chunkSize));
    oversampling->reset();
    setLatencySamples(juce::roundToInt(oversampling->getLatencyInSamples()));
    for(auto& ch:circuits) {
        const auto initialBoost=parameters.getRawParameterValue("boost")->load();
        ch[0].prepare(fs*4,rm::Transistor::germanium(),initialBoost);
        ch[1].prepare(fs*4,rm::Transistor::silicon(),initialBoost);
    }
    for(auto* s:{&boost,&master,&mode,&enabled}) s->reset(fs*4,0.02);
    boost.setCurrentAndTargetValue(parameters.getRawParameterValue("boost")->load());
    master.setCurrentAndTargetValue(juce::Decibels::decibelsToGain<double>(parameters.getRawParameterValue("master")->load()));
    mode.setCurrentAndTargetValue(parameters.getRawParameterValue("material")->load());
    enabled.setCurrentAndTargetValue(parameters.getRawParameterValue("enabled")->load());
    peak=0;
}
void RangeMasterProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    if(!oversampling) { buffer.clear(); return; }
    boost.setTargetValue(parameters.getRawParameterValue("boost")->load());
    master.setTargetValue(juce::Decibels::decibelsToGain<double>(parameters.getRawParameterValue("master")->load()));
    mode.setTargetValue(parameters.getRawParameterValue("material")->load());
    enabled.setTargetValue(parameters.getRawParameterValue("enabled")->load());
    const auto channels=static_cast<size_t>(getTotalNumInputChannels());
    for(int c=getTotalNumInputChannels();c<buffer.getNumChannels();++c) buffer.clear(c,0,buffer.getNumSamples());
    float blockPeak=0;
    for(int start=0;start<buffer.getNumSamples();start+=chunkSize) {
        auto block=juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0,channels)
            .getSubBlock(static_cast<size_t>(start),static_cast<size_t>(juce::jmin(chunkSize,buffer.getNumSamples()-start)));
        for(size_t c=0;c<channels;++c) for(size_t i=0;i<block.getNumSamples();++i)
            if(!std::isfinite(block.getSample(static_cast<int>(c),static_cast<int>(i)))) block.setSample(static_cast<int>(c),static_cast<int>(i),0);
        auto up=oversampling->processSamplesUp(block);
        for(size_t i=0;i<up.getNumSamples();++i) {
            const auto b=boost.getNextValue(), g=master.getNextValue(), m=mode.getNextValue(), wet=enabled.getNextValue();
            for(size_t c=0;c<channels;++c) {
                const double dry=up.getSample(static_cast<int>(c),static_cast<int>(i));
                // Digital 1.0 corresponds to 1 V peak at the pedal input.
                const double in=std::clamp(dry,-4.0,4.0);
                const double ge=circuits[c][0].tick(in,b), si=circuits[c][1].tick(in,b);
                up.setSample(static_cast<int>(c),static_cast<int>(i),static_cast<float>(dry*(1-wet)+(ge+(si-ge)*m)*g*wet));
            }
        }
        oversampling->processSamplesDown(block);
        for(size_t c=0;c<channels;++c) for(size_t i=0;i<block.getNumSamples();++i)
            blockPeak=juce::jmax(blockPeak,std::abs(block.getSample(static_cast<int>(c),static_cast<int>(i))));
    }
    peak.store(blockPeak);
}
uint64_t RangeMasterProcessor::solverFailures() const {
    uint64_t n=0; for(const auto& c:circuits) for(const auto& q:c) n+=q.failures(); return n;
}
void RangeMasterProcessor::getStateInformation(juce::MemoryBlock& data) {
    if(auto xml=parameters.copyState().createXml()) copyXmlToBinary(*xml,data);
}
void RangeMasterProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* RangeMasterProcessor::createEditor() { return new RangeMasterEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RangeMasterProcessor(); }
