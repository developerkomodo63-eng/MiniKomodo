#pragma once
#include "PluginProcessor.h"
#include "Common/KomodoUi.h"

// ------------------------------------------------------------------ aspecto "Minimoog": madera, panel negro, perillas con relieve
class MoogLook : public juce::LookAndFeel_V4
{
public:
    static juce::Colour orange() { return juce::Colour (0xffff9a2e); }

    MoogLook()
    {
        const juce::Colour txt (0xffeed7a6);
        setColour (juce::ComboBox::textColourId, txt);
        setColour (juce::ComboBox::arrowColourId, orange());
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1612));
        setColour (juce::PopupMenu::textColourId, txt);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, orange());
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff120d09));
        setColour (juce::Label::textColourId, juce::Colour (0xffbfa77c));
        setColour (juce::Slider::textBoxTextColourId, txt);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha (0.75f));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha (0.4f));
        setColour (juce::TextButton::textColourOffId, orange());
        setColour (juce::TextButton::textColourOnId, orange());
        setColour (juce::ToggleButton::textColourId, txt);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.f)); }
    juce::Font getPopupMenuFont() override               { return juce::Font (juce::FontOptions (13.f)); }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (juce::FontOptions (11.f));
        return l;
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        const float size = (float) juce::jmin (w, h);
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float R = size * 0.5f - 2.5f;
        const float ang = a0 + pos * (a1 - a0);

        // arco de valor
        juce::Path track, val;
        track.addCentredArc (cx, cy, R, R, 0.f, a0, a1, true);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.strokePath (track, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        val.addCentredArc (cx, cy, R, R, 0.f, a0, ang, true);
        g.setColour (orange());
        g.strokePath (val, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = R - 5.f;
        // sombra proyectada
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (cx - kr + 1.5f, cy - kr + 4.f, kr * 2.f, kr * 2.f);
        // falda metalica
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7d7167), cx - kr * 0.6f, cy - kr * 0.8f,
                                                  juce::Colour (0xff0b0908), cx + kr * 0.9f, cy + kr, true));
        g.fillEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 1.f);
        // cuerpo
        const float br = kr - 3.5f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff574e46), cx - br * 0.45f, cy - br * 0.6f,
                                                  juce::Colour (0xff0d0a08), cx + br * 0.7f, cy + br * 0.9f, true));
        g.fillEllipse (cx - br, cy - br, br * 2.f, br * 2.f);
        // brillo especular
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), cx - br * 0.35f, cy - br * 0.55f,
                                                  juce::Colours::transparentWhite, cx - br * 0.35f, cy - br * 0.05f, true));
        g.fillEllipse (cx - br * 0.8f, cy - br * 0.95f, br * 1.2f, br * 0.8f);
        // indicador
        juce::Path ptr;
        ptr.addRoundedRectangle (-1.5f, -br + 1.5f, 3.f, br * 0.55f, 1.3f);
        g.setColour (orange());
        g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff1a1410 : 0xff40362e), 0.f, 0.f,
                                                  juce::Colour (0xff120e0b), 0.f, (float) h, false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 4.f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.drawLine (r.getX() + 4.f, r.getY() + 1.5f, r.getRight() - 4.f, r.getY() + 1.5f);
        juce::Path arrow;
        const float ax = (float) w - 12.f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
        g.setColour (orange());
        g.fillPath (arrow);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff1a1410 : over ? 0xff52463b : 0xff40362e), 0.f, r.getY(),
                                                  juce::Colour (0xff15100c), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 4.f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawLine (r.getX() + 4.f, r.getY() + 1.5f, r.getRight() - 4.f, r.getY() + 1.5f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        const float d = 14.f, cy = r.getCentreY(), cx = r.getX() + 12.f;
        if (b.getToggleState())
        {
            g.setGradientFill (juce::ColourGradient (orange().withAlpha (0.55f), cx, cy, juce::Colours::transparentBlack, cx + 14.f, cy, true));
            g.fillEllipse (cx - 14.f, cy - 14.f, 28.f, 28.f);
        }
        g.setColour (juce::Colour (0xff050403));
        g.fillEllipse (cx - d * 0.5f - 1.5f, cy - d * 0.5f - 1.5f, d + 3.f, d + 3.f);
        g.setGradientFill (b.getToggleState()
            ? juce::ColourGradient (juce::Colour (0xffffd9a0), cx - 2.f, cy - 3.f, orange().darker (0.2f), cx + 4.f, cy + 5.f, true)
            : juce::ColourGradient (juce::Colour (0xff4a3d33), cx - 2.f, cy - 3.f, juce::Colour (0xff17110d), cx + 4.f, cy + 5.f, true));
        g.fillEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d);
        g.setColour (juce::Colours::white.withAlpha (over ? 0.3f : 0.15f));
        g.drawEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d, 1.f);
        g.setColour (findColour (juce::ToggleButton::textColourId));
        g.setFont (juce::FontOptions (12.f));
        g.drawText (b.getButtonText(), (int) (cx + 12.f), 0, b.getWidth() - 30, b.getHeight(), juce::Justification::centredLeft);
    }
};

