#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "BrandLookAndFeel.h"

//==============================================================================
// Pill-shaped bar showing the last CC value sent (0-127). The label is dark
// where it sits on the amber bar and light on the empty track.
class CcMeter : public juce::Component
{
public:
    void setValue (int v)
    {
        if (v != value) { value = v; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        const float radius = r.getHeight() * 0.5f;

        g.setColour (brand::raised);
        g.fillRoundedRectangle (r, radius);

        juce::Rectangle<float> bar;
        if (value >= 0)
        {
            bar = r.withWidth (juce::jmax (r.getHeight(), r.getWidth() * (float) value / 127.0f));
            g.setColour (brand::accent);
            g.fillRoundedRectangle (bar, radius);
        }

        const auto label = value >= 0 ? "CC out: " + juce::String (value) : juce::String ("CC out: -");
        g.setFont (brand::monoFont (12.0f));

        g.setColour (brand::text);
        g.drawText (label, getLocalBounds(), juce::Justification::centred);

        if (! bar.isEmpty())
        {
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (bar.toNearestInt());
            g.setColour (brand::bg);
            g.drawText (label, getLocalBounds(), juce::Justification::centred);
        }
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

    static constexpr int headerHeight = 96;

    NrpnToCcProcessor& proc;
    BrandLookAndFeel   lnf;   // declared first: outlives every component that uses it

    std::unique_ptr<juce::Drawable> logo;   // GPR music project lockup with the claim
    juce::Rectangle<int> productArea;       // product name + version, right of the logo

    juce::Label settingsTitle;
    juce::Label inLabel, outLabel, modeLabel, maxLabel;
    juce::ComboBox inputChannelBox, outputChannelBox, valueModeBox;
    juce::Slider inputMaxSlider;
    juce::ToggleButton passthroughButton { "Pass through original NRPN" };
    juce::ToggleButton dedupeButton { "Filter repeated CC" };
    juce::TextButton   resetLearnButton { "Reset learning" };

    juce::Label infoLabel;
    juce::Label monitorTitle;
    juce::Label monitorText;
    juce::Rectangle<int> monitorPanel;      // rounded card behind the monitor text
    CcMeter     ccMeter;

    juce::HyperlinkButton siteLink { "www.gprmusic.com", juce::URL ("https://www.gprmusic.com") };

    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ComboAttach>  inAtt, outAtt, modeAtt;
    std::unique_ptr<ButtonAttach> passAtt, dedupeAtt;
    std::unique_ptr<SliderAttach> maxAtt;

    uint32_t lastSeenCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NrpnToCcEditor)
};
