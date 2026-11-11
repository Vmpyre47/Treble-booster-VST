// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#include "PluginEditor.h"

namespace {
const auto ink=juce::Colour::fromRGB(38,44,43);
const auto cream=juce::Colour::fromRGB(224,222,208);
void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size,
          juce::Justification align=juce::Justification::centred, bool bold=false) {
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions("Arial",size,bold?juce::Font::bold:juce::Font::plain)));
    g.drawText(s,r,align,false);
}
void screw(juce::Graphics& g,float x,float y) {
    g.setColour(juce::Colours::black.withAlpha(0.25f)); g.fillEllipse(x-6,y-5,13,13);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffdeded4),x-5,y-5,juce::Colour(0xff787f79),x+5,y+5,false));
    g.fillEllipse(x-5,y-5,10,10);
    g.setColour(juce::Colour(0xff545b56)); g.drawLine(x-3,y+2,x+3,y-2,1.4f);
}
}
void CDRCLook::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                                    float pos,float start,float end,juce::Slider&) {
    const float radius=juce::jmin(width,height)*0.38f;
    const float cx=x+width*0.5f,cy=y+height*0.5f;
    const float a=start+pos*(end-start);
    for(int i=0;i<=10;++i) {
        const auto angle=start+(end-start)*i/10.0f;
        g.setColour(ink.withAlpha(i%5==0?0.8f:0.35f));
        g.drawLine(cx+std::sin(angle)*(radius+10),cy-std::cos(angle)*(radius+10),
                   cx+std::sin(angle)*(radius+15),cy-std::cos(angle)*(radius+15),i%5==0?2.0f:1.0f);
    }
    g.setColour(juce::Colours::black.withAlpha(0.20f)); g.fillEllipse(cx-radius-3,cy-radius+4,2*radius+6,2*radius+6);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4e5450),cx-radius,cy-radius,
                        juce::Colour(0xff0f1413),cx+radius,cy+radius,false));
    g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    for(int i=0;i<64;++i) {
        float t=juce::MathConstants<float>::twoPi*i/64;
        g.setColour((i%2?juce::Colours::white:juce::Colours::black).withAlpha(0.16f));
        g.drawLine(cx+std::sin(t)*(radius-8),cy-std::cos(t)*(radius-8),cx+std::sin(t)*(radius-1),cy-std::cos(t)*(radius-1),1);
    }
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff353b37),cx,cy-radius,juce::Colour(0xff1a201e),cx,cy+radius,false));
    g.fillEllipse(cx-radius+9,cy-radius+9,2*radius-18,2*radius-18);
    g.setColour(cream);
    g.drawLine(cx+std::sin(a)*(radius*0.47f),cy-std::cos(a)*(radius*0.47f),
               cx+std::sin(a)*(radius-11),cy-std::cos(a)*(radius-11),3.5f);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1);
}
void CDRCLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool over,bool) {
    auto r=b.getLocalBounds().toFloat().reduced(3);
    g.setColour(juce::Colour(0xff6d756d)); g.fillRoundedRectangle(r,3);
    r=r.reduced(3); g.setColour(juce::Colour(0xff171f1d)); g.fillRoundedRectangle(r,2);
    auto h=r.reduced(4); h.setWidth(h.getWidth()*0.46f);
    if(b.getToggleState()) h.setX(r.getRight()-h.getWidth()-4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(over?0xff7c8377:0xff626b61),h.getX(),h.getY(),
                        juce::Colour(0xff343e36),h.getX(),h.getBottom(),false));
    g.fillRoundedRectangle(h,2);
    g.setColour(juce::Colour(0xff1e2822));
    for(float y=h.getY()+5;y<h.getBottom()-2;y+=4) g.drawHorizontalLine(static_cast<int>(y),h.getX()+3,h.getRight()-3);
}
RangeMasterEditor::RangeMasterEditor(RangeMasterProcessor& p):AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&look);
    for(auto* slider:{&boost,&master}) {
        addAndMakeVisible(*slider);
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        slider->setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
        slider->onValueChange=[this]{updateLabels();};
    }
    boost.setName("Boost"); boost.setTitle("Original 10k audio-taper collector potentiometer");
    boost.setTooltip("Original 10 kOhm output wiper. Clockwise = louder; fully left = mute. Not an input drive control.");
    master.setName("Master"); master.setTooltip("Output level after the original circuit, -60 to +6 dB.");
    boostAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"boost",boost);
    masterAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"master",master);
    boost.setDoubleClickReturnValue(true,0.75); master.setDoubleClickReturnValue(true,-18);
    addAndMakeVisible(power); addAndMakeVisible(material);
    power.setTooltip("Effect on / bypass (20 ms crossfade)");
    material.setTooltip("Germanium OC44 / silicon transistor in the same original circuit");
    powerAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"enabled",power);
    materialAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"material",material);
    addAndMakeVisible(materialValue);
    materialValue.setJustificationType(juce::Justification::centred);
    materialValue.setFont(juce::Font(juce::FontOptions("Arial",10,juce::Font::bold)));
    materialValue.setColour(juce::Label::textColourId,ink);
    materialValue.setText(material.getToggleState()?"Si SELECTED":"Ge / OC44 SELECTED",juce::dontSendNotification);
    for(auto* label:{&boostValue,&masterValue}) {
        addAndMakeVisible(*label); label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(juce::FontOptions("Courier New",15,juce::Font::plain)));
        label->setColour(juce::Label::textColourId,ink);
    }
    setSize(760,500);
    updateLabels(); startTimerHz(20);
}
RangeMasterEditor::~RangeMasterEditor() { setLookAndFeel(nullptr); }
void RangeMasterEditor::updateLabels() {
    boostValue.setText(juce::String(juce::roundToInt(boost.getValue()*100))+" %",juce::dontSendNotification);
    masterValue.setText(juce::String(master.getValue(),1)+" dB",juce::dontSendNotification);
}
void RangeMasterEditor::timerCallback() {
    materialValue.setText(material.getToggleState()?"Si SELECTED":"Ge / OC44 SELECTED",juce::dontSendNotification);
    repaint(650,406,50,20);
}
void RangeMasterEditor::resized() {
    boost.setBounds(216,210,190,182); master.setBounds(460,223,154,154);
    boostValue.setBounds(245,385,132,26); masterValue.setBounds(471,385,132,26);
    power.setBounds(81,284,70,40); material.setBounds(496,153,78,35);
    materialValue.setBounds(466,186,145,14);
}
void RangeMasterEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff171e1b));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffa9b4a9),0,10,juce::Colour(0xff59695f),760,490,false));
    g.fillRoundedRectangle(15,15,730,470,10);
    // Subtle deterministic enamel grain, generated directly by the UI.
    juce::Random grain(1965);
    for(int i=0;i<9000;++i) {
        g.setColour(juce::Colours::white.withAlpha(grain.nextFloat()*0.035f));
        g.fillRect(19+grain.nextInt(722),19+grain.nextInt(462),1,1);
    }
    g.setColour(juce::Colour(0xff37463b)); g.drawRoundedRectangle(15,15,730,470,10,2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe4e5da),30,28,juce::Colour(0xffbec7bd),725,470,false));
    g.fillRoundedRectangle(30,28,700,442,5);
    g.setColour(juce::Colours::white.withAlpha(0.6f)); g.drawRoundedRectangle(31,29,698,440,5,1);
    for(auto p:{juce::Point<float>(49,48),{711,48},{49,451},{711,451}}) screw(g,p.x,p.y);
    text(g,"RANGEMASTER",{70,59,422,49},43,juce::Justification::centredLeft);
    text(g,"T R E B L E   B O O S T E R",{72,110,395,22},15,juce::Justification::centredLeft);
    text(g,"C I R C U I T   S E R I E S   /   0 1",{73,153,335,18},11,juce::Justification::centredLeft);
    // Original amp-top unit's rising musical-staff motif, redrawn in code.
    g.setColour(ink);
    for(int i=0;i<5;++i) g.drawLine(529,76+9.0f*i,675,60+9.0f*i,1.5f);
    for(int i=0;i<4;++i) {
        float x=549+32.0f*i,y=102-13.0f*i;
        g.fillEllipse(x,y,12,7); g.drawLine(x+11,y+3,x+11,y-25,2);
    }
    text(g,"GERMANIUM",{432,159,64,20},9);
    text(g,"SILICON",{579,159,67,20},10);
    text(g,"OFF",{73,262,30,18},10); text(g,"ON",{129,262,28,18},10);
    text(g,"B O O S T",{228,201,166,24},16);
    text(g,"M A S T E R",{455,201,166,24},16);
    text(g,"SET",{284,418,55,19},11);
    text(g,"OUTPUT",{497,418,80,19},11);
    // Hexagonal guitar socket evokes the original front panel.
    juce::Path hex;
    for(int i=0;i<6;++i) {
        float a=juce::MathConstants<float>::twoPi*i/6;
        auto x=116+23*std::cos(a),y=375+23*std::sin(a);
        if(i==0) hex.startNewSubPath(x,y); else hex.lineTo(x,y);
    }
    hex.closeSubPath();
    g.setGradientFill(juce::ColourGradient(juce::Colours::white,100,355,juce::Colour(0xff68746c),133,397,false)); g.fillPath(hex);
    g.setColour(juce::Colour(0xff58655d)); g.fillEllipse(99,358,34,34);
    g.setColour(juce::Colour(0xff101b15)); g.fillEllipse(104,363,24,24);
    g.setColour(juce::Colour(0xff9ca79c)); g.drawEllipse(104,363,24,24,2);
    text(g,"GUITAR",{79,411,74,18},11);
    text(g,"c.d.r.c plugins  /  1.0.2",{75,448,270,13},10,juce::Justification::centredLeft);
    text(g,"9V  /  5nF  /  4x",{510,448,175,13},10,juce::Justification::centredRight);
    const float level=processor.peak.load();
    g.setColour(level>=1.0f?juce::Colour(0xffb2402d):juce::Colour(0xff4f6551));
    g.fillEllipse(666,412,9,9);
    text(g,"PEAK",{651,426,40,13},8);
}
