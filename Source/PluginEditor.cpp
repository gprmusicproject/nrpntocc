#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
NrpnToCcEditor::NrpnToCcEditor (NrpnToCcProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    // --- Header: GPR music project lockup (symbol, name and claim) ---
    logo = juce::Drawable::createFromImageData (BinaryData::gprlogohorizontal_svg,
                                                BinaryData::gprlogohorizontal_svgSize);
    if (logo != nullptr)
    {
        logo->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*logo);
    }

    // Section labels in the site style: mono, uppercase, amber.
    auto setupSectionTitle = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (brand::monoFont (11.0f).withExtraKerningFactor (0.18f));
        l.setColour (juce::Label::textColourId, brand::accent);
        l.setBorderSize ({});
        addAndMakeVisible (l);
    };

    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions{}.withHeight (14.0f)));
        l.setColour (juce::Label::textColourId, brand::textMuted);
        l.setJustificationType (juce::Justification::centredLeft);
        l.setBorderSize ({});
        addAndMakeVisible (l);
    };

    setupSectionTitle (settingsTitle, "SETTINGS");

    // --- Input channel ---
    setupLabel (inLabel, "Input channel");
    inputChannelBox.addItem ("Omni (all)", 1);
    for (int i = 1; i <= 16; ++i)
        inputChannelBox.addItem (juce::String (i), i + 1);
    addAndMakeVisible (inputChannelBox);

    // --- Output channel ---
    setupLabel (outLabel, "Output channel");
    outputChannelBox.addItem ("Same as input", 1);
    for (int i = 1; i <= 16; ++i)
        outputChannelBox.addItem (juce::String (i), i + 1);
    addAndMakeVisible (outputChannelBox);

    // --- Output format ---
    setupLabel (modeLabel, "Output format");
    valueModeBox.addItem ("7-bit auto-range", 1);
    valueModeBox.addItem ("7-bit fixed scale", 2);
    valueModeBox.addItem ("14-bit (MSB+LSB)", 3);
    addAndMakeVisible (valueModeBox);

    // --- Max for "fixed scale" ---
    setupLabel (maxLabel, "Max (fixed scale)");
    inputMaxSlider.setSliderStyle (juce::Slider::IncDecButtons);
    inputMaxSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 150, 28);
    inputMaxSlider.setRange (1.0, 16383.0, 1.0);
    addAndMakeVisible (inputMaxSlider);

    // --- Pass-through and repeated-CC filter ---
    addAndMakeVisible (passthroughButton);
    addAndMakeVisible (dedupeButton);

    // --- Reset of the auto-range learning ---
    resetLearnButton.onClick = [this] { proc.requestResetLearning(); };
    addAndMakeVisible (resetLearnButton);

    // Parameter <-> control attachments
    inAtt     = std::make_unique<ComboAttach>  (proc.apvts, "inputChannel",  inputChannelBox);
    outAtt    = std::make_unique<ComboAttach>  (proc.apvts, "outputChannel", outputChannelBox);
    modeAtt   = std::make_unique<ComboAttach>  (proc.apvts, "valueMode",     valueModeBox);
    maxAtt    = std::make_unique<SliderAttach> (proc.apvts, "inputMax",      inputMaxSlider);
    passAtt   = std::make_unique<ButtonAttach> (proc.apvts, "passthrough",   passthroughButton);
    dedupeAtt = std::make_unique<ButtonAttach> (proc.apvts, "dedupe",        dedupeButton);

    infoLabel.setText ("Automatic mapping: NRPN number -> CC number.\n"
                       "Auto-range: learns each parameter's max and scales to 0-127.\n"
                       "Do one full sweep of each knob to calibrate it (it is remembered).",
                       juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::topLeft);
    infoLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
    infoLabel.setColour (juce::Label::textColourId, brand::textFaint);
    infoLabel.setBorderSize ({});
    addAndMakeVisible (infoLabel);

    // --- Monitor ---
    setupSectionTitle (monitorTitle, "MONITOR");

    monitorText.setJustificationType (juce::Justification::topLeft);
    monitorText.setFont (brand::monoFont (13.0f));
    monitorText.setColour (juce::Label::textColourId, brand::textMuted);
    monitorText.setBorderSize ({});
    monitorText.setText ("Waiting for NRPN messages...", juce::dontSendNotification);
    addAndMakeVisible (monitorText);

    addAndMakeVisible (ccMeter);

    // --- Footer link to the site ---
    siteLink.setFont (brand::monoFont (12.0f), false, juce::Justification::centredRight);
    siteLink.setTooltip ("https://www.gprmusic.com");
    addAndMakeVisible (siteLink);

    // Set last, once every child is attached: the change is passed down to all of
    // them, so the slider rebuilds its value box with the brand style.
    setLookAndFeel (&lnf);

    setSize (480, 594);
    startTimerHz (30);
}

