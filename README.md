# NRPN to CC — VST3 (strumento MIDI per Cubase)

Plugin VST3 (solo VST3) di tipo **strumento (VSTi)** che riceve MIDI, **lascia passare**
tutto ciò che non è NRPN e **converte gli NRPN in CC**, restituendo il risultato sulla
sua uscita MIDI.

> **Perché uno strumento e non un "MIDI effect"?** Cubase **non** carica i VST3 MIDI-effect
> negli slot MIDI Insert (accetta lì solo i suoi plug-in MIDI nativi), e come insert audio
> non riceverebbe il MIDI della traccia. A uno **strumento**, invece, Cubase invia tutto il
> MIDI della traccia: così il plugin riceve gli NRPN e può emettere i CC in uscita, che
> instradi verso un virtual instrument o una traccia MIDI esterna. In più la porta dell'OB-6
> la apre Cubase, non il plugin: nessun conflitto sulla porta.

## Cosa fa

- Riceve NRPN (CC 99/98 = numero parametro, CC 6/38 = valore).
- Genera un CC con **mappatura automatica**: `numero CC = numero parametro NRPN` (7 bit bassi).
- Dall'interfaccia puoi impostare:
  - **Canale ingresso** (Omni oppure 1–16): filtra su quale canale ascoltare.
  - **Canale uscita** (Come ingresso oppure 1–16).
  - **Formato uscita**:
    - `7 bit auto-range` (default) → un solo CC. Il plugin **impara** per ogni NRPN (per canale)
      il valore massimo che riceve e scala su **0–127**. Generico, senza tabelle specifiche di
      dispositivo: gli **interruttori** (max 1) diventano 0/127, le **manopole** diventano 0–127
      fluide. Fai **una passata completa** di ogni manopola per calibrarla; i range appresi sono
      **salvati nel progetto**. Il pulsante **Reset apprendimento** li azzera.
    - `7 bit scala fissa` → un solo CC scalato con un massimo fisso (**Max (scala fissa)**), utile
      quando conosci il range e vuoi un comportamento deterministico.
    - `14 bit (MSB+LSB)` → due CC: `CC#n` = MSB e `CC#n+32` = LSB (alta risoluzione).
  - **Max (scala fissa)**: usato solo dalla modalità "scala fissa".
  - **Passthrough NRPN originali**: se attivo, oltre al CC lascia passare anche gli NRPN.
  - **Filtra CC ripetuti** (default ON): in modalità 7 bit evita di inviare CC con lo stesso
    valore consecutivo (utile perché scalando 0–255 → 0–127 molti step darebbero lo stesso CC).
  - **Monitor**: mostra l'ultimo NRPN ricevuto, il CC inviato e una **barra** del valore CC in uscita.

> Nota sulle **porte MIDI**: un VST3 inserito in una traccia non apre porte hardware.
> La porta fisica (interfaccia MIDI) si sceglie nel **routing della traccia della DAW**.
> Nel plugin scegli il **canale**; la porta la gestisce Cubase.

---

## Prerequisiti (Windows)

Sul sistema servono un compilatore C++ e CMake (attualmente non installati):

1. **Visual Studio 2022 Community** (gratuito) — durante l'installazione seleziona il workload
   **"Sviluppo di applicazioni desktop con C++"**. Include il compilatore MSVC, il Windows SDK
   e i "C++ CMake tools" (quindi CMake).
   Download: https://visualstudio.microsoft.com/it/downloads/
2. **Git** — già installato.

