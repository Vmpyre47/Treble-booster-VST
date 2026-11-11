// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc<2) return 2;
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> types;
    format.findAllTypesForFile(types,argv[1]);
    if(types.size()!=1) {std::cerr<<"FAIL VST3 scan: "<<types.size()<<'\n';return 1;}
    juce::String error;
    auto plugin=format.createInstanceFromDescription(*types[0],48000,512,error);
    if(!plugin) {std::cerr<<error<<'\n';return 1;}
    plugin->setPlayConfigDetails(2,2,48000,512);
    plugin->prepareToPlay(48000,512);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> audio(2,512);
    float max=0;
    for(int n=0;n<100;++n) {
        for(int i=0;i<512;++i) for(int ch=0;ch<2;++ch) audio.setSample(ch,i,0.1f*std::sin(float(n*512+i)*0.2f));
        plugin->processBlock(audio,midi);
        for(int ch=0;ch<2;++ch) for(int i=0;i<512;++i) {
            float x=audio.getSample(ch,i);if(!std::isfinite(x)) return 1;max=juce::jmax(max,std::abs(x));
        }
    }
    juce::MemoryBlock state;plugin->getStateInformation(state);plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditor());
    if(!editor||max<0.001f||state.getSize()==0) return 1;
    std::cout<<"PASS VST3 scan, load, stereo processing, state restore, native editor\n"
             <<"PLUGIN "<<types[0]->name<<" latency="<<plugin->getLatencySamples()<<" peak="<<max<<"\n";
    editor.reset();plugin->releaseResources();return 0;
}