NrpnToCcEditor::~NrpnToCcEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void NrpnToCcEditor::paint (juce::Graphics& g)
{
    g.fillAll (brand::bg);

    // Header band with a hairline under it.
    g.setColour (brand::surface);
    g.fillRect (0, 0, getWidth(), headerHeight);
    g.setColour (brand::line);
    g.fillRect (0, headerHeight - 1, getWidth(), 1);

    // Product name and version, right-aligned next to the lockup.
    auto product = productArea;
    g.setColour (brand::text);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (22.0f).withStyle ("Bold")));
    g.drawText ("NRPN to CC", product.removeFromTop (28), juce::Justification::bottomRight);
    g.setColour (brand::textFaint);
    g.setFont (brand::monoFont (12.0f));
    g.drawText (juce::String ("v") + JucePlugin_VersionString, product, juce::Justification::topRight);

    // Card behind the monitor text.
    const auto panel = monitorPanel.toFloat().reduced (0.5f);
    g.setColour (brand::card);
    g.fillRoundedRectangle (panel, 10.0f);
    g.setColour (brand::line);
    g.drawRoundedRectangle (panel, 10.0f, 1.0f);
}

void NrpnToCcEditor::resized()
{
    // --- Header: lockup on the left, product on the right ---
    const auto header = getLocalBounds().removeFromTop (headerHeight).reduced (20, 0);
    const auto logoBounds = header.withWidth (270).withSizeKeepingCentre (270, 64);
    if (logo != nullptr)
        logo->setTransformToFit (logoBounds.toFloat(),
                                 juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid);
    productArea = header.withTrimmedLeft (logoBounds.getWidth() + 12).withSizeKeepingCentre (
                      header.getWidth() - logoBounds.getWidth() - 12, 46);

    // --- Content ---
    auto area = getLocalBounds().withTrimmedTop (headerHeight).reduced (20, 16);

    auto row = [&area] (int h, int gapAfter)
    {
        auto r = area.removeFromTop (h);
        area.removeFromTop (gapAfter);
        return r;
    };

    const int labelW = 150;

    settingsTitle.setBounds (row (16, 8));

    for (auto [label, control] : { std::pair<juce::Component*, juce::Component*> { &inLabel,   &inputChannelBox },
                                   std::pair<juce::Component*, juce::Component*> { &outLabel,  &outputChannelBox },
                                   std::pair<juce::Component*, juce::Component*> { &modeLabel, &valueModeBox },
                                   std::pair<juce::Component*, juce::Component*> { &maxLabel,  &inputMaxSlider } })
    {
        auto r = row (28, 8);
        label->setBounds (r.removeFromLeft (labelW));
        control->setBounds (r);
    }

    passthroughButton.setBounds (row (24, 4));
    dedupeButton.setBounds (row (24, 6));
    resetLearnButton.setBounds (row (28, 10).removeFromLeft (150));

    infoLabel.setBounds (row (46, 10));

    monitorTitle.setBounds (row (16, 8));
    ccMeter.setBounds (row (22, 8));
    monitorPanel = row (66, 10);
    monitorText.setBounds (monitorPanel.reduced (12, 8));

    siteLink.setBounds (area.removeFromTop (18));
}

//==============================================================================
void NrpnToCcEditor::timerCallback()
{
    const uint32_t c = proc.monitor.counter.load();
    if (c != lastSeenCounter)
    {
        lastSeenCounter = c;
        updateMonitorText();
        ccMeter.setValue (proc.monitor.lastCcValue.load());
    }
}

void NrpnToCcEditor::updateMonitorText()
{
    const auto& m = proc.monitor;

    const int  param   = m.lastNrpnParam.load();
    const int  value   = m.lastNrpnValue.load();
    const int  inCh    = m.lastNrpnChannel.load();
    const bool is14    = m.lastNrpn14bit.load();
    const int  ccNum   = m.lastCcNumber.load();
    const int  ccVal   = m.lastCcValue.load();
    const int  outCh   = m.lastCcChannel.load();
    const int  ccLsbN  = m.lastCcLsbNumber.load();
    const int  ccLsbV  = m.lastCcLsbValue.load();
    const int  rangeMx = m.lastRangeMax.load();

    if (param < 0)
    {
        monitorText.setText ("Waiting for NRPN messages...", juce::dontSendNotification);
        return;
    }

    juce::String t;
    t << "IN   NRPN #" << param
      << "  val=" << value
      << (is14 ? "  (14-bit)" : "  (raw value)")
      << "  ch=" << inCh << "\n";

    t << "OUT  CC #" << ccNum << " = " << ccVal
      << (is14 ? "  (MSB)"
                : "  (range 0-" + juce::String (rangeMx) + ")")
      << "  ch=" << outCh;

    if (is14 && ccLsbN >= 0)
        t << "\n     CC #" << ccLsbN << " = " << ccLsbV << "  (LSB)  ch=" << outCh;

    monitorText.setText (t, juce::dontSendNotification);
}
