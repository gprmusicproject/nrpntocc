#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Barra orizzontale che mostra l'ultimo valore CC inviato (0-127).
class CcMeter : public juce::Component
{
public:
    void setValue (int v)
    {
        if (v != value) { value = v; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillRoundedRectangle (r, 3.0f);

        if (value >= 0)
        {
            const float w = juce::jmax (3.0f, r.getWidth() * (float) value / 127.0f);
            g.setColour (juce::Colour (0xff4aa3ff));
            g.fillRoundedRectangle (r.withWidth (w), 3.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
        g.drawText (value >= 0 ? "CC out: " + juce::String (value) : juce::String ("CC out: -"),
                    getLocalBounds(), juce::Justification::centred);
    }

private:
    int value = -1;
};

//==============================================================================
class NrpnToCcEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit NrpnToCcEditor (NrpnToCcProcessor&);
    ~NrpnToCcEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateMonitorText();

    NrpnToCcProcessor& proc;

    juce::Label titleLabel;
    juce::Label inLabel, outLabel, modeLabel, maxLabel;
    juce::ComboBox inputChannelBox, outputChannelBox, valueModeBox;
    juce::Slider inputMaxSlider;
    juce::ToggleButton passthroughButton { "Pass through original NRPN" };
    juce::ToggleButton dedupeButton { "Filter repeated CC" };
    juce::TextButton   resetLearnButton { "Reset learning" };

    juce::Label infoLabel;
    juce::Label monitorTitle;
    juce::Label monitorText;
    CcMeter     ccMeter;

    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ComboAttach>  inAtt, outAtt, modeAtt;
    std::unique_ptr<ButtonAttach> passAtt, dedupeAtt;
    std::unique_ptr<SliderAttach> maxAtt;

    uint32_t lastSeenCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NrpnToCcEditor)
};
