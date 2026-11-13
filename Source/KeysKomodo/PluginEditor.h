#pragma once
#include "PluginProcessor.h"
#include "Common/KomodoUi.h"

// ------------------------------------------------------------------ aspecto "workstation" claro (chasis blanco/plata, perillas negras, pantalla azul)
class KeysLook : public juce::LookAndFeel_V4
{
public:
    KeysLook()
    {
        const juce::Colour ink (0xff2c3640);
        setColour (juce::ComboBox::textColourId, ink);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfff3f5f7));
        setColour (juce::PopupMenu::textColourId, ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2f7de1));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::Label::textColourId, juce::Colour (0xff4a5663));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff3a4652));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff0b1620));
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0f3f5f7));
        setColour (juce::BubbleComponent::outlineColourId, juce::Colour (0xff2f7de1));
        setColour (juce::TooltipWindow::textColourId, ink);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.f)); }
    juce::Font getPopupMenuFont() override               { return juce::Font (juce::FontOptions (13.f)); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (12.f, juce::Font::bold)); }
    int getSliderThumbRadius (juce::Slider&) override    { return 8; }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::thumbColourId);
        const float cx = (float) x + (float) w * 0.5f, top = (float) y + 8.f, bot = (float) (y + h) - 8.f;
        g.setColour (juce::Colour (0xff2c3640).withAlpha (0.35f));
        for (int i = 0; i <= 8; ++i)
        {
            const float ty = top + (bot - top) * (float) i / 8.f;
            g.fillRect (cx - 13.f, ty - 0.5f, 4.f, 1.f);
            g.fillRect (cx + 9.f, ty - 0.5f, 4.f, 1.f);
        }
        auto slot = juce::Rectangle<float> (cx - 2.5f, top - 3.f, 5.f, bot - top + 6.f);
        g.setColour (juce::Colour (0xff10161c));
        g.fillRoundedRectangle (slot, 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (slot.expanded (0.5f), 3.f, 0.8f);
        juce::Rectangle<float> cap (cx - 8.f, pos - 9.f, 16.f, 18.f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (cap.translated (1.5f, 2.5f), 3.f);
        g.setGradientFill (juce::ColourGradient (accent.brighter (0.35f), cap.getX(), 0.f, accent.darker (0.5f), cap.getRight(), 0.f, false));
        g.fillRoundedRectangle (cap, 3.f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (cap, 3.f, 1.f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (cap.getX() + 3.f, cap.getY() + 1.5f, cap.getRight() - 3.f, cap.getY() + 1.5f, 1.f);
    }

    // perilla negra de goma con tapa plateada y marca blanca, como las del Triton
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::thumbColourId);
        const float size = (float) juce::jmin (w, h);
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float R = size * 0.5f - 2.f, ang = a0 + pos * (a1 - a0);

        juce::Path track, val;
        track.addCentredArc (cx, cy, R, R, 0.f, a0, a1, true);
        g.setColour (juce::Colour (0xff2c3640).withAlpha (0.22f));
        g.strokePath (track, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        val.addCentredArc (cx, cy, R, R, 0.f, a0, ang, true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = R - 5.f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (cx - kr + 1.2f, cy - kr + 3.f, kr * 2.f, kr * 2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff59626b), cx - kr * 0.5f, cy - kr * 0.8f,
                                                  juce::Colour (0xff0d1115), cx + kr * 0.7f, cy + kr, true));
        g.fillEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
        // tapa plateada
        const float cr = kr * 0.64f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff1f4f7), cx - cr * 0.4f, cy - cr, juce::Colour (0xff8d98a3), cx + cr * 0.5f, cy + cr, true));
        g.fillEllipse (cx - cr, cy - cr, cr * 2.f, cr * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (cx - cr, cy - cr, cr * 2.f, cr * 2.f, 0.8f);
        // marca
        juce::Path ptr;
        ptr.addRoundedRectangle (-1.5f, -kr + 1.5f, 3.f, kr * 0.52f, 1.3f);
        g.setColour (juce::Colours::white);
        g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool down, int, int, int, int, juce::ComboBox& cb) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        if (cb.findColour (juce::ComboBox::textColourId) == juce::Colours::white)      // selector de presets sobre el LCD
        {
            g.setColour (juce::Colour (0xff062c47).withAlpha (down ? 0.8f : 0.55f));
            g.fillRoundedRectangle (r, 4.f);
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawRoundedRectangle (r, 4.f, 1.f);
            juce::Path arrow;
            const float ax = (float) w - 12.f, ay = (float) h * 0.5f;
            arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
            g.setColour (juce::Colours::white);
            g.fillPath (arrow);
            return;
        }
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xffcfd6dc : 0xfffbfcfd), 0.f, 0.f, juce::Colour (0xffd9dfe4), 0.f, (float) h, false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (juce::Colour (0xff3a4550).withAlpha (0.75f));
        g.drawRoundedRectangle (r, 4.f, 1.1f);
        juce::Path arrow;
        const float ax = (float) w - 12.f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
        g.setColour (cb.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();
        const auto accent = b.findColour (juce::TextButton::buttonOnColourId);
        g.setGradientFill (on ? juce::ColourGradient (accent.brighter (0.25f), 0.f, r.getY(), accent.darker (0.15f), 0.f, r.getBottom(), false)
                              : juce::ColourGradient (juce::Colour (down ? 0xffc3cbd2 : over ? 0xfffdfdfe : 0xfff3f5f7), 0.f, r.getY(),
                                                      juce::Colour (0xffcdd4da), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (juce::Colour (0xff3a4550).withAlpha (0.8f));
        g.drawRoundedRectangle (r, 5.f, 1.1f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.4f : 0.9f));
        g.drawLine (r.getX() + 5.f, r.getY() + 1.5f, r.getRight() - 5.f, r.getY() + 1.5f);
    }
};

