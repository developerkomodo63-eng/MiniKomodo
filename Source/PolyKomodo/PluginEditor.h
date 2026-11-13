#pragma once
#include "PluginProcessor.h"
#include "Common/KomodoUi.h"

// ------------------------------------------------------------------ aspecto "Juno": metal azul acero, faders con relieve, secciones de colores
class JunoLook : public juce::LookAndFeel_V4
{
public:
    JunoLook()
    {
        const juce::Colour txt (0xffcfe9f7);
        setColour (juce::ComboBox::textColourId, txt);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff131b22));
        setColour (juce::PopupMenu::textColourId, txt);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff39c0ed));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff081016));
        setColour (juce::Label::textColourId, juce::Colour (0xffa9bccb));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff39c0ed));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff39c0ed));
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0131b22));
        setColour (juce::BubbleComponent::outlineColourId, juce::Colour (0xff39c0ed));
        setColour (juce::TooltipWindow::textColourId, txt);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.f)); }
    juce::Font getPopupMenuFont() override               { return juce::Font (juce::FontOptions (13.f)); }
    int getSliderThumbRadius (juce::Slider&) override    { return 8; }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::thumbColourId);
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y + 8.f, bot = (float) (y + h) - 8.f;

        // marcas
        g.setColour (juce::Colours::white.withAlpha (0.24f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = top + (bot - top) * (float) i / 10.f, len = (i % 5 == 0) ? 6.f : 3.5f;
            g.fillRect (cx - 15.f, ty - 0.5f, len, 1.f);
            g.fillRect (cx + 15.f - len, ty - 0.5f, len, 1.f);
        }
        // ranura hundida
        auto slot = juce::Rectangle<float> (cx - 3.f, top - 3.f, 6.f, bot - top + 6.f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (slot.expanded (1.f).translated (0.f, 1.f), 3.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff020406), slot.getX(), 0.f, juce::Colour (0xff1c2630), slot.getRight(), 0.f, false));
        g.fillRoundedRectangle (slot, 3.f);
        // relleno de valor
        g.setColour (accent.withAlpha (0.8f));
        g.fillRoundedRectangle (cx - 1.2f, pos, 2.4f, juce::jmax (0.f, bot + 3.f - pos), 1.2f);

        // capuchon
        juce::Rectangle<float> cap (cx - 13.f, pos - 7.5f, 26.f, 15.f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (cap.translated (1.5f, 3.f), 3.f);
        juce::ColourGradient grad (juce::Colour (0xfff2f5f8), 0.f, cap.getY(), juce::Colour (0xff4d5861), 0.f, cap.getBottom(), false);
        grad.addColour (0.45, juce::Colour (0xffa9b3bb));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (cap, 3.f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (cap, 3.f, 1.f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawLine (cap.getX() + 3.f, cap.getY() + 1.5f, cap.getRight() - 3.f, cap.getY() + 1.5f, 1.f);
        g.setColour (accent);
        g.fillRect (cap.getX() + 4.f, pos - 1.f, cap.getWidth() - 8.f, 2.f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::thumbColourId);
        const float size = (float) juce::jmin (w, h);
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float R = size * 0.5f - 2.f, ang = a0 + pos * (a1 - a0);

        juce::Path track, val;
        track.addCentredArc (cx, cy, R, R, 0.f, a0, a1, true);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.strokePath (track, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        val.addCentredArc (cx, cy, R, R, 0.f, a0, ang, true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = R - 5.f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (cx - kr + 1.5f, cy - kr + 4.f, kr * 2.f, kr * 2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeef2f5), cx - kr * 0.5f, cy - kr * 0.7f,
                                                  juce::Colour (0xff3b444c), cx + kr, cy + kr, true));
        g.fillEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f, 1.f);
        const float fr = kr * 0.7f;                                           // tapa interior
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6f7a84), cx - fr * 0.5f, cy - fr * 0.7f,
                                                  juce::Colour (0xffd9dfe4), cx + fr * 0.7f, cy + fr, true));
        g.fillEllipse (cx - fr, cy - fr, fr * 2.f, fr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (cx - fr, cy - fr, fr * 2.f, fr * 2.f, 0.8f);
        juce::Path ptr;
        ptr.addRoundedRectangle (-1.6f, -kr + 1.f, 3.2f, kr * 0.6f, 1.4f);
        g.setColour (juce::Colour (0xff10171d));
        g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool down, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff0e1318 : 0xff303c48), 0.f, 0.f, juce::Colour (0xff0d1318), 0.f, (float) h, false));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawRoundedRectangle (r, 5.f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawLine (r.getX() + 5.f, r.getY() + 1.5f, r.getRight() - 5.f, r.getY() + 1.5f);
        juce::Path arrow;
        const float ax = (float) w - 12.f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff0e1318 : over ? 0xff3d4b59 : 0xff303c48), 0.f, r.getY(),
                                                  juce::Colour (0xff0d1318), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawRoundedRectangle (r, 5.f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawLine (r.getX() + 5.f, r.getY() + 1.5f, r.getRight() - 5.f, r.getY() + 1.5f);
    }
};

