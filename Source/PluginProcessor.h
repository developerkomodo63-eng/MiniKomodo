#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

// Orden = indices de parametros (el editor los recorre en este orden)
enum { R0, R1, R2, W0, W1, W2, D1, D2, L0, L1, L2, NZ, CUT, RES, AMT, FA, FD, FS,
       AA, AD, AS, DEC, GL, LR, LA, VOL, NP };
inline const char* const paramIds[NP] = { "r0","r1","r2","w0","w1","w2","d1","d2","l0","l1","l2","nz",
       "cut","res","amt","fa","fd","fs","aa","ad","as","dec","gl","lr","la","vol" };

class MiniKomodoAudioProcessor : public juce::AudioProcessor
{
public:
    MiniKomodoAudioProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MiniKomodo"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handle (const juce::MidiMessage&);
    void render (float* out, int n);

    std::atomic<float>* pp[NP] {};
    juce::dsp::LadderFilter<float> ladder;
    juce::ADSR ampEnv, fltEnv;
    juce::Random rng;
    std::vector<int> held;
    double sr = 44100.0;
    float ph[3] {}, lfoPh = 0.f, curF = 110.f, tgtF = 110.f;
    float tmp[16] {}, ae[16] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniKomodoAudioProcessor)
};
