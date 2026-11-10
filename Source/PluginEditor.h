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
        for (int i = 0; i < NP; ++i)
        {
            auto* par = p.apvts.getParameter (paramIds[i]);
            auto lab = std::make_unique<juce::Label> ({}, par->getName (24));
            lab->setJustificationType (juce::Justification::centred);
            lab->setColour (juce::Label::textColourId, juce::Colour (0xffb09c78));
            lab->setFont (juce::FontOptions (12.f));

            if (i <= W2)                                   // combos de rango / onda
            {
                auto cb = std::make_unique<juce::ComboBox>();
                cb->addItemList (dynamic_cast<juce::AudioParameterChoice*> (par)->choices, 1);
                lab->attachToComponent (cb.get(), false);
                addAndMakeVisible (*cb);
                combos.push_back (std::move (cb));
                comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, paramIds[i], *combos.back()));
            }
            else if (i == DEC)                             // interruptor Decay on
            {
                auto tb = std::make_unique<juce::ToggleButton> ("Decay on");
                tb->setColour (juce::ToggleButton::textColourId, juce::Colours::white);
                tb->setColour (juce::ToggleButton::tickColourId, orange);
                addAndMakeVisible (*tb);
                toggles.push_back (std::move (tb));
                togAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, paramIds[i], *toggles.back()));
                continue;                                  // sin label propio
            }
            else                                           // perillas
            {
                auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
                s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
                s->setColour (juce::Slider::rotarySliderFillColourId, orange);
                s->setColour (juce::Slider::thumbColourId, juce::Colours::white);
                lab->attachToComponent (s.get(), false);
                addAndMakeVisible (*s);
                knobs.push_back (std::move (s));
                knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, paramIds[i], *knobs.back()));
            }
            addAndMakeVisible (*lab);
            labels.push_back (std::move (lab));
        }
        kb.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, orange);
        addAndMakeVisible (kb);
        setSize (790, 540);
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
        auto top = r.removeFromTop (70).withTrimmedTop (30);              // fila de combos
        const int cw = top.getWidth() / 6;
        for (size_t i = 0; i < combos.size(); ++i)
            combos[i]->setBounds (top.getX() + (int) i * cw + 4, top.getY(), cw - 8, 24);

        auto kbArea = r.removeFromBottom (100);
        kb.setBounds (kbArea);

        const int cols = 7, cellW = r.getWidth() / cols, cellH = 112;
        for (size_t i = 0; i < knobs.size(); ++i)
            knobs[i]->setBounds (r.getX() + ((int) i % cols) * cellW + 10, r.getY() + ((int) i / cols) * cellH + 22, cellW - 20, cellH - 26);
        const size_t k = knobs.size();
        if (! toggles.empty())
            toggles[0]->setBounds (r.getX() + ((int) k % cols) * cellW + 10, r.getY() + ((int) k / cols) * cellH + 40, cellW - 12, 24);
    }

private:
    juce::MidiKeyboardComponent kb;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    // los attachments se declaran al final: se destruyen primero
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> togAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniKomodoEditor)
};