// ------------------------------------------------------------------ editor
class MiniUi : public juce::Component
{
   #if KOMODO_FULL
    static constexpr const char* kName = "MINIKOMODO";
    static constexpr const char* kSub  = "3-OSCILLATOR ANALOG SYNTHESIZER  -  FULL";
    static constexpr int kRowH = 116;
   #else
    static constexpr const char* kName = "MINIKOMODO LITE";
    static constexpr const char* kSub  = "LIVE EDITION  -  LOW CPU  -  PROGRAM CHANGE";
    static constexpr int kRowH = 116;
   #endif
    struct Cell { int p0; int p1; float w; };                  // p1 >= 0: segundo combo apilado debajo
    struct Sec  { const char* title; std::vector<Cell> cells; };
    struct SecRect { juce::Rectangle<int> r; const char* title; };

public:
    float startScale = 1.f;

    explicit MiniUi (MiniKomodoAudioProcessor& p)
        : proc (p),
          kb (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
          presets (p)
   #if KOMODO_FULL
          , scope (p.scope, juce::Colour (0xffff9a2e))
   #endif
    {
        setLookAndFeel (&look);
        panicBtn.setTooltip ("Corta todas las notas, el arpegiador y las colas de efectos");
        rows = {
            { { "OSCILLATOR BANK", { { R0, W0, 104 }, { R1, W1, 104 }, { R2, W2, 104 }, { D1, -1, 60 }, { D2, -1, 60 } } },
              { "MIXER",           { { L0, -1, 60 }, { L1, -1, 60 }, { L2, -1, 60 }, { NZ, -1, 60 } } } },
            { { "FILTER",          { { CUT, -1, 60 }, { RES, -1, 60 }, { AMT, -1, 60 }, { KEY, -1, 60 }, { LFF, -1, 60 } } },
              { "FILTER ENVELOPE", { { FA, -1, 60 }, { FD, -1, 60 }, { FS, -1, 60 } } },
              { "LOUDNESS ENVELOPE", { { AA, -1, 60 }, { AD, -1, 60 }, { AS, -1, 60 }, { DEC, -1, 84 } } } },
            { { "LFO",             { { LSH, -1, 100 }, { LR, -1, 60 }, { LA, -1, 60 } } },
              { "PERFORMANCE",     { { GL, -1, 60 }, { VEL, -1, 60 }, { DRF, -1, 60 }, { MODE, CHO, 100 } } },
              { "ARPEGGIATOR",     { { ARP, ARR, 100 }, { ARO, -1, 72 }, { ARG, -1, 60 } } } }
           #if KOMODO_FULL
            ,
            { { "OSC MOD",         { { SYN, -1, 70 }, { XMD, -1, 60 }, { SPR, -1, 60 } } },
              { "FILTER TYPE",     { { FTY, -1, 100 }, { FDR, -1, 60 } } },
              { "LFO 2",           { { L2S, -1, 100 }, { L2R, -1, 60 } } },
              { "OUTPUT",          { { DRV, -1, 60 } } } },
            { { "MODULATION MATRIX", { { M1S, M1D, 100 }, { M1A, -1, 56 }, { M2S, M2D, 100 }, { M2A, -1, 56 },
                                       { M3S, M3D, 100 }, { M3A, -1, 56 }, { M4S, M4D, 100 }, { M4A, -1, 56 } } } },
            { { "DELAY",           { { DLT, -1, 90 }, { DLM, -1, 60 }, { DLF, -1, 60 } } },
              { "REVERB",          { { RVM, -1, 60 }, { RVS, -1, 60 }, { RVD, -1, 60 } } },
              { "SCOPE",           { { -2, -1, 150 } } } }
           #endif
        };
        for (auto& row : rows) for (auto& s : row) for (auto& c : s.cells) { build (c.p0); build (c.p1); }
        build (VOL);
       #if KOMODO_FULL
        addAndMakeVisible (scope);
       #endif
        panicBtn.onClick = [this] { proc.panic(); };
        addAndMakeVisible (panicBtn);

        addAndMakeVisible (presets);
        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xffff9a2e));
        kb.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x33ff9a2e));
        kb.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff1e4c6));
        kb.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff17110d));
        kb.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6b5a44));
        kb.setScrollButtonsVisible (false);
        kb.setAvailableRange (36, 96);
        addAndMakeVisible (kb);
       #if KOMODO_FULL
        startScale = 0.85f;
        setSize (800, 912);
       #else
        setSize (800, 556);
       #endif
    }

    ~MiniUi() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth(), h = getHeight(), cheek = 28;
        g.fillAll (juce::Colour (0xff0b0807));

        // laterales de madera
        auto wood = [&] (juce::Rectangle<int> r, int seed)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a5028), (float) r.getX(), 0.f,
                                                      juce::Colour (0xff4a2412), (float) r.getRight(), 0.f, false));
            g.fillRect (r);
            juce::Random rnd (seed);
            for (int i = 0; i < 46; ++i)
            {
                const float gx = (float) r.getX() + rnd.nextFloat() * (float) r.getWidth();
                g.setColour ((rnd.nextBool() ? juce::Colours::black : juce::Colour (0xffd9a066)).withAlpha (0.05f + rnd.nextFloat() * 0.08f));
                g.drawLine (gx, 0.f, gx + rnd.nextFloat() * 4.f - 2.f, (float) h, 0.6f + rnd.nextFloat());
            }
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), 0.f, 0.f,
                                                      juce::Colours::transparentWhite, 0.f, (float) h * 0.25f, false));
            g.fillRect (r);
        };
        wood ({ 0, 0, cheek, h }, 11);
        wood ({ w - cheek, 0, cheek, h }, 29);

        // panel
        auto panel = juce::Rectangle<float> ((float) cheek, 0.f, (float) (w - 2 * cheek), (float) h);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35302a), 0.f, 0.f, juce::Colour (0xff0c0a08), 0.f, (float) h, false));
        g.fillRect (panel);
        juce::Random noise (5);
        for (int i = 0; i < 260; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.018f + noise.nextFloat() * 0.02f));
            g.fillRect (panel.getX() + noise.nextFloat() * panel.getWidth(), noise.nextFloat() * (float) h, 1.f, 1.f);
        }
        // sombra interior junto a la madera
        for (int side = 0; side < 2; ++side)
        {
            const float px = side == 0 ? panel.getX() : panel.getRight();
            const float dx = side == 0 ? 10.f : -10.f;
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), px, 0.f, juce::Colours::transparentBlack, px + dx, 0.f, false));
            g.fillRect (juce::Rectangle<float> (side == 0 ? px : px - 10.f, 0.f, 10.f, (float) h));
        }

        // tornillos
        auto screw = [&] (float sx, float sy)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8b0a6), sx - 2.f, sy - 2.f, juce::Colour (0xff2a2622), sx + 4.f, sy + 4.f, true));
            g.fillEllipse (sx - 5.f, sy - 5.f, 10.f, 10.f);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawLine (sx - 3.f, sy + 1.f, sx + 3.f, sy - 1.f, 1.4f);
        };
        screw (panel.getX() + 12.f, 12.f);  screw (panel.getRight() - 12.f, 12.f);
        screw (panel.getX() + 12.f, (float) h - 12.f);  screw (panel.getRight() - 12.f, (float) h - 12.f);

        // titulo
        juce::Font logo (juce::FontOptions (28.f, juce::Font::bold));
        logo.setExtraKerningFactor (0.14f);
        g.setFont (logo);
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.drawText (kName, 52, 15, 340, 34, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xffff9a2e));
        g.drawText (kName, 50, 12, 340, 34, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.f));
        g.setColour (juce::Colour (0xffa88958));
        g.drawText (kSub, 52, 46, 330, 14, juce::Justification::centredLeft);

        // secciones hundidas
        juce::Font title (juce::FontOptions (10.5f, juce::Font::bold));
        title.setExtraKerningFactor (0.1f);
        for (auto& s : secRects)
        {
            auto r = s.r.toFloat();
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (r.translated (0.f, 1.f), 6.f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.5f), 0.f, r.getY(),
                                                      juce::Colours::black.withAlpha (0.25f), 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 6.f);
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.drawRoundedRectangle (r, 6.f, 1.2f);
            g.setFont (title);
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.drawText (s.title, s.r.getX() + 10, s.r.getY() + 6, s.r.getWidth() - 16, 14, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xffe0a560));
            g.drawText (s.title, s.r.getX() + 10, s.r.getY() + 5, s.r.getWidth() - 16, 14, juce::Justification::centredLeft);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawHorizontalLine (s.r.getY() + 21, (float) s.r.getX() + 8.f, (float) s.r.getRight() - 8.f);
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawHorizontalLine (s.r.getY() + 22, (float) s.r.getX() + 8.f, (float) s.r.getRight() - 8.f);
        }
    }

    void resized() override
    {
        secRects.clear();
        auto content = getLocalBounds().reduced (28, 0).reduced (14, 0);

        auto head = content.removeFromTop (74);
        presets.setBounds (head.withSizeKeepingCentre (300, 26).translated (60, 2));
        placeCell ({ VOL, -1, 0 }, head.removeFromRight (74).reduced (0, 4));
        panicBtn.setBounds (head.removeFromRight (70).withSizeKeepingCentre (62, 24).translated (0, -2));
        content.removeFromTop (2);

        for (auto& row : rows)
        {
            auto rowR = content.removeFromTop (kRowH);
            content.removeFromTop (6);
            const int gap = 10, n = (int) row.size();
            float sum = 0.f;
            for (auto& s : row) for (auto& c : s.cells) sum += c.w;
            const float scale = ((float) rowR.getWidth() - (float) (gap * (n - 1)) - 12.f * (float) n) / sum;

            int x = rowR.getX();
            for (auto& s : row)
            {
                float sw = 0.f;
                for (auto& c : s.cells) sw += c.w * scale;
                juce::Rectangle<int> sr (x, rowR.getY(), (int) std::round (sw) + 12, rowR.getHeight());
                secRects.push_back ({ sr, s.title });
                auto cells = sr.reduced (6, 0).withTrimmedTop (26).withTrimmedBottom (4);
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
    static juce::String labelFor (int i, const juce::String& def)
    {
       #if KOMODO_FULL
        if (i >= M1S && i <= M4A) { static const char* n[3] { "Fuente", "Destino", "Cant." }; return n[(i - M1S) % 3]; }
       #endif
        return def;
    }

    void addLabel (int i, const juce::String& text)
    {
        auto l = std::make_unique<juce::Label> (juce::String(), text);
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::FontOptions (10.5f));
        l->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*l);
        labels[i] = std::move (l);
    }

    void build (int i)
    {
        if (i < 0 || built[i]) return;
        built[i] = true;
        auto* par = proc.apvts.getParameter (paramIds[i]);

        if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (par))
        {
            auto cb = std::make_unique<juce::ComboBox>();
            cb->addItemList (ch->choices, 1);
            addAndMakeVisible (*cb);
            comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramIds[i], *cb));
            combos[i] = std::move (cb);
            addLabel (i, labelFor (i, par->getName (24)));
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (par) != nullptr)
        {
            auto tb = std::make_unique<juce::ToggleButton> (i == DEC ? "Decay" : "Sync");
            addAndMakeVisible (*tb);
            togAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramIds[i], *tb));
            toggles[i] = std::move (tb);
        }
        else
        {
            auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
            s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 14);
            s->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
            addAndMakeVisible (*s);
            knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramIds[i], *s));
            knobs[i] = std::move (s);
            addLabel (i, labelFor (i, par->getName (24)));
        }
    }

    void placeCell (const Cell& c, juce::Rectangle<int> r)
    {
       #if KOMODO_FULL
        if (c.p0 == -2) { scope.setBounds (r.reduced (2, 0)); return; }
       #endif
        auto comboWithLabel = [&] (int i, juce::Rectangle<int>& area)
        {
            labels[i]->setBounds (area.removeFromTop (13));
            combos[i]->setBounds (area.removeFromTop (22).reduced (3, 0));
        };

        if (c.p1 >= 0)
        {
            auto a = r.withSizeKeepingCentre (r.getWidth(), 75);
            comboWithLabel (c.p0, a);
            a.removeFromTop (5);
            comboWithLabel (c.p1, a);
        }
        else if (combos[c.p0])
        {
            auto a = r.withSizeKeepingCentre (r.getWidth(), 35);
            comboWithLabel (c.p0, a);
        }
        else if (toggles[c.p0])
            toggles[c.p0]->setBounds (r.withSizeKeepingCentre (r.getWidth() - 2, 24));
        else
        {
            labels[c.p0]->setBounds (r.removeFromTop (13));
            knobs[c.p0]->setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 64), r.getHeight()));
        }
    }

    MiniKomodoAudioProcessor& proc;
    MoogLook look;
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
    std::unique_ptr<juce::ToggleButton> toggles[NP];
    // los attachments se declaran al final: se destruyen primero
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> togAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniUi)
};
