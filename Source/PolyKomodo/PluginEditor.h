#pragma once
#include "PluginProcessor.h"

// Editor ligero: perillas, combos y teclado MIDI, todo generado desde la lista de parametros.
class PolyKomodoEditor : public juce::AudioProcessorEditor
{
public:
    explicit PolyKomodoEditor (PolyKomodoAudioProcessor& p)
        : AudioProcessorEditor (&p),
          kb (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        const auto orange = juce::Colour (0xffff9a2e);
        const auto labCol = juce::Colour (0xffb09c78);

        auto makeLabel = [&] (const juce::String& text)
        {
            auto lab = std::make_unique<juce::Label> (juce::String(), text);
            lab->setJustificationType (juce::Justification::centred);
            lab->setColour (juce::Label::textColourId, labCol);
            lab->setFont (juce::FontOptions (12.f));
            return lab;
        };

        // selector de presets
        for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
        presetBox.setSelectedItemIndex (p.getCurrentProgram(), juce::dontSendNotification);
        presetBox.onChange = [this, &p] { p.setCurrentProgram (presetBox.getSelectedItemIndex()); };
        auto presetLab = makeLabel ("Preset");
        presetLab->attachToComponent (&presetBox, false);
        addAndMakeVisible (presetBox);
        addAndMakeVisible (*presetLab);
        labels.push_back (std::move (presetLab));

        for (int i = 0; i < NP; ++i)
        {
            auto* par = p.apvts.getParameter (paramIds[i]);
            auto lab = makeLabel (par->getName (24));

            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (par))   // combos
            {
                auto cb = std::make_unique<juce::ComboBox>();
                cb->addItemList (choice->choices, 1);
                lab->attachToComponent (cb.get(), false);
                addAndMakeVisible (*cb);
                combos.push_back (std::move (cb));
                comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, paramIds[i], *combos.back()));
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
                knobIdx.push_back (i);
                knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, paramIds[i], *knobs.back()));
            }
            addAndMakeVisible (*lab);
            labels.push_back (std::move (lab));
        }
        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, orange);
        addAndMakeVisible (kb);
        setSize (800, 520);
    }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2d221a), 0, 0, juce::Colour (0xff120d09), 0, (float) getHeight(), false));
        g.fillAll();
        g.setColour (juce::Colour (0xffff9a2e));
        g.setFont (juce::FontOptions (20.f, juce::Font::bold));
        g.drawText ("POLYKOMODO", 16, 6, 300, 28, juce::Justification::left);
        g.fillRect (0, 38, getWidth(), 2);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (34);
        auto top = r.removeFromTop (70).withTrimmedTop (30);                      // fila de combos
        const int nc = (int) combos.size() + 1, cw = top.getWidth() / nc;
        presetBox.setBounds (top.getX() + 4, top.getY(), cw * 2 - 8, 24);          // el preset ocupa doble ancho
        for (size_t i = 0; i < combos.size(); ++i)
            combos[i]->setBounds (top.getX() + ((int) i + 2) * cw + 4, top.getY(), cw - 8, 24);

        kb.setBounds (r.removeFromBottom (80));

        const int cols = 8, cellW = r.getWidth() / cols, cellH = 112;
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const int id = knobIdx[i];
            const int row = id <= LP ? 0 : id <= VOL ? 1 : 2;
            const int col = row == 0 ? id - SAW : row == 1 ? id - CUT : id - FA;
            knobs[i]->setBounds (r.getX() + col * cellW + 10, r.getY() + row * cellH + 22, cellW - 20, cellH - 26);
        }
    }

private:
    juce::MidiKeyboardComponent kb;
    juce::ComboBox presetBox;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<int> knobIdx;
    // los attachments se declaran al final: se destruyen primero
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyKomodoEditor)
};
