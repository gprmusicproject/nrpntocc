#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
NrpnToCcProcessor::NrpnToCcProcessor()
    : AudioProcessor (BusesProperties()
          // Strumento VST3: serve un'uscita audio (resta in silenzio).
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createLayout())
{
    learnedMax.assign ((size_t) 16 * kNumParams, 0);
}

//==============================================================================
bool NrpnToCcProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Nessun ingresso audio; uscita mono o stereo (audio in silenzio).
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout NrpnToCcProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    // Canale di ingresso: indice 0 = Omni (tutti), 1..16 = canale specifico.
    StringArray inChoices;
    inChoices.add ("Omni (tutti)");
    for (int i = 1; i <= 16; ++i) inChoices.add (String (i));

    // Canale di uscita: indice 0 = "Come ingresso", 1..16 = canale specifico.
    StringArray outChoices;
    outChoices.add ("Come ingresso");
    for (int i = 1; i <= 16; ++i) outChoices.add (String (i));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "inputChannel", 1 }, "Canale ingresso", inChoices, 0));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "outputChannel", 1 }, "Canale uscita", outChoices, 0));

    // 0 = 7 bit auto-range: impara il max per ogni parametro e scala a 0-127.
    // 1 = 7 bit scala fissa: scala usando "inputMax".
    // 2 = 14 bit: emette MSB su CC#n e LSB su CC#n+32.
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "valueMode", 1 }, "Formato uscita",
        StringArray { "7 bit auto-range", "7 bit scala fissa", "14 bit (MSB+LSB)" }, 0));

    // Massimo usato SOLO in "scala fissa" per portare il valore a 0-127.
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "inputMax", 1 }, "Max (scala fissa)", 1, 16383, 255));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "passthrough", 1 }, "Passthrough NRPN originali", false));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "dedupe", 1 }, "Filtra CC ripetuti", true));

    return layout;
}

//==============================================================================
void NrpnToCcProcessor::prepareToPlay (double, int)
{
    for (auto& s : nrpnState)
        s = ChannelNrpnState {};

    for (auto& ch : lastSentCc)
        for (auto& v : ch)
            v = -1;
}