// ------------------------------------------------------------------ editor
class PolyUi : public juce::Component
{
   #if KOMODO_FULL
    static constexpr const char* kName = "POLYKOMODO";
    static constexpr const char* kSub  = "8-VOICE POLYPHONIC SYNTHESIZER  -  FULL";
   #else
    static constexpr const char* kName = "POLYKOMODO LITE";
    static constexpr const char* kSub  = "6-VOICE  -  LIVE EDITION  -  LOW CPU";
   #endif
    struct Cell { int p0; int p1; float w; bool fader; };      // p0 = -1: selector de presets; p1 >= 0: combo apilado
    struct Sec  { const char* title; juce::uint32 colour; std::vector<Cell> cells; };
    struct SecRect { juce::Rectangle<int> r; const char* title; juce::Colour col; };

public:
    float startScale = 1.f;

    explicit PolyUi (PolyKomodoAudioProcessor& p)
        : proc (p),
          kb (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
          presets (p)
   #if KOMODO_FULL
          , scope (p.scope, juce::Colour (0xff39c0ed))
   #endif
    {
        setLookAndFeel (&look);
        const juce::uint32 cLfo = 0xff39c0ed, cDco = 0xff4d8af0, cVcf = 0xffa66cff, cFe = 0xfff06ab0, cAe = 0xffffc93c,
                           cPat = 0xff9fb2c2, cCho = 0xff2ee6c5, cVoi = 0xffff8f5a, cArp = 0xff8ddb5a, cMas = 0xffe9eef2;
        rows = {
            { { "LFO",     cLfo, { { LR, -1, 36, true }, { LP, -1, 36, true } } },
              { "DCO",     cDco, { { RNG, -1, 64, false }, { SAW, -1, 36, true }, { PUL, -1, 36, true }, { PW, -1, 36, true },
                                   { PWM, -1, 36, true }, { SUB, -1, 36, true }, { NOI, -1, 36, true } } },
              { "VCF",     cVcf, { { HPF, -1, 64, false }, { CUT, -1, 36, true }, { RES, -1, 36, true }, { ENV, -1, 36, true },
                                   { KEY, -1, 36, true }, { LFF, -1, 36, true } } },
              { "VCF ENV", cFe,  { { FA, -1, 36, true }, { FD, -1, 36, true }, { FS, -1, 36, true }, { FR, -1, 36, true } } },
              { "VCA ENV", cAe,  { { AA, -1, 36, true }, { AD, -1, 36, true }, { AS, -1, 36, true }, { AR, -1, 36, true } } } },
            { { "PATCH",   cPat, { { -1, -1, 230, false } } },
              { "CHORUS",  cCho, { { CHO, -1, 80, false } } },
              { "VOICE",   cVoi, { { MODE, -1, 80, false }, { GL, -1, 62, false }, { VEL, -1, 62, false }, { DRF, -1, 62, false } } },
              { "ARPEGGIATOR", cArp, { { ARP, ARR, 92, false }, { ARO, -1, 78, false }, { ARG, -1, 62, false } } },
              { "MASTER",  cMas, { { VOL, -1, 70, false } } } }
           #if KOMODO_FULL
            ,
            { { "OSC 2",   cDco, { { O2L, -1, 62, false }, { O2D, -1, 62, false }, { O2R, -1, 80, false } } },
              { "UNISON",  cCho, { { UNI, -1, 76, false }, { UND, -1, 62, false }, { SPR, -1, 62, false } } },
              { "FILTER TYPE", cVcf, { { FTY, -1, 84, false }, { FDR, -1, 62, false } } },
              { "LFO 2",   cLfo, { { L2S, -1, 90, false }, { L2R, -1, 62, false } } },
              { "OUTPUT",  cMas, { { DRV, -1, 62, false } } } },
            { { "MODULATION MATRIX", cFe, { { M1S, M1D, 92, false }, { M1A, -1, 54, false }, { M2S, M2D, 92, false }, { M2A, -1, 54, false },
                                            { M3S, M3D, 92, false }, { M3A, -1, 54, false }, { M4S, M4D, 92, false }, { M4A, -1, 54, false } } } },
            { { "DELAY",   cVoi, { { DLT, -1, 84, false }, { DLM, -1, 62, false }, { DLF, -1, 62, false } } },
              { "REVERB",  cArp, { { RVM, -1, 62, false }, { RVS, -1, 62, false }, { RVD, -1, 62, false } } },
              { "SCOPE",   cPat, { { -2, -1, 230, false } } } }
           #endif
        };
        for (auto& row : rows) for (auto& s : row) for (auto& c : s.cells) { build (c.p0, s.colour); build (c.p1, s.colour); }

        addAndMakeVisible (presets);
       #if KOMODO_FULL
        addAndMakeVisible (scope);
       #endif
        panicBtn.onClick = [this] { proc.panic(); };
        addAndMakeVisible (panicBtn);
        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xff39c0ed));
        kb.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x3339c0ed));
        kb.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffeef2f5));
        kb.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0d1319));
        kb.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6a7580));
        kb.setScrollButtonsVisible (false);
        kb.setAvailableRange (36, 96);
        addAndMakeVisible (kb);
       #if KOMODO_FULL
        startScale = 0.85f;
        setSize (960, 880);
       #else
        setSize (960, 520);
       #endif
    }

    ~PolyUi() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth(), h = getHeight();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b3641), 0.f, 0.f, juce::Colour (0xff0b1117), 0.f, (float) h, false));
        g.fillAll();

        // metal cepillado
        juce::Random rnd (99);
        for (int i = 0; i < 240; ++i)
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.012f + rnd.nextFloat() * 0.025f));
            g.drawHorizontalLine ((int) (rnd.nextFloat() * (float) h), 0.f, (float) w);
        }
        // marco biselado
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRect (0, 0, w, h, 1);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRect (1, 1, w - 2, h - 2, 1);

        // titulo
        juce::Font logo (juce::FontOptions (30.f, juce::Font::bold));
        logo.setExtraKerningFactor (0.16f);
        g.setFont (logo);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawText (kName, 26, 13, 400, 34, juce::Justification::centredLeft);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffffff), 0.f, 8.f, juce::Colour (0xff8fb7cf), 0.f, 42.f, false));
        g.drawText (kName, 24, 10, 400, 34, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (juce::Colour (0xff8fa6b8));
        g.drawText (kSub, 508, 22, 426, 14, juce::Justification::centredRight);

        // franja multicolor
        juce::ColourGradient band (juce::Colour (0xff39c0ed), 24.f, 0.f, juce::Colour (0xffef476f), (float) w - 24.f, 0.f, false);
        band.addColour (0.33, juce::Colour (0xff8ddb5a));
        band.addColour (0.66, juce::Colour (0xffffc93c));
        g.setGradientFill (band);
        g.fillRoundedRectangle (24.f, 52.f, (float) w - 48.f, 4.f, 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawHorizontalLine (57, 24.f, (float) w - 24.f);

        // secciones
        juce::Font title (juce::FontOptions (10.f, juce::Font::bold));
        title.setExtraKerningFactor (0.12f);
        for (auto& s : secRects)
        {
            auto r = s.r.toFloat();
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillRoundedRectangle (r.translated (0.f, 2.5f), 7.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d1318), 0.f, r.getY(), juce::Colour (0xff1b2530), 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 7.f);
            g.setColour (juce::Colours::white.withAlpha (0.1f));
            g.drawRoundedRectangle (r.translated (0.f, 0.6f), 7.f, 1.f);
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.drawRoundedRectangle (r, 7.f, 1.2f);

            juce::Path strip;
            strip.addRoundedRectangle (r.getX() + 1.f, r.getY() + 1.f, r.getWidth() - 2.f, 17.f, 6.f, 6.f, true, true, false, false);
            g.setGradientFill (juce::ColourGradient (s.col.brighter (0.3f), 0.f, r.getY(), s.col.darker (0.4f), 0.f, r.getY() + 18.f, false));
            g.fillPath (strip);
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawHorizontalLine ((int) r.getY() + 2, r.getX() + 6.f, r.getRight() - 6.f);
            g.setFont (title);
            g.setColour (juce::Colour (0xff081016));
            g.drawText (s.title, s.r.getX(), s.r.getY() + 2, s.r.getWidth(), 15, juce::Justification::centred);
        }
    }

    void resized() override
    {
        secRects.clear();
        auto content = getLocalBounds().reduced (24, 0);
        content.removeFromTop (66);
        panicBtn.setBounds (404, 14, 76, 26);

        const int heights[5] { 186, 116, 116, 116, 116 };
        int rowIdx = 0;
        for (auto& row : rows)
        {
            auto rowR = content.removeFromTop (heights[rowIdx++]);
            content.removeFromTop (10);
            const int gap = 8, n = (int) row.size();
            float sum = 0.f;
            for (auto& s : row) for (auto& c : s.cells) sum += c.w;
            const float scale = ((float) rowR.getWidth() - (float) (gap * (n - 1)) - 10.f * (float) n) / sum;

            int x = rowR.getX();
            for (auto& s : row)
            {
                float sw = 0.f;
                for (auto& c : s.cells) sw += c.w * scale;
                juce::Rectangle<int> sr (x, rowR.getY(), (int) std::round (sw) + 10, rowR.getHeight());
                secRects.push_back ({ sr, s.title, juce::Colour (s.colour) });
                auto cells = sr.reduced (5, 0).withTrimmedTop (22).withTrimmedBottom (4);
                float cx = (float) cells.getX();
                for (auto& c : s.cells)
                {
                    placeCell (c, { (int) std::round (cx), cells.getY(), (int) std::round (c.w * scale), cells.getHeight() });
                    cx += c.w * scale;
                }
                x = sr.getRight() + gap;
            }
        }
        kb.setBounds (content.withTrimmedBottom (14));
        kb.setKeyWidth ((float) kb.getWidth() / 36.f);
    }

