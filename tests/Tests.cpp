// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#include "../Source/PluginProcessor.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

namespace {
int checks=0;
void check(bool condition,const std::string& name) {
    ++checks;
    if(!condition) throw std::runtime_error(name);
    std::cout<<"PASS "<<name<<'\n';
}
void set(RangeMasterProcessor& p,const char* id,float value) {
    auto* param=p.parameters.getParameter(id);
    param->setValueNotifyingHost(param->convertTo0to1(value));
}
double rms(const juce::AudioBuffer<float>& b,int channel=0) {
    return b.getRMSLevel(channel,0,b.getNumSamples());
}
double circuitGain(rm::Transistor q,double freq) {
    rm::Circuit c; c.prepare(192000,q);
    double sum=0; constexpr double amp=0.00001;
    for(int i=0;i<96000;++i) {
        auto out=c.tick(amp*std::sin(juce::MathConstants<double>::twoPi*freq*i/192000),1);
        if(i>=48000) sum+=out*out;
    }
    check(c.failures()==0,"solver convergence at "+std::to_string(freq)+" Hz");
    return std::sqrt(sum/48000)/(amp/std::sqrt(2.0));
}
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        for(auto device:{rm::Transistor::germanium(),rm::Transistor::silicon()}) {
            const auto name=device.is>1e-10?"Ge":"Si";
            rm::Circuit c; c.prepare(192000,device);
            const auto bias=c.bias();
            std::cout<<"BIAS "<<name<<" B="<<bias[0]<<" E="<<bias[1]<<" C="<<bias[2]<<" volts (polarity-normalised)\n";
            check(bias[2]>4.5&&bias[2]<8.8,"active-region DC operating point");
            double silence=0;
            for(int i=0;i<48000;++i) silence=std::max(silence,std::abs(c.tick(0,1)));
            check(silence<1e-6,"zero-input silence and DC initialization");
            const double low=circuitGain(device,100), mid=circuitGain(device,1000),high=circuitGain(device,5000);
            std::cout<<"GAIN "<<name<<" 100Hz="<<20*std::log10(low)<<"dB 1k="<<20*std::log10(mid)<<"dB 5k="<<20*std::log10(high)<<"dB\n";
            check(high>low*8&&high>mid,"treble emphasis above the bass band");
            check(high>10&&high<200,"plausible original-circuit treble gain");
            // Superposition must fail for a nonlinear transistor driven hard.
            rm::Circuit small,big; small.prepare(192000,device); big.prepare(192000,device);
            double a=0,b=0,asym=0;
            for(int i=0;i<96000;++i) {
                double x=std::sin(juce::MathConstants<double>::twoPi*3000*i/192000);
                auto previousFailures=big.failures();
                double y=small.tick(x*0.0001,1),z=big.tick(x*0.3,1);
                if(big.failures()!=previousFailures&&big.failures()<12) std::cout<<"SOLVER_FAIL sample="<<i<<" input="<<x*0.3<<" B="<<big.bias()[0]<<" E="<<big.bias()[1]<<" C="<<big.bias()[2]<<'\n';
                if(i>48000) {a+=y*y;b+=z*z;asym+=z*z*z;}
            }
            check(std::sqrt(b/a)<2500,"transistor large-signal compression");
            check(std::abs(asym)>1,"asymmetric saturation");
            std::cout<<"SOLVER small="<<small.failures()<<" big="<<big.failures()<<" residual="<<big.residual()<<'\n';
            check(small.failures()==0&&big.failures()==0,"large-signal solver convergence");
            rm::Circuit off; off.prepare(192000,device,0);
            double offPeak=0;
            for(int i=0;i<10000;++i) offPeak=std::max(offPeak,std::abs(off.tick(0.25*std::sin(i*0.1),0)));
            check(offPeak<1e-6,"Boost zero routes wiper to AC ground");
        }
        for(double rate:{44100.0,48000.0,96000.0,192000.0}) for(int channels:{1,2}) {
            RangeMasterProcessor p;
            p.setPlayConfigDetails(channels,channels,rate,257);
            check(p.getTotalNumInputChannels()==channels&&p.getTotalNumOutputChannels()==channels,"bus setup");
            p.prepareToPlay(rate,257);
            check(p.getLatencySamples()>0&&p.getLatencySamples()<64,"reported oversampling latency");
            juce::MidiBuffer midi;
            std::mt19937 random(1965); std::uniform_real_distribution<float> noise(-0.4f,0.4f);
            double maxPeak=0;
            auto begin=std::chrono::steady_clock::now();
            // Includes blocks larger than prepareToPlay's hint, and empty blocks.
            for(int block=0;block<180;++block) {
                int n=block%11==0?0:(block%7==0?1023:257);
                juce::AudioBuffer<float> audio(channels,n);
                for(int i=0;i<n;++i) {
                    auto x=noise(random);
                    for(int ch=0;ch<channels;++ch) audio.setSample(ch,i,ch==0?x:0);
                }
                if(block%10==0) {
                    set(p,"material",static_cast<float>((block/10)%2));
                    set(p,"boost",(block%30)/30.0f); set(p,"enabled",static_cast<float>((block/10)%2));
                }
                p.processBlock(audio,midi);
                for(int ch=0;ch<channels;++ch) for(int i=0;i<n;++i) {
                    const auto x=audio.getSample(ch,i);
                    if(!std::isfinite(x)||std::abs(x)>20) throw std::runtime_error("non-finite or unbounded output");
                    maxPeak=std::max(maxPeak,static_cast<double>(std::abs(x)));
                }
            }
            auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
            std::cout<<"STRESS rate="<<rate<<" channels="<<channels<<" wall="<<elapsed<<"s peak="<<maxPeak<<" failures="<<p.solverFailures()<<'\n';
            check(p.solverFailures()==0,"automation/noise solver convergence");
            check(true,"finite output, block sizes 0/257/1023");
        }
        {
            RangeMasterProcessor stereo;stereo.prepareToPlay(48000,512);
            juce::MidiBuffer midi;double rightPeak=0;
            for(int b=0;b<100;++b) {
                juce::AudioBuffer<float> audio(2,512);audio.clear();
                for(int i=0;i<512;++i) audio.setSample(0,i,0.3f*std::sin(float(b*512+i)*0.1f));
                stereo.processBlock(audio,midi);
                for(int i=0;i<512;++i) rightPeak=std::max(rightPeak,double(std::abs(audio.getSample(1,i))));
            }
            check(rightPeak<1e-6,"stereo isolation with signal on left input only");
        }
        RangeMasterProcessor p;
        set(p,"boost",0.93f); set(p,"master",-12.5f); set(p,"material",1); set(p,"enabled",0);
        juce::MemoryBlock state; p.getStateInformation(state);
        RangeMasterProcessor restored; restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        check(std::abs(restored.parameters.getRawParameterValue("boost")->load()-0.93f)<1e-5f&&
              std::abs(restored.parameters.getRawParameterValue("master")->load()+12.5f)<1e-5f&&
              restored.parameters.getRawParameterValue("material")->load()==1&&
              restored.parameters.getRawParameterValue("enabled")->load()==0,"all parameters survive state round trip");
        restored.setStateInformation("invalid",7);
        check(restored.parameters.getRawParameterValue("material")->load()==1,"invalid state ignored");
        // Master is independent of the circuit and must produce an exact level ratio.
        RangeMasterProcessor one,two;
        set(one,"master",-12); set(two,"master",-24);
        one.prepareToPlay(48000,512); two.prepareToPlay(48000,512);
        juce::MidiBuffer midi;
        double masterError=0;
        for(int block=0;block<100;++block) {
            juce::AudioBuffer<float> a(2,512),b(2,512);
            for(int i=0;i<512;++i) for(int ch=0;ch<2;++ch) {
                float x=0.08f*std::sin(float(block*512+i)*0.17f);a.setSample(ch,i,x);b.setSample(ch,i,x);
            }
            one.processBlock(a,midi);two.processBlock(b,midi);
            for(int i=0;i<512;++i) masterError=std::max(masterError,std::abs(double(a.getSample(0,i))-double(b.getSample(0,i))*std::pow(10.0,12.0/20.0)));
        }
        check(masterError<2e-5,"Master -12 dB scales output without changing distortion");
        // Nonfinite input defence at the plugin boundary.
        juce::AudioBuffer<float> bad(2,512);bad.clear();bad.setSample(0,0,std::numeric_limits<float>::quiet_NaN());
        bad.setSample(1,5,std::numeric_limits<float>::infinity());one.processBlock(bad,midi);
        for(int c=0;c<2;++c) for(int i=0;i<512;++i) if(!std::isfinite(bad.getSample(c,i))) throw std::runtime_error("NaN input propagated");
        check(true,"NaN / infinity input does not contaminate state");
        {
            RangeMasterProcessor controls;controls.prepareToPlay(48000,512);
            std::unique_ptr<juce::AudioProcessorEditor> editor(controls.createEditor());
            juce::Button* material=nullptr;juce::Slider* boostControl=nullptr;
            for(auto* c:editor->getChildren()) {
                if(auto* b=dynamic_cast<juce::Button*>(c))if(b->getButtonText()=="SILICON")material=b;
                if(auto* s=dynamic_cast<juce::Slider*>(c))if(s->getName()=="Boost")boostControl=s;
            }
            check(material&&boostControl,"editor material and Boost controls exist");
            material->triggerClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
            check(controls.parameters.getRawParameterValue("material")->load()==1,"editor click selects silicon in DSP");
            material->triggerClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
            check(controls.parameters.getRawParameterValue("material")->load()==0,"editor click selects germanium in DSP");
            set(controls,"material",1);juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
            check(material->getToggleState(),"host automation updates material switch");
            boostControl->setValue(0.8,juce::sendNotificationSync);
            check(std::abs(controls.parameters.getRawParameterValue("boost")->load()-0.8f)<1e-5,"editor Boost reaches DSP");
        }
        if(argc>1) {
            RangeMasterProcessor screenshot;
            screenshot.prepareToPlay(48000,512);
            std::unique_ptr<juce::AudioProcessorEditor> editor(screenshot.createEditor());
            auto img=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f);
            juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile(juce::String(argv[1]));
            auto stream=file.createOutputStream();
            if(stream) {stream->setPosition(0);stream->truncate();}
            check(stream&&juce::PNGImageFormat().writeImageToStream(img,*stream),"rendered actual JUCE editor preview");
        }
        std::cout<<"RESULT "<<checks<<" checks passed\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
