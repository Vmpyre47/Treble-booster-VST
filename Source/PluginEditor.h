// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#pragma once
#include "PluginProcessor.h"

class CDRCLook : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
};
class RangeMasterEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit RangeMasterEditor(RangeMasterProcessor&);
    ~RangeMasterEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    RangeMasterProcessor& processor;
    CDRCLook look;
    juce::Slider boost, master;
    juce::ToggleButton power{"ON"}, material{"SILICON"};
    juce::Label boostValue, masterValue, materialValue;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> boostAttachment,masterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment, materialAttachment;
    void timerCallback() override;
    void updateLabels();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RangeMasterEditor)
};
