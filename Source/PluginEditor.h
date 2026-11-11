#pragma once
#include "PluginProcessor.h"

// Editor ligero: perillas, combos y teclado MIDI, todo generado desde la lista de parametros.
class MiniKomodoEditor : public juce::AudioProcessorEditor
{
public:
    explicit MiniKomodoEditor (MiniKomodoAudioProcessor& p)
        : AudioProcessorEditor (&p),
          kb (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        const auto orange = juce::Colour (0xffff9a2e);

        for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
        presetBox.setSelectedItemIndex (p.getCurrentProgram(), juce::dontSendNotification);
        presetBox.onChange = [this, &p] { p.setCurrentProgram (presetBox.getSelectedItemIndex()); };
        auto presetLab = std::make_unique<juce::Label> (juce::String(), "Preset");
        presetLab->setJustificationType (juce::Justification::centred);
        presetLab->setColour (juce::Label::textColourId, juce::Colour (0xffb09c78));
        presetLab->setFont (juce::FontOptions (12.f));
        presetLab->attachToComponent (&presetBox, false);
        addAndMakeVisible (presetBox);
        addAndMakeVisible (*presetLab);
        labels.push_back (std::move (presetLab));

        for (int i = 0; i < NP; ++i)
        {
            auto* par = p.apvts.getParameter (paramIds[i]);
            auto lab = std::make_unique<juce::Label> (juce::String(), par->getName (24));
            lab->setJustificationType (juce::Justification::centred);
            lab->setColour (juce::Label::textColourId, juce::Colour (0xffb09c78));
            lab->setFont (juce::FontOptions (12.f));

            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (par))   // combos
            {
                auto cb = std::make_unique<juce::ComboBox>();
                cb->addItemList (choice->choices, 1);
                lab->attachToComponent (cb.get(), false);
                addAndMakeVisible (*cb);
                combos.push_back (std::move (cb));
                comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, paramIds[i], *combos.back()));
            }
            else if (dynamic_cast<juce::AudioParameterBool*> (par) != nullptr)    // interruptor (Decay on)
            {
                auto tb = std::make_unique<juce::ToggleButton> ("Decay on");
                tb->setColour (juce::ToggleButton::textColourId, juce::Colours::white);
                tb->setColour (juce::ToggleButton::tickColourId, orange);
                addAndMakeVisible (*tb);
                toggles.push_back (std::move (tb));
                toggleCell.push_back (cells++);
                togAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, paramIds[i], *toggles.back()));
                continue;                                                         // sin label propio
            }
            else                                                                  // perillas
            {
                auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
                s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
                s->setColour (juce::Slider::rotarySliderFillColourId, orange);
                s->setColour (juce::Slider::thumbColourId, juce::Colours::white);
                lab->attachToComponent (s.get(), false);
                addAndMakeVisible (*s);
                knobs.push_back (std::move (s));
                knobCell.push_back (cells++);
                knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, paramIds[i], *knobs.back()));
            }
            addAndMakeVisible (*lab);
            labels.push_back (std::move (lab));
        }
        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, orange);
        addAndMakeVisible (kb);
        setSize (790, 600);
    }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2d221a), 0, 0, juce::Colour (0xff120d09), 0, (float) getHeight(), false));
        g.fillAll();
        g.setColour (juce::Colour (0xffff9a2e));
        g.setFont (juce::FontOptions (20.f, juce::Font::bold));
        g.drawText ("MINIKOMODO", 16, 6, 300, 28, juce::Justification::left);
        g.fillRect (0, 38, getWidth(), 2);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (34);

        // combos: 6 en la primera fila (rango/onda de los 3 osc), el resto (modo, LFO, chorus) en la segunda
        auto row1 = r.removeFromTop (62).withTrimmedTop (30);
        auto row2 = r.removeFromTop (62).withTrimmedTop (30);
        const int cw = row1.getWidth() / 6;
        for (size_t i = 0; i < combos.size(); ++i)
        {
            auto& row = i < 6 ? row1 : row2;
            const int col = (int) (i < 6 ? i : i - 6);
            combos[i]->setBounds (row.getX() + col * cw + 4, row.getY(), cw - 8, 24);
        }

        presetBox.setBounds (row2.getX() + 3 * cw + 4, row2.getY(), cw * 3 - 8, 24);   // columnas 4-6 de la fila 2

        kb.setBounds (r.removeFromBottom (100));

        const int cols = 8, cellW = r.getWidth() / cols, cellH = 112;
        auto place = [&] (juce::Component& c, int cell, int dy, int h, int dw)
            { c.setBounds (r.getX() + (cell % cols) * cellW + 10, r.getY() + (cell / cols) * cellH + dy, cellW - dw, h); };
        for (size_t i = 0; i < knobs.size(); ++i)   place (*knobs[i], knobCell[i], 22, cellH - 26, 20);
        for (size_t i = 0; i < toggles.size(); ++i) place (*toggles[i], toggleCell[i], 40, 24, 12);
    }

private:
    int cells = 0;
    juce::MidiKeyboardComponent kb;
    juce::ComboBox presetBox;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<int> knobCell, toggleCell;
    // los attachments se declaran al final: se destruyen primero
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> togAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniKomodoEditor)
};