// ------------------------------------------------------------------ interfaz
class KeysUi : public juce::Component
{
    struct Cell { int p0; int p1; float w; bool fader; };      // p0 = -2: osciloscopio; p1 >= 0: combo apilado debajo
    struct Sec  { const char* title; juce::uint32 colour; int mask; int row; std::vector<Cell> cells; };
    struct SecRect { juce::Rectangle<int> r; const char* title; juce::Colour col; };

public:
    float startScale = 0.9f;

    explicit KeysUi (KeysKomodoAudioProcessor& p)
        : proc (p),
          kb (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
          presets (p), scope (p.scope, juce::Colour (0xff3aa0ff))
    {
        setLookAndFeel (&look);
        const juce::uint32 cFm = 0xff2ee6c5, cOrg = 0xffffb02e, cAna = 0xffa66cff, cCho = 0xfff06ab0,
                           cEnv = 0xff3aa0ff, cPlay = 0xff8ddb5a, cFx = 0xffff8f5a, cDly = 0xff4d8af0, cRev = 0xff9fb2c2, cOut = 0xff6b7a88;
        secs = {
            { "FM ELECTRIC PIANO", cFm,  1, 0, { { FMR, -1, 70, false }, { FMI, -1, 62, false }, { FMV, -1, 62, false }, { FMD, -1, 62, false }, { TNE, -1, 62, false }, { BRK, -1, 62, false } } },
            { "TONEWHEEL ORGAN",   cOrg, 2, 0, { { DB0, -1, 34, true }, { DB1, -1, 34, true }, { DB2, -1, 34, true }, { DB3, -1, 34, true }, { DB4, -1, 34, true },
                                                 { DB5, -1, 34, true }, { DB6, -1, 34, true }, { DB7, -1, 34, true }, { DB8, -1, 34, true },
                                                 { PRC, -1, 84, false }, { CLK, -1, 62, false } } },
            { "ANALOG KEYS",       cAna, 4, 0, { { WAV, -1, 84, false }, { ENS, -1, 62, false }, { CUT, -1, 62, false }, { RES, -1, 62, false },
                                                 { FEA, -1, 62, false }, { FDC, -1, 62, false }, { KEY, -1, 62, false } } },
            { "CHOIR",             cCho, 8, 0, { { VOW, -1, 70, false }, { BRT, -1, 70, false } } },
            { "SCOPE",             cEnv, 15, 0, { { -2, -1, 210, false } } },
            { "AMP ENVELOPE",      cEnv, 15, 1, { { ATK, -1, 62, false }, { DEC, -1, 62, false }, { SUS, -1, 62, false }, { REL, -1, 62, false } } },
            { "PLAY",              cPlay,15, 1, { { RNG, -1, 80, false }, { VEL, -1, 62, false }, { VIB, -1, 62, false }, { VBR, -1, 62, false } } },
            { "MODULATION FX",     cFx,  15, 1, { { MOD, -1, 104, false }, { MDR, -1, 62, false }, { MDD, -1, 62, false } } },
            { "DELAY",             cDly, 15, 2, { { DLT, -1, 80, false }, { DLM, -1, 62, false }, { DLF, -1, 62, false } } },
            { "REVERB",            cRev, 15, 2, { { RVM, -1, 62, false }, { RVS, -1, 62, false }, { RVD, -1, 62, false } } },
            { "OUTPUT",            cOut, 15, 2, { { DRV, -1, 62, false }, { TONE, -1, 62, false } } }
        };
        for (auto& s : secs) for (auto& c : s.cells) { build (c.p0, s.colour); build (c.p1, s.colour); }
        build (VOL, cOut);

        static const char* engNames[4] { "ELECTRIC PIANO", "ORGAN", "ANALOG KEYS", "CHOIR" };
        const juce::uint32 engCol[4] { cFm, cOrg, cAna, cCho };
        for (int i = 0; i < 4; ++i)
        {
            engBtn[i].setButtonText (engNames[i]);
            engBtn[i].setColour (juce::TextButton::buttonOnColourId, juce::Colour (engCol[i]));
            engBtn[i].onClick = [this, i] { if (engAtt) engAtt->setValueAsCompleteGesture ((float) i); };
            addAndMakeVisible (engBtn[i]);
        }
        presets.box.setColour (juce::ComboBox::textColourId, juce::Colours::white);
        presets.box.setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
        addAndMakeVisible (presets);
        addAndMakeVisible (scope);
        panicBtn.onClick = [this] { proc.panic(); };
        addAndMakeVisible (panicBtn);

        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xff2f7de1));
        kb.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x333aa0ff));
        kb.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffeef2f5));
        kb.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0d1319));
        kb.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6a7580));
        kb.setScrollButtonsVisible (false);
        kb.setAvailableRange (36, 96);
        addAndMakeVisible (kb);

        setSize (960, 644);
        engAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (paramIds[ENG]),
                                                              [this] (float v) { setEngine ((int) std::round (v)); });
        engAtt->sendInitialUpdate();
    }

    ~KeysUi() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth(), h = getHeight();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7f9fb), 0.f, 0.f, juce::Colour (0xffc9d0d7), 0.f, (float) h, false));
        g.fillAll();
        juce::Random rnd (7);
        for (int i = 0; i < 260; ++i)                       // aluminio cepillado
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.015f + rnd.nextFloat() * 0.03f));
            g.drawHorizontalLine ((int) (rnd.nextFloat() * (float) h), 0.f, (float) w);
        }
        g.setColour (juce::Colours::white);
        g.drawRect (0, 0, w, h, 1);
        g.setColour (juce::Colour (0xff3a4550).withAlpha (0.7f));
        g.drawRect (1, 1, w - 2, h - 2, 1);

        juce::Font logo (juce::FontOptions (28.f, juce::Font::bold));
        logo.setExtraKerningFactor (0.14f);
        g.setFont (logo);
        g.setColour (juce::Colours::white);
        g.drawText ("KEYSKOMODO", 25, 14, 360, 32, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff1d2833));
        g.drawText ("KEYSKOMODO", 24, 12, 360, 32, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.f, juce::Font::bold));
        g.setColour (juce::Colour (0xff2f7de1));
        g.drawText ("MUSIC WORKSTATION  -  SYNTH KEYS", 26, 44, 330, 14, juce::Justification::centredLeft);

        // pantalla LCD azul detras del selector de presets
        auto lcd = presets.getBounds().expanded (10, 8).toFloat();
        g.setColour (juce::Colour (0xff3a4550).withAlpha (0.55f));
        g.fillRoundedRectangle (lcd.expanded (3.f), 8.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b6ea8), 0.f, lcd.getY(), juce::Colour (0xff0d4f7f), 0.f, lcd.getBottom(), false));
        g.fillRoundedRectangle (lcd, 6.f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        for (float y = lcd.getY() + 2.f; y < lcd.getBottom() - 2.f; y += 3.f) g.drawHorizontalLine ((int) y, lcd.getX() + 3.f, lcd.getRight() - 3.f);

        juce::Font title (juce::FontOptions (10.f, juce::Font::bold));
        title.setExtraKerningFactor (0.12f);
        for (auto& s : secRects)
        {
            auto r = s.r.toFloat();
            g.setColour (juce::Colours::black.withAlpha (0.16f));
            g.fillRoundedRectangle (r.translated (0.f, 2.f), 7.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff1f4f6), 0.f, r.getY(), juce::Colour (0xffd5dbe0), 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 7.f);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.drawRoundedRectangle (r.translated (0.f, 0.6f), 7.f, 1.f);
            g.setColour (juce::Colour (0xff3a4550).withAlpha (0.65f));
            g.drawRoundedRectangle (r, 7.f, 1.1f);

            // titulo impreso + linea de color (como la serigrafia del panel)
            g.setFont (title);
            g.setColour (juce::Colour (0xff25303b));
            g.drawText (s.title, s.r.getX() + 10, s.r.getY() + 3, s.r.getWidth() - 20, 15, juce::Justification::centredLeft);
            g.setColour (s.col);
            g.fillRoundedRectangle (r.getX() + 8.f, r.getY() + 19.f, r.getWidth() - 16.f, 2.5f, 1.2f);
        }
    }

    void resized() override
    {
        secRects.clear();
        auto content = getLocalBounds().reduced (24, 0);

        auto head = content.removeFromTop (70);
        placeCell ({ VOL, -1, 0, false }, head.removeFromRight (70).reduced (0, 4));
        panicBtn.setBounds (head.removeFromRight (80).withSizeKeepingCentre (70, 26).translated (-8, -2));
        presets.setBounds (head.withSizeKeepingCentre (340, 28).translated (60, -2));

        auto eb = content.removeFromTop (30);
        const int ebw = (eb.getWidth() - 3 * 8) / 4;
        for (int i = 0; i < 4; ++i) engBtn[i].setBounds (eb.getX() + i * (ebw + 8), eb.getY(), ebw, 26);
        content.removeFromTop (8);

        static const int heights[3] { 150, 116, 116 };
        for (int row = 0; row < 3; ++row)
        {
            auto rowR = content.removeFromTop (heights[row]);
            content.removeFromTop (8);

            std::vector<Sec*> vis;
            for (auto& s : secs)
            {
                if (s.row != row) continue;
                const bool v = (s.mask & (1 << eng)) != 0;
                for (auto& c : s.cells) { setVisibleParam (c.p0, v); setVisibleParam (c.p1, v); }
                if (s.cells[0].p0 == -2) scope.setVisible (v);
                if (v) vis.push_back (&s);
            }
            const int gap = 8, n = (int) vis.size();
            float sum = 0.f;
            for (auto* s : vis) for (auto& c : s->cells) sum += c.w;
            const float scale = ((float) rowR.getWidth() - (float) (gap * (n - 1)) - 10.f * (float) n) / sum;

            int x = rowR.getX();
            for (auto* s : vis)
            {
                float sw = 0.f;
                for (auto& c : s->cells) sw += c.w * scale;
                juce::Rectangle<int> sr (x, rowR.getY(), (int) std::round (sw) + 10, rowR.getHeight());
                secRects.push_back ({ sr, s->title, juce::Colour (s->colour) });
                auto cells = sr.reduced (5, 0).withTrimmedTop (22).withTrimmedBottom (4);
                float cx = (float) cells.getX();
                for (auto& c : s->cells)
                {
                    placeCell (c, { (int) std::round (cx), cells.getY(), (int) std::round (c.w * scale), cells.getHeight() });
                    cx += c.w * scale;
                }
                x = sr.getRight() + gap;
            }
        }
        kb.setBounds (content.withTrimmedBottom (14));
        kb.setKeyWidth ((float) kb.getWidth() / 36.f);
        repaint();
    }

