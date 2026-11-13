#pragma once
#include "KeysKomodo/KeysDsp.h"

// Orden = indices de parametros (el editor los recorre en este orden)
enum { ENG, RNG, ATK, DEC, SUS, REL, VEL, VIB, VBR,
       FMR, FMI, FMV, FMD, TNE, BRK,                        // motor 0: piano electrico (FM)
       DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7, DB8, PRC, CLK,   // motor 1: organo (drawbars)
       WAV, ENS, CUT, RES, FEA, FDC, KEY,                   // motor 2: teclas analogicas (strings, brass, clav...)
       VOW, BRT,                                            // motor 3: coro
       DRV, TONE, MOD, MDR, MDD, DLT, DLM, DLF, RVM, RVS, RVD, VOL, NP };
inline const char* const paramIds[NP] = {
       "eng","rng","atk","dec","sus","rel","vel","vib","vbr",
       "fmr","fmi","fmv","fmd","tne","brk",
       "db0","db1","db2","db3","db4","db5","db6","db7","db8","prc","clk",
       "wav","ens","cut","res","fea","fdc","key",
       "vow","brt",
       "drv","tone","mod","mdr","mdd","dlt","dlm","dlf","rvm","rvs","rvd","vol" };

constexpr int kVoices = 8;

struct Voice
{
    int note = -1;
    bool released = false, sustained = false;
    unsigned age = 0;
    float vel = 1.f, hz = 440.f;
    float p[3] {};                                       // fases (portadora / modulador / tine, o 3 sierras)
    float st[4] {}, sv[6] {};                            // filtro ladder / formantes
    float gAmp = 0.f, g = -1.f;                          // valores del tick anterior (para rampa); g < 0 = voz nueva
    float envI = 1.f, tineE = 1.f, percE = 1.f, click = 1.f;
    Env amp, flt;

    bool free() const noexcept { return amp.st == 0; }
};

class KeysKomodoAudioProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    KeysKomodoAudioProcessor();
    ~KeysKomodoAudioProcessor() override { cancelPendingUpdate(); }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "KeysKomodo"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return curProg; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void panic() noexcept { panicFlag.store (true); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    ScopeBuffer scope;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override;
    void readParams();
    void handle (const juce::MidiMessage&);
    void noteOn (int n, float vel);
    void noteOff (int n);
    void releaseAll();
    void doPanic();
    void renderVoice (Voice&, float* out, int n);

    std::atomic<float>* pp[NP] {};
    std::atomic<int> pendingProgram { -1 };
    std::atomic<bool> panicFlag { false };
    Voice voices[kVoices];
    int curProg = 0, eng = 0, lastMod = -1, mod = 0;
    unsigned ageCounter = 0, rngState = 22222u;
    bool sustain = false, reverbWasOn = false;
    double sr = 44100.0;
    float invSr = 1.f / 44100.f, bend = 0.f, modWheel = 0.f, lfoPh = 0.f, lfo = 0.f;

    // parametros ya convertidos (una vez por bloque)
    float rangeMul = 1.f, velS = 0.f, vibCents = 0.f, lfoInc = 0.f, gain = 0.f;
    float fmRatio = 1.f, fmI = 0.f, fmV = 0.f, fmIdxCoef = 0.f, fmTine = 0.f, fmBark = 0.f, tineCoef = 0.f;
    float dbg[9] {}, percCoef = 0.f, clickAmt = 0.f, clickCoef = 0.f;
    int prc = 0, wav = 0;
    float ensCents = 0.f, cut = 1500.f, res = 0.f, fea = 0.f, keyAmt = 0.f, brt = 0.f;
    float fF[3] {}, fQ[3] { 0.12f, 0.09f, 0.07f }, fG[3] { 1.f, 0.55f, 0.3f };
    float drv = 0.f, tone = 0.f, mdr = 0.f, mdd = 0.f, dlm = 0.f, dlf = 0.f, rvm = 0.f, dlTarget = 0.f, toneLp = 0.f;
    EnvP ampP, fltP;

    StereoChorus chorus;
    Ensemble ensemble;
    Phaser phaser;
    Tremolo trem;
    Rotary rotary;
    StereoDelay delay;
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeysKomodoAudioProcessor)
};
