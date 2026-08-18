#include "PluginEditor.h"

//==============================================================================
NrpnToCcEditor::NrpnToCcEditor (NrpnToCcProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };

    // Titolo
    titleLabel.setText ("NRPN to CC", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (22.0f).withStyle ("Bold")));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

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
    inputMaxSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 90, 22);
    inputMaxSlider.setRange (1.0, 16383.0, 1.0);
    addAndMakeVisible (inputMaxSlider);

    // --- Passthrough e filtro ripetizioni ---
    addAndMakeVisible (passthroughButton);
    addAndMakeVisible (dedupeButton);

    // --- Reset dell'apprendimento auto-range ---
    resetLearnButton.onClick = [this] { proc.requestResetLearning(); };
    addAndMakeVisible (resetLearnButton);

    // Collegamento parametri <-> controlli
    inAtt     = std::make_unique<ComboAttach>  (proc.apvts, "inputChannel",  inputChannelBox);
    outAtt    = std::make_unique<ComboAttach>  (proc.apvts, "outputChannel", outputChannelBox);
    modeAtt   = std::make_unique<ComboAttach>  (proc.apvts, "valueMode",     valueModeBox);
    maxAtt    = std::make_unique<SliderAttach> (proc.apvts, "inputMax",      inputMaxSlider);
    passAtt   = std::make_unique<ButtonAttach> (proc.apvts, "passthrough",   passthroughButton);
    dedupeAtt = std::make_unique<ButtonAttach> (proc.apvts, "dedupe",        dedupeButton);

    // Nota informativa sul routing delle porte
    infoLabel.setText ("Automatic mapping: NRPN number -> CC number.\n"
                       "Auto-range: learns each parameter's max and scales to 0-127.\n"
                       "Do one full sweep of each knob to calibrate it (it is remembered).",
                       juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::topLeft);
    infoLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
    infoLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (infoLabel);

    // Monitor
    monitorTitle.setText ("Monitor", juce::dontSendNotification);
    monitorTitle.setFont (juce::Font (juce::FontOptions{}.withHeight (14.0f).withStyle ("Bold")));
    addAndMakeVisible (monitorTitle);

    monitorText.setJustificationType (juce::Justification::topLeft);
    monitorText.setFont (juce::Font (juce::FontOptions{}.withName (juce::Font::getDefaultMonospacedFontName()).withHeight (13.0f)));
    monitorText.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (0.25f));
    monitorText.setText ("Waiting for NRPN messages...", juce::dontSendNotification);
    addAndMakeVisible (monitorText);

    addAndMakeVisible (ccMeter);

    setSize (440, 486);
    startTimerHz (30);
}

NrpnToCcEditor::~NrpnToCcEditor()
{
    stopTimer();
}

//==============================================================================
void NrpnToCcEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void NrpnToCcEditor::resized()
{
    auto area = getLocalBounds().reduced (14);

    titleLabel.setBounds (area.removeFromTop (34));
    area.removeFromTop (6);

    auto row = [&area] (int h) { auto r = area.removeFromTop (h); area.removeFromTop (6); return r; };

    const int labelW = 150;

    {
        auto r = row (26);
        inLabel.setBounds (r.removeFromLeft (labelW));
        inputChannelBox.setBounds (r);
    }
    {
        auto r = row (26);
        outLabel.setBounds (r.removeFromLeft (labelW));
        outputChannelBox.setBounds (r);
    }
    {
        auto r = row (26);
        modeLabel.setBounds (r.removeFromLeft (labelW));
        valueModeBox.setBounds (r);
    }
    {
        auto r = row (26);
        maxLabel.setBounds (r.removeFromLeft (labelW));
        inputMaxSlider.setBounds (r);
    }

    passthroughButton.setBounds (row (26));
    dedupeButton.setBounds (row (26));
    resetLearnButton.setBounds (row (26).removeFromLeft (170));

    infoLabel.setBounds (row (40));

    monitorTitle.setBounds (row (20));
    ccMeter.setBounds (row (24));
    monitorText.setBounds (area);   // resto dello spazio
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