private:
    static const char* shortName (int i)
    {
        static const char* n[NP] = { "RANGE", "SAW", "PULSE", "PW", "PWM", "SUB", "NOISE", "RATE", "PITCH",
                                     "CUTOFF", "RES", "ENV", "KEY", "LFO", "GLIDE", "MASTER",
                                     "A", "D", "S", "R", "A", "D", "S", "R",
                                     "HPF", "MODE", "CHORUS", "ARP", "RATE", "OCTAVES", "GATE", "DRIFT", "VELOCITY"
                                    #if KOMODO_FULL
                                     , "TYPE", "DRIVE", "LEVEL", "FINE", "SEMI", "VOICES", "DETUNE", "SPREAD", "RATE", "SHAPE",
                                     "SRC", "DEST", "AMT", "SRC", "DEST", "AMT", "SRC", "DEST", "AMT", "SRC", "DEST", "AMT",
                                     "DRIVE", "TIME", "MIX", "FDBK", "MIX", "SIZE", "DAMP"
                                    #endif
                                   };
        return (i >= 0 && i < NP && n[i] != nullptr) ? n[i] : "";
    }

    void addLabel (int i, const juce::String& text)
    {
        auto l = std::make_unique<juce::Label> (juce::String(), text);
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::FontOptions (10.f, juce::Font::bold));
        l->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*l);
        labels[i] = std::move (l);
    }

    void build (int i, juce::uint32 colour)
    {
        if (i < 0 || built[i]) return;
        built[i] = true;
        auto* par = proc.apvts.getParameter (paramIds[i]);
        const juce::Colour col (colour);

        if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (par))
        {
            auto cb = std::make_unique<juce::ComboBox>();
            cb->addItemList (ch->choices, 1);
            cb->setColour (juce::ComboBox::arrowColourId, col);
            addAndMakeVisible (*cb);
            comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramIds[i], *cb));
            combos[i] = std::move (cb);
            addLabel (i, shortName (i));
        }
        else
        {
            const bool fader = isFader (i);
            auto s = std::make_unique<juce::Slider> (fader ? juce::Slider::LinearVertical : juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox);
            if (! fader) s->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
            s->setColour (juce::Slider::thumbColourId, col);
            s->setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (*s);
            knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramIds[i], *s));
            knobs[i] = std::move (s);
            const juce::String sn (shortName (i));
            addLabel (i, sn.isNotEmpty() ? sn : juce::String (par->getName (24)).toUpperCase());
        }
    }

    bool isFader (int i) const
    {
        for (auto& row : rows) for (auto& s : row) for (auto& c : s.cells) if (c.p0 == i) return c.fader;
        return false;
    }

    void placeCell (const Cell& c, juce::Rectangle<int> r)
    {
        auto comboWithLabel = [&] (int i, juce::Rectangle<int>& area)
        {
            labels[i]->setBounds (area.removeFromTop (13));
            combos[i]->setBounds (area.removeFromTop (22).reduced (3, 0));
        };

       #if KOMODO_FULL
        if (c.p0 == -2) { scope.setBounds (r.reduced (2, 0)); return; }
       #endif
        if (c.p0 < 0)
            presets.setBounds (r.withSizeKeepingCentre (r.getWidth() - 4, 28));
        else if (c.p1 >= 0)
        {
            auto a = r.withSizeKeepingCentre (r.getWidth(), 71);
            comboWithLabel (c.p0, a);
            a.removeFromTop (4);
            comboWithLabel (c.p1, a);
        }
        else if (combos[c.p0])
        {
            auto a = r.withSizeKeepingCentre (r.getWidth(), 35);
            comboWithLabel (c.p0, a);
        }
        else if (c.fader)
        {
            labels[c.p0]->setBounds (r.removeFromBottom (14).expanded (6, 0));
            knobs[c.p0]->setBounds (r.reduced (0, 2));
        }
        else
        {
            labels[c.p0]->setBounds (r.removeFromBottom (14).expanded (6, 0));
            knobs[c.p0]->setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 66), juce::jmin (r.getHeight(), 66)));
        }
    }

    PolyKomodoAudioProcessor& proc;
    JunoLook look;
    juce::MidiKeyboardComponent kb;
    PresetBar presets;
    juce::TextButton panicBtn { "PANIC" };
   #if KOMODO_FULL
    ScopeView scope;
   #endif
    std::vector<std::vector<Sec>> rows;
    std::vector<SecRect> secRects;
    bool built[NP] {};
    std::unique_ptr<juce::Label> labels[NP];
    std::unique_ptr<juce::ComboBox> combos[NP];
    std::unique_ptr<juce::Slider> knobs[NP];
    // los attachments se declaran al final: se destruyen primero
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyUi)
};
