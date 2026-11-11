// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Circuit.h"

class RangeMasterProcessor : public juce::AudioProcessor {
public:
    RangeMasterProcessor();
    const juce::String getName() const override { return "c.d.r.c plugins - RangeMaster"; }
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.25; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> peak{0};
    uint64_t solverFailures() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::array<std::array<rm::Circuit,2>,2> circuits;
    juce::SmoothedValue<double> boost, master, mode, enabled;
    int chunkSize=512;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RangeMasterProcessor)
};
