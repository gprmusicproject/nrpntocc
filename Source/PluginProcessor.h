#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

//==============================================================================
// Processore: riceve NRPN in ingresso e genera i corrispondenti CC in uscita.
// Il plugin e' un "MIDI effect" da inserire in una traccia (es. Cubase).
//==============================================================================
class NrpnToCcProcessor : public juce::AudioProcessor
{
public:
    NrpnToCcProcessor();
    ~NrpnToCcProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true; }

    //==============================================================================
    const juce::String getName() const override            { return "NRPN to CC"; }

    bool acceptsMidi() const override                      { return true; }
    bool producesMidi() const override                     { return true; }
    bool isMidiEffect() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0.0; }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override       { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Dati per il monitor a schermo (aggiornati nel processBlock, letti dall'editor).
    struct MonitorData
    {
        std::atomic<int>      lastNrpnParam   { -1 };
        std::atomic<int>      lastNrpnValue   { -1 };   // 7 o 14 bit a seconda della modalita'
        std::atomic<int>      lastNrpnChannel { -1 };
        std::atomic<bool>     lastNrpn14bit   { false };
        std::atomic<int>      lastCcNumber    { -1 };   // CC MSB (o CC 7-bit)
        std::atomic<int>      lastCcValue     { -1 };
        std::atomic<int>      lastCcChannel   { -1 };
        std::atomic<int>      lastCcLsbNumber { -1 };   // CC LSB (solo 14 bit)
        std::atomic<int>      lastCcLsbValue  { -1 };
        std::atomic<int>      lastRangeMax    { -1 };   // max usato per la scalatura (auto o fisso)
        std::atomic<uint32_t> counter         { 0 };    // cambia a ogni nuovo evento
    };
    MonitorData monitor;

    // Azzera i range appresi in auto-range (chiamabile dall'editor).
    void requestResetLearning() { resetLearnRequested = true; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Stato NRPN corrente per ciascun canale MIDI (1..16).
    struct ChannelNrpnState
    {
        int paramMSB = 0;   // CC 99
        int paramLSB = 0;   // CC 98
        int dataMSB  = 0;   // CC 6
        int dataLSB  = 0;   // CC 38
    };
    ChannelNrpnState nrpnState[16];

    // Ultimo valore CC inviato per canale (1..16) e numero CC (0..127); -1 = nessuno.
    // Usato dal filtro anti-ripetizione in modalita' 7 bit.
    int lastSentCc[16][128];

    // Auto-range: massimo valore (14 bit) osservato per canale (0..15) e parametro NRPN
    // (0..16383). Dimensione 16 * 16384. 0 = mai osservato.
    static constexpr int kNumParams = 16384;
    std::vector<int> learnedMax;
    std::atomic<bool> resetLearnRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NrpnToCcProcessor)
};
