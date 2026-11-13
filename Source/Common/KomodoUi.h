#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "KomodoFx.h"

// Selector de presets con flechas < >. Se mantiene sincronizado si el preset cambia por MIDI (Program Change) o desde el host.
class PresetBar : public juce::Component, private juce::Timer
{
public:
    explicit PresetBar (juce::AudioProcessor& p) : proc (p)
    {
        for (int i = 0; i < p.getNumPrograms(); ++i) box.addItem (p.getProgramName (i), i + 1);
        box.setSelectedItemIndex (p.getCurrentProgram(), juce::dontSendNotification);
        box.onChange = [this] { proc.setCurrentProgram (box.getSelectedItemIndex()); };
        prev.onClick = [this] { step (-1); };
        next.onClick = [this] { step (1); };
        addAndMakeVisible (box); addAndMakeVisible (prev); addAndMakeVisible (next);
        startTimer (250);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        prev.setBounds (r.removeFromLeft (28));
        next.setBounds (r.removeFromRight (28));
        box.setBounds (r.reduced (4, 0));
    }

    juce::ComboBox box;
    juce::TextButton prev { "<" }, next { ">" };

private:
    void timerCallback() override
    {
        const int cur = proc.getCurrentProgram();
        if (cur != box.getSelectedItemIndex()) box.setSelectedItemIndex (cur, juce::dontSendNotification);
    }
    void step (int d)
    {
        const int n = proc.getNumPrograms();
        box.setSelectedItemIndex ((box.getSelectedItemIndex() + d + n) % n, juce::sendNotification);
    }
    juce::AudioProcessor& proc;
};

// Osciloscopio (solo version completa). Dibuja a 30 fps mientras el editor esta abierto.
class ScopeView : public juce::Component, private juce::Timer
{
public:
    ScopeView (const ScopeBuffer& s, juce::Colour c) : scope (s), colour (c) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff040608), 0.f, r.getY(), juce::Colour (0xff0b1014), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (colour.withAlpha (0.10f));
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 4.f), r.getX() + 3.f, r.getRight() - 3.f);
        for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 8.f), r.getY() + 3.f, r.getBottom() - 3.f);

        // disparo: cruce por cero ascendente, para que la onda quede quieta
        int start = 0;
        for (int i = 0; i < kSpan - 1; ++i) if (data[i] <= 0.f && data[i + 1] > 0.f) { start = i; break; }
        juce::Path p;
        const float cy = r.getCentreY(), amp = r.getHeight() * 0.42f;
        for (int i = 0; i < kShow; ++i)
        {
            const float x = r.getX() + 3.f + (r.getWidth() - 6.f) * (float) i / (float) (kShow - 1);
            const float y = cy - juce::jlimit (-1.f, 1.f, data[start + i] * 2.2f) * amp;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (colour.withAlpha (0.25f));
        g.strokePath (p, juce::PathStrokeType (3.5f));
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.3f));
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 5.f, 1.2f);
    }

private:
    static constexpr int kShow = 400, kSpan = 1200;
    void timerCallback() override { scope.snapshot (data, kSpan + kShow); repaint(); }
    const ScopeBuffer& scope;
    juce::Colour colour;
    float data[kSpan + kShow] {};
};

// Editor redimensionable: el diseno se dibuja a tamano fijo y se escala (util en notebooks chicas o pantallas 4K).
template <class Ui>
class ScaledEditor : public juce::AudioProcessorEditor
{
public:
    template <class Proc>
    explicit ScaledEditor (Proc& p) : juce::AudioProcessorEditor (&p), ui (p)
    {
        addAndMakeVisible (ui);
        designW = ui.getWidth();  designH = ui.getHeight();
        setResizable (true, true);
        getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);
        setResizeLimits (designW / 2, designH / 2, designW * 2, designH * 2);
        setSize ((int) ((float) designW * ui.startScale), (int) ((float) designH * ui.startScale));
    }

    void resized() override
    {
        ui.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) designW));
        ui.setBounds (0, 0, designW, designH);
    }

private:
    Ui ui;
    int designW = 100, designH = 100;
};
