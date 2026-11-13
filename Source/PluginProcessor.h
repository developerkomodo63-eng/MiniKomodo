#pragma once
#include "Common/KomodoFx.h"

// KOMODO_FULL = 1: version completa (matriz, FX, sync, X-mod, scope). 0: version Lite para directo.
#ifndef KOMODO_FULL
 #define KOMODO_FULL 1
#endif

// Orden = indices de parametros (el editor los recorre en este orden)
enum { R0, R1, R2, W0, W1, W2, MODE, LSH, CHO,
       D1, D2, L0, L1, L2, NZ,
       CUT, RES, AMT, KEY, LFF,
       FA, FD, FS, AA, AD, AS, DEC,
       GL, LR, LA, VOL,
       ARP, ARR, ARO, ARG, DRF, VEL,
#if KOMODO_FULL
       FTY, FDR, SYN, XMD, SPR, L2R, L2S,
       M1S, M1D, M1A, M2S, M2D, M2A, M3S, M3D, M3A, M4S, M4D, M4A,
       DRV, DLT, DLM, DLF, RVM, RVS, RVD,
#endif
       NP };
inline const char* const paramIds[NP] = {
       "r0","r1","r2","w0","w1","w2","mode","lsh","cho",
       "d1","d2","l0","l1","l2","nz",
       "cut","res","amt","key","lff",
       "fa","fd","fs","aa","ad","as","dec",
       "gl","lr","la","vol",
       "arp","arr","aro","arg","drf","vel"
#if KOMODO_FULL
       ,"fty","fdr","syn","xmd","spr","l2r","l2s",
       "m1s","m1d","m1a","m2s","m2d","m2a","m3s","m3d","m3a","m4s","m4d","m4a",
       "drv","dlt","dlm","dlf","rvm","rvs","rvd"
#endif
       };

#if KOMODO_FULL
constexpr int kVoices = 16;
#else
constexpr int kVoices = 6;
#endif

struct Voice
{
    int note = -1;
    bool released = false, sustained = false;
    unsigned age = 0;
    float vel = 1.f, drift = 0.f, tgtF = 110.f, curF = 110.f;
    float ph[3] {};
    float s1 = 0.f, s2 = 0.f, s3 = 0.f, s4 = 0.f;       // filtro ladder de 4 polos
    float gAmp = 0.f, g = 0.1f;                         // valores del tick anterior (para rampa)
    Env amp, flt;
#if KOMODO_FULL
    float st[4] {};                                      // estado del filtro multimodo
    float rnd = 0.f, gl = 1.f, gr = 1.f;                 // valor aleatorio por nota (pan / matriz) y ganancias L/R previas
#endif

    bool free() const noexcept { return amp.st == 0; }
};

class MiniKomodoAudioProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    MiniKomodoAudioProcessor();
    ~MiniKomodoAudioProcessor() override { cancelPendingUpdate(); }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
   #if KOMODO_FULL
    const juce::String getName() const override { return "MiniKomodo"; }
    double getTailLengthSeconds() const override { return 3.0; }
   #else
    const juce::String getName() const override { return "MiniKomodo Lite"; }
    double getTailLengthSeconds() const override { return 0.0; }
   #endif
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return curProg; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void panic() noexcept { panicFlag.store (true); }            // corta todo: notas, arp, colas de delay/reverb

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
   #if KOMODO_FULL
    ScopeBuffer scope;
   #endif

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override;                           // Program Change MIDI -> cambia de preset en el hilo de mensajes
    void readParams();
    void handle (const juce::MidiMessage&);
    void noteOn (int n, float vel);
    void noteOff (int n);
    void releaseAll();
    void doPanic();
   #if KOMODO_FULL
    void renderVoice (Voice&, float* outL, float* outR, int n);
   #else
    void renderVoice (Voice&, float* out, int n);
   #endif

    std::atomic<float>* pp[NP] {};
    std::atomic<int> pendingProgram { -1 };
    std::atomic<bool> panicFlag { false };
    Voice voices[kVoices];
    Arp arp;
    int lastArp = 0;
    int held[32] {}, nHeld = 0, curProg = 0, mode = 0, lastMode = 0, cho = 0, lsh = 0, wf[3] {};
    unsigned ageCounter = 0, rngState = 22222u, shState = 777u;
    bool sustain = false;
    double sr = 44100.0;
    float invSr = 1.f / 44100.f, bend = 0.f, modWheel = 0.f, lfoPh = 0.f, lfo = 0.f, shVal = 0.f;

    // parametros ya convertidos (una vez por bloque)
    float mul[3] { 1.f, 1.f, 1.f }, lv[3] {}, noise = 0, vibCents = 0, lfoInc = 0, glideC = 1, gain = 0;
    float cut = 1500, res = 0, amt = 0, keyAmt = 0, lffAmt = 0, driftCents = 0, velS = 0;
    EnvP ampP, fltP;

   #if KOMODO_FULL
    StereoChorus chorus;
    StereoDelay delay;
    juce::Reverb reverb;
    int fty = 0, syn = 0, msrc[4] {}, mdst[4] {};
    float fdr = 0, xmd = 0, spr = 0, mamt[4] {}, drv = 0, dlm = 0, dlf = 0, rvm = 0, dlTarget = 0;
    float lfo2Ph = 0.f, lfo2 = 0.f, sh2Val = 0.f, lfo2Inc = 0.f;
    int l2s = 0;
    unsigned sh2State = 999u;
    bool reverbWasOn = false;
   #else
    Chorus chorus;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniKomodoAudioProcessor)
};