//==============================================================================
void NrpnToCcProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    buffer.clear();

    if (resetLearnRequested.exchange (false))
        std::fill (learnedMax.begin(), learnedMax.end(), 0);

    const int  inChParam  = (int) *apvts.getRawParameterValue ("inputChannel");   // 0 = Omni
    const int  outChParam = (int) *apvts.getRawParameterValue ("outputChannel");  // 0 = come ingresso
    const int  valueMode  = (int) *apvts.getRawParameterValue ("valueMode"); // 0 = auto, 1 = fissa, 2 = 14 bit
    const int  inputMax   = juce::jmax (1, (int) *apvts.getRawParameterValue ("inputMax"));
    const bool passthrough = *apvts.getRawParameterValue ("passthrough") > 0.5f;
    const bool dedupe      = *apvts.getRawParameterValue ("dedupe") > 0.5f;

    juce::MidiBuffer output;

    for (const auto metadata : midi)
    {
        const auto msg       = metadata.getMessage();
        const int  samplePos = metadata.samplePosition;
        const int  ch        = msg.getChannel();   // 1..16, oppure 0 se non e' un messaggio di canale

        const bool channelMatches = (inChParam == 0) || (ch == inChParam);

        bool consumed = false;

        if (msg.isController() && channelMatches && ch >= 1 && ch <= 16)
        {
            const int cc  = msg.getControllerNumber();
            const int val = msg.getControllerValue();
            auto& st = nrpnState[ch - 1];

            const int paramNum = (st.paramMSB << 7) | st.paramLSB;
            const int targetCc = paramNum & 0x7F;              // mappatura automatica NRPN->CC
            const int outCh    = (outChParam == 0) ? ch : outChParam;

            switch (cc)
            {
                case 99:  // NRPN Parameter MSB
                    st.paramMSB = val;
                    consumed = true;
                    break;

                case 98:  // NRPN Parameter LSB
                    st.paramLSB = val;
                    consumed = true;
                    break;

                case 6:   // Data Entry MSB
                {
                    st.dataMSB = val;

                    if (valueMode == 2)         // 14 bit -> emette subito la parte MSB (CC#n)
                    {
                        output.addEvent (juce::MidiMessage::controllerEvent (outCh, targetCc, val), samplePos);
                        monitor.lastNrpnParam   = paramNum;
                        monitor.lastNrpnChannel = ch;
                        monitor.lastNrpn14bit   = true;
                        monitor.lastCcNumber    = targetCc;
                        monitor.lastCcValue     = val;
                        monitor.lastCcChannel   = outCh;
                        monitor.lastRangeMax    = -1;
                        monitor.counter.fetch_add (1);
                    }
                    // Modi 7 bit (0 auto, 1 fissa): non emette qui, aspetta il CC 38
                    // per avere il valore completo a 14 bit.
                    consumed = true;
                    break;
                }

                case 38:  // Data Entry LSB
                {
                    st.dataLSB = val;
                    const int value14 = (st.dataMSB << 7) | st.dataLSB;   // valore reale a 14 bit

                    if (valueMode == 0 || valueMode == 1)   // 7 bit: scala il valore su 0-127
                    {
                        int rangeMax;
                        if (valueMode == 0)     // auto-range: impara il max per (canale, parametro)
                        {
                            int& learned = learnedMax[(size_t) (ch - 1) * kNumParams + paramNum];
                            learned  = juce::jmax (learned, value14);
                            rangeMax = juce::jmax (1, learned);
                        }
                        else                    // scala fissa
                        {
                            rangeMax = inputMax;
                        }

                        const int value7 = juce::jlimit (0, 127,
                            (int) std::lround (value14 * 127.0 / (double) rangeMax));

                        // Filtro anti-ripetizione: salta se il CC avrebbe lo stesso valore.
                        const bool isDuplicate = dedupe && (lastSentCc[outCh - 1][targetCc] == value7);

                        if (! isDuplicate)
                        {
                            output.addEvent (juce::MidiMessage::controllerEvent (outCh, targetCc, value7), samplePos);
                            lastSentCc[outCh - 1][targetCc] = value7;

                            monitor.lastNrpnParam   = paramNum;
                            monitor.lastNrpnChannel = ch;
                            monitor.lastNrpn14bit   = false;
                            monitor.lastNrpnValue   = value14;     // valore grezzo
                            monitor.lastCcNumber    = targetCc;
                            monitor.lastCcValue     = value7;      // valore scalato inviato
                            monitor.lastCcChannel   = outCh;
                            monitor.lastCcLsbNumber = -1;
                            monitor.lastCcLsbValue  = -1;
                            monitor.lastRangeMax    = rangeMax;    // range usato (per il Monitor)
                            monitor.counter.fetch_add (1);
                        }
                    }
                    else                        // 14 bit -> emette la parte LSB (CC#n+32)
                    {
                        const int lsbCc = (targetCc + 32) & 0x7F;   // convenzione CC 14-bit
                        output.addEvent (juce::MidiMessage::controllerEvent (outCh, lsbCc, val), samplePos);
                        monitor.lastNrpnValue   = value14;
                        monitor.lastCcLsbNumber = lsbCc;
                        monitor.lastCcLsbValue  = val;
                        monitor.counter.fetch_add (1);
                    }
                    consumed = true;
                    break;
                }

                default:
                    break;
            }
        }

        // I messaggi NRPN vengono assorbiti (a meno del passthrough); tutto il resto passa.
        if (! consumed || passthrough)
            output.addEvent (msg, samplePos);
    }

    midi.swapWith (output);
}

//==============================================================================
juce::AudioProcessorEditor* NrpnToCcProcessor::createEditor()
{
    return new NrpnToCcEditor (*this);
}

//==============================================================================
void NrpnToCcProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("NrpnToCcState");

    if (auto params = apvts.copyState().createXml())
        root.addChildElement (params.release());   // tag "PARAMETERS"

    // Range appresi in auto-range: salvo solo le voci osservate (>0).
    auto* learned = root.createNewChildElement ("Learned");
    for (int c = 0; c < 16; ++c)
        for (int p = 0; p < kNumParams; ++p)
        {
            const int v = learnedMax[(size_t) c * kNumParams + p];
            if (v > 0)
            {
                auto* e = learned->createNewChildElement ("m");
                e->setAttribute ("c", c);
                e->setAttribute ("p", p);
                e->setAttribute ("v", v);
            }
        }

    copyXmlToBinary (root, destData);
}

void NrpnToCcProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    if (xml->hasTagName ("NrpnToCcState"))
    {
        if (auto* params = xml->getChildByName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*params));

        std::fill (learnedMax.begin(), learnedMax.end(), 0);
        if (auto* learned = xml->getChildByName ("Learned"))
            for (auto* e : learned->getChildIterator())
            {
                const int c = e->getIntAttribute ("c", -1);
                const int p = e->getIntAttribute ("p", -1);
                const int v = e->getIntAttribute ("v", 0);
                if (c >= 0 && c < 16 && p >= 0 && p < kNumParams)
                    learnedMax[(size_t) c * kNumParams + p] = v;
            }
    }
    else if (xml->hasTagName (apvts.state.getType()))   // compatibilita' vecchio formato
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================
// Entry point richiesto da JUCE per creare l'istanza del plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NrpnToCcProcessor();
}
