#pragma once
#include "Common/KomodoFx.h"

// KOMODO_FULL = 1: version completa (unison, OSC2, matriz, FX, scope). 0: version Lite para directo.
#ifndef KOMODO_FULL
 #define KOMODO_FULL 1
#endif

// Orden = indices de parametros (el editor los recorre en este orden)
enum { RNG, SAW, PUL, PW, PWM, SUB, NOI, LR, LP,
       CUT, RES, ENV, KEY, LFF, GL, VOL,
       FA, FD, FS, FR, AA, AD, AS, AR,
       HPF, MODE, CHO,
       ARP, ARR, ARO, ARG, DRF, VEL,
#if KOMODO_FULL
       FTY, FDR, O2L, O2D, O2R, UNI, UND, SPR, L2R, L2S,
       M1S, M1D, M1A, M2S, M2D, M2A, M3S, M3D, M3A, M4S, M4D, M4A,
       DRV, DLT, DLM, DLF, RVM, RVS, RVD,
#endif
       NP };
inline const char* const paramIds[NP] = {
       "rng","saw","pul","pw","pwm","sub","noi","lr","lp",
       "cut","res","env","key","lff","gl","vol",
       "fa","fd","fs","fr","aa","ad","as","ar",
       "hpf","mode","cho",
       "arp","arr","aro","arg","drf","vel"
#if KOMODO_FULL
       ,"fty","fdr","o2l","o2d","o2r","uni","und","spr","l2r","l2s",
       "m1s","m1d","m1a","m2s","m2d","m2a","m3s","m3d","m3a","m4s","m4d","m4a",
       "drv","dlt","dlm","dlf","rvm","rvs","rvd"
#endif
       };

#if KOMODO_FULL
constexpr int kVoices = 8;
constexpr int kMaxLay = 7;     // capas de unison
#else
constexpr int kVoices = 6;     // como el Juno-60
#endif

// ---------------------------------------------------------------- voz
struct Voice
{
    int note = -1;
    bool released = false, sustained = false;
    unsigned age = 0;
    float vel = 1.f, drift = 0.f, tgtF = 110.f, curF = 110.f;
   #if KOMODO_FULL
    float ph[kMaxLay] {}, ph2[kMaxLay] {}, sph = 0.f;   // fases por capa de unison, osc2 y sub
    float hpL = 0.f, hpR = 0.f, st[4] {}, stR[4] {};    // filtro multimodo (L y R) y HPF
    float rnd = 0.f, gl = 1.f, gr = 1.f;                // valor aleatorio por nota y ganancias L/R previas
   #else
    float ph = 0.f, sph = 0.f;                          // fase oscilador y sub
    float hpL = 0.f, s1 = 0.f, s2 = 0.f, s3 = 0.f, s4 = 0.f;
   #endif
    float gAmp = 0.f, g = 0.1f;                         // valores del tick anterior (para rampa)
    Env amp, flt;

    bool free() const noexcept { return amp.st == 0; }
};

// ---------------------------------------------------------------- procesador
class PolyKomodoAudioProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    PolyKomodoAudioProcessor();
    ~PolyKomodoAudioProcessor() override { cancelPendingUpdate(); }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
   #if KOMODO_FULL
    const juce::String getName() const override { return "PolyKomodo"; }
    double getTailLengthSeconds() const override { return 3.0; }
   #else
    const juce::String getName() const override { return "PolyKomodo Lite"; }
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
    void doPanic();
    void readParams();
    void handle (const juce::MidiMessage&);
    void noteOn (int n, float vel);
    void noteOff (int n);
    void releaseAll();
    Voice* allocate (int n);
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
    int held[32] {}, nHeld = 0, curProg = 0, mode = 0, lastMode = 0, cho = 0;
    unsigned ageCounter = 0, rngState = 22222u;
    bool sustain = false;
    double sr = 44100.0;
    float invSr = 1.f / 44100.f, bend = 0.f, modWheel = 0.f, lfoPh = 0.f, lfo = 0.f;

    // parametros ya convertidos (se calculan una vez por bloque)
    float lsaw = 0, lpul = 0, lsub = 0, noi = 0, pwBase = 0.5f, pwmAmt = 0, vibCents = 0, lfoInc = 0;
    float cut = 2000, res = 0, envAmt = 0, keyAmt = 0, lffAmt = 0, glideC = 1, rangeMul = 1, hpa = 0, gain = 0;
    float driftCents = 0, velS = 0;
    bool hpOn = false;
    EnvP ampP, fltP;

   #if KOMODO_FULL
    StereoChorus chorus;
    StereoDelay delay;
    juce::Reverb reverb;
    int fty = 0, msrc[4] {}, mdst[4] {}, nLay = 1, l2s = 0;
    bool dual = false, reverbWasOn = false;
    float fdr = 0, lo2 = 0, o2Mul = 1, und = 0, spr = 0, mamt[4] {}, drv = 0, dlm = 0, dlf = 0, rvm = 0, dlTarget = 0;
    float lyOff[kMaxLay] {}, lyGL[kMaxLay] {}, lyGR[kMaxLay] {};
    float lfo2Ph = 0.f, lfo2 = 0.f, sh2Val = 0.f, lfo2Inc = 0.f;
    unsigned sh2State = 999u;
   #else
    Chorus chorus;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyKomodoAudioProcessor)
};