(In alternativa a VS puoi installare CMake standalone da https://cmake.org/download/ e i
Build Tools for Visual Studio, ma l'IDE completo è la strada più semplice.)

---

## Compilazione

Apri **"Developer PowerShell for VS 2022"** (dal menu Start) e posizionati nella cartella del
progetto:

```
cd "D:\AI projects\NrpnToCc"
```

Configura (la prima volta scarica automaticamente JUCE: qualche minuto):

```
cmake -B build -G "Visual Studio 17 2022" -A x64
```

Compila in Release:

```
cmake --build build --config Release
```

Il plugin compilato si trova in:

```
build\NrpnToCc_artefacts\Release\VST3\NRPN to CC.vst3
```

---

## Installazione

Il build **non** copia automaticamente il plugin. Al termine trovi il bundle in
`build\NrpnToCc_artefacts\Release\VST3\NRPN to CC.vst3`: copia a mano quella cartella
in una di queste destinazioni:

- `C:\Program Files\Common Files\VST3`  (per tutti gli utenti)
- `%LOCALAPPDATA%\Programs\Common\VST3`  (solo per il tuo utente)

---

## Uso in Cubase (routing consigliato: rack strumenti)

Obiettivo: **OB-6 → plugin → (passthrough + CC) → virtual instrument / strumento esterno.**

1. Riscansiona i VST3 (Studio → VST Plug-in Manager → Aggiorna). Il plugin appare tra gli
   **strumenti** (Instrument), non tra gli insert.
2. Apri il **rack strumenti**: `Studio → VST Instruments` (tasto **F11**).
   Aggiungi **NRPN to CC**. Quando Cubase chiede di creare una traccia MIDI associata, accetta.
3. **Traccia "IN"** (quella creata al punto 2, che punta al plugin):
   - Input MIDI = **OB-6** (o "All MIDI Inputs").
   - Attiva il **Monitor** (icona altoparlante) o il record-enable, così il MIDI raggiunge il plugin.
   - Apri la GUI del plugin: inviando NRPN dall'OB-6, il **Monitor** deve accendersi.
4. **Abilita l'uscita MIDI del plugin**: nel rack strumenti, sullo slot di NRPN to CC, attiva
   l'uscita MIDI del plug-in (in molte versioni c'è un piccolo controllo "MIDI Out"/connettore
   sullo slot; la posizione varia con la versione di Cubase).
5. **Traccia "OUT"** (nuova traccia MIDI):
   - Input MIDI = **NRPN to CC** (l'uscita MIDI del plugin ora compare nell'elenco degli input).
   - Output MIDI = la destinazione desiderata: un **virtual instrument** (altra traccia strumento)
     oppure una **porta MIDI esterna** (synth hardware).
   - Attiva il **Monitor** su questa traccia così il flusso convertito passa alla destinazione.

Flusso finale: `OB-6 → traccia IN → NRPN to CC → traccia OUT → strumento/hardware`.

**Cubase 15 Pro** — note specifiche:
- Usa il **rack** (F11), non una traccia strumento semplice: solo l'istanza nel rack espone in
  modo affidabile l'uscita MIDI del plugin come *input* selezionabile su un'altra traccia.
- In Cubase 15 l'uscita MIDI dello strumento compare **automaticamente** tra gli input MIDI: non
  serve alcun interruttore globale.
- Se "NRPN to CC" non compare tra gli input al punto 5: chiudi/riapri la finestra di routing o
  ricarica il progetto dopo aver aggiunto lo strumento nel rack.

### Comportamento del MIDI

- **Note, pitch bend, CC normali, ecc.** → passano invariati in uscita.
- **NRPN** (CC 99/98/6/38) → convertiti in **CC** (mappatura automatica: numero NRPN → numero CC).
- Gli NRPN grezzi vengono **assorbiti** (non inoltrati), a meno che tu non attivi
  *Passthrough NRPN originali* nella GUI.

---

## Struttura del progetto

```
NrpnToCc/
├─ CMakeLists.txt          # build VST3-only, scarica JUCE via FetchContent
├─ LICENSE                 # GNU GPL v3
└─ Source/
   ├─ PluginProcessor.h/.cpp   # logica NRPN -> CC
   └─ PluginEditor.h/.cpp      # interfaccia (canali, modalità, monitor)
```

---

## Licenza

Questo progetto è distribuito sotto **GNU General Public License v3.0** (vedi [LICENSE](LICENSE)).

Il plugin è costruito con [JUCE](https://juce.com), usato in conformità alla sua licenza
**GPLv3**. Di conseguenza anche questo progetto è rilasciato come software libero GPLv3.
JUCE **non** è incluso nel repository: viene scaricato automaticamente da CMake
(FetchContent) al momento della compilazione.

© GPR Music Project.
