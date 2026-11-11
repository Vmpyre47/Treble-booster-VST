// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <stdexcept>

void require(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
juce::AudioProcessorParameter* parameter(juce::AudioProcessor& p,const juce::String& id) {
    const juce::String name=id=="material"?"Transistor":id=="boost"?"Boost":id=="master"?"Master":"On";
    for(auto* a:p.getParameters()) if(a->getName(128)==name) return a;
    throw std::runtime_error("Missing parameter "+id.toStdString());
}
void set(juce::AudioProcessor& p,const char* id,float value) {
    auto* a=parameter(p,id);a->setValueNotifyingHost(juce::String(id)=="master"?(value+60.0f)/66.0f:value);
}
std::vector<float> render(juce::AudioProcessor& p,float material,float boost,float master,float amp) {
    p.releaseResources();set(p,"material",material);set(p,"boost",boost);set(p,"master",master);set(p,"enabled",1);
    p.prepareToPlay(48000,256);
    juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,256);std::vector<float> out;
    for(int block=0;block<375;++block) {
        for(int i=0;i<256;++i) {
            double t=(block*256+i)/48000.0;
            float x=amp*float(0.45*std::sin(2*juce::MathConstants<double>::pi*220*t)+
                0.30*std::sin(2*juce::MathConstants<double>::pi*1000*t)+0.25*std::sin(2*juce::MathConstants<double>::pi*5000*t));
            b.setSample(0,i,x);b.setSample(1,i,x);
        }
        p.processBlock(b,midi);
        if(block>=188) for(int i=0;i<256;++i)out.push_back(b.getSample(0,i));
    }
    return out;
}
double energy(const std::vector<float>& a) {double e=0;for(float v:a)e+=v*v;return e/a.size();}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        require(argc==2,"Need VST3 bundle path");
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> types;
        format.findAllTypesForFile(types,argv[1]);require(types.size()==1,"VST3 scan failed");
        juce::String error;auto plugin=format.createInstanceFromDescription(*types[0],48000,256,error);
        require(bool(plugin),"VST3 load failed");plugin->setPlayConfigDetails(2,2,48000,256);
        std::cout<<std::setprecision(9)<<"BINARY "<<types[0]->name<<" version="<<types[0]->version<<'\n';
        for(float amp:{0.005f,0.05f,0.3f}) {
            auto ge=render(*plugin,0,1,-18,amp),si=render(*plugin,1,1,-18,amp);
            double diff=0;for(size_t i=0;i<ge.size();++i)diff+=std::pow(double(ge[i])-si[i],2);
            const double relative=10*std::log10(diff/ge.size()/energy(ge));
            std::cout<<"MATERIAL amp="<<amp<<" Ge_RMS="<<std::sqrt(energy(ge))<<" Si_RMS="<<std::sqrt(energy(si))<<" difference_relative_dB="<<relative<<'\n';
            require(relative>-45,"Material switch produced effectively identical audio");
        }
        double previous=-1;
        for(float boost:{0.0f,0.25f,0.5f,0.75f,1.0f}) {
            auto signal=render(*plugin,0,boost,-18,0.005f);double level=std::sqrt(energy(signal));
            std::cout<<"BOOST "<<boost<<" output_RMS="<<level<<" dBFS="<<20*std::log10(std::max(1e-30,level))<<'\n';
            require(level>previous,"Boost is not monotonic clockwise");previous=level;
        }
        std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditor());require(bool(editor),"No editor");
        std::cout<<"PASS real VST3 Ge/Si audio difference, clockwise Boost sweep, native editor creation\n";
        editor.reset();plugin->releaseResources();return 0;
    } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