private:
    void setEngine (int e)
    {
        eng = juce::jlimit (0, 3, e);
        for (int i = 0; i < 4; ++i) engBtn[i].setToggleState (i == eng, juce::dontSendNotification);
        resized();
    }

    static juce::String shortName (int i)
    {
        static const char* n[NP] = { "ENGINE", "OCTAVE", "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "VELOCITY", "VIBRATO", "V.RATE",
                                     "RATIO", "INDEX", "VEL>IDX", "IDX DECAY", "TINE", "BARK",
                                     "16'", "5.3'", "8'", "4'", "2.6'", "2'", "1.6'", "1.3'", "1'", "PERCUSSION", "CLICK",
                                     "WAVE", "ENSEMBLE", "CUTOFF", "RES", "ENV", "ENV DEC", "KEY",
                                     "VOWEL", "BREATH",
                                     "DRIVE", "TONE", "EFFECT", "RATE", "DEPTH", "TIME", "MIX", "FDBK", "MIX", "SIZE", "DAMP", "MASTER" };
        return (i >= 0 && i < NP) ? n[i] : "";
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

    bool isFader (int i) const { return i >= DB0 && i <= DB8; }

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
            // drawbars como un Hammond: marrones (16', 5.3'), blancas (8', 4', 2', 1') y negras (2.6', 1.6', 1.3')
            juce::Colour c = col;
            if (fader) { static const juce::uint32 dc[9] { 0xff8a4a22, 0xff8a4a22, 0xfff2ede2, 0xfff2ede2, 0xff2a2a2a, 0xfff2ede2, 0xff2a2a2a, 0xff2a2a2a, 0xfff2ede2 };
                         c = juce::Colour (dc[i - DB0]); }
            s->setColour (juce::Slider::thumbColourId, c);
            s->setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (*s);
            knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramIds[i], *s));
            knobs[i] = std::move (s);
            addLabel (i, shortName (i));
        }
    }

    void setVisibleParam (int i, bool v)
    {
        if (i < 0) return;
        if (labels[i]) labels[i]->setVisible (v);
        if (combos[i]) combos[i]->setVisible (v);
        if (knobs[i])  knobs[i]->setVisible (v);
    }

    void placeCell (const Cell& c, juce::Rectangle<int> r)
    {
        auto comboWithLabel = [&] (int i, juce::Rectangle<int>& area)
        {
            labels[i]->setBounds (area.removeFromTop (13));
            combos[i]->setBounds (area.removeFromTop (22).reduced (3, 0));
        };

        if (c.p0 == -2) { scope.setBounds (r.reduced (2, 0)); return; }
        if (c.p1 >= 0)
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

    KeysKomodoAudioProcessor& proc;
    KeysLook look;
    juce::MidiKeyboardComponent kb;
    PresetBar presets;
    ScopeView scope;
    juce::TextButton engBtn[4], panicBtn { "PANIC" };
    std::vector<Sec> secs;
    std::vector<SecRect> secRects;
    int eng = 0;
    bool built[NP] {};
    std::unique_ptr<juce::Label> labels[NP];
    std::unique_ptr<juce::ComboBox> combos[NP];
    std::unique_ptr<juce::Slider> knobs[NP];
    // los attachments se declaran al final: se destruyen primero
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;
    std::unique_ptr<juce::ParameterAttachment> engAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeysUi)
};
