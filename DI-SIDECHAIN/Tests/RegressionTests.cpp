#include "PluginEditor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

static void set(DISidechainProcessor& p, const char* id, float value)
{
    auto* parameter = p.state.getParameter(id);
    require(parameter != nullptr, "Missing parameter");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

static void editorLifecycle(DISidechainProcessor& p)
{
    for (int iteration = 0; iteration < 10; ++iteration)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        require(editor != nullptr, "Editor creation failed");
        require(editor->getWidth() == 700 && editor->getHeight() == 760, "Initial editor size");
        require(editor->getNumChildComponents() == 20, "Missing editor controls");
        int sliders = 0, combos = 0, buttons = 0;
        for (auto* child : editor->getChildren())
        {
            require(!child->getBounds().isEmpty(), "Control was not laid out during construction");
            sliders += dynamic_cast<juce::Slider*>(child) != nullptr;
            combos += dynamic_cast<juce::ComboBox*>(child) != nullptr;
            buttons += dynamic_cast<juce::ToggleButton*>(child) != nullptr;
        }
        require(sliders == 8 && combos == 3 && buttons == 1, "Control types changed");
        editor->setSize(900, 600);
        editor->setSize(700, 760);
        require(editor->createComponentSnapshot(editor->getLocalBounds()).isValid(), "Editor paint failed");
        // Exercise attachments from a host automation thread while the editor exists.
        std::thread automation([&] {
            for (auto* parameter : p.getParameters())
                parameter->setValueNotifyingHost(0.5f);
        });
        automation.join();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
        editor.reset();
        // Pending parameter/timer updates must not access the deleted editor.
        set(p, "depth", 1.0f);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    }
}

static void envelopeInterpolationRegression()
{
    DISidechainProcessor processor;
    const float keys[8] = {1.f, .9f, .1f, 0.f, .4f, .75f, .95f, 1.f};
    for (int i=0; i<8; ++i)
    {
        processor.points[i] = keys[i];
        auto exact = processor.evaluateEnvelope((double)i/7.0);
        require(std::abs(exact - keys[i]) < 0.0001f,
                "Envelope evaluator does not match an edited node");
    }
    for (int i=0; i<=256; ++i)
    {
        float gain = processor.evaluateEnvelope(i/256.0);
        require(std::isfinite(gain) && gain >= 0.f && gain <= 1.f,
                "Envelope evaluator returned invalid gain");
    }
    require(std::abs(processor.evaluateEnvelope(-1.0) - keys[0]) < 0.0001f,
            "Envelope negative phase is not clamped");
    require(std::abs(processor.evaluateEnvelope(3.0) - keys[7]) < 0.0001f,
            "Envelope phase greater than one is not clamped");
}

static void editableEnvelopeRoundtrip()
{
    DISidechainProcessor p;
    require(p.getEnvelopeNodeCount()==8, "Legacy eight-node default missing");
    const float preserved=p.evaluateEnvelope(0.5);
    const int inserted=p.insertEnvelopeNode(.46f,.12f);
    require(inserted>0 && inserted<8, "Envelope point insertion failed");
    require(p.getEnvelopeNodeCount()==9, "Envelope point count not updated");
    p.setEnvelopeNode(inserted,.48f,.22f);
    // Per-segment cubic controls must change the true DSP evaluation,
    // remain bounded, and survive serialized host state.
    p.setEnvelopeSegmentCurve(inserted, .7f);
    require(std::abs(p.getEnvelopeSegmentCurve(inserted)-.7f)<.0001f,
            "Cubic segment curvature was not applied");
    const auto a=p.getEnvelopeNode(inserted);
    const auto b=p.getEnvelopeNode(inserted+1);
    const double halfway=((double)a.x+b.x)*.5;
    const float bent=p.evaluateEnvelope(halfway);
    p.setEnvelopeSegmentCurve(inserted, 0.f);
    const float neutral=p.evaluateEnvelope(halfway);
    require(std::abs(bent-neutral)>0.001f,
            "Cubic curvature did not change the processed envelope");
    p.setEnvelopeSegmentCurve(inserted, .7f);
    for(int i=0;i<=100;++i){
        const float v=p.evaluateEnvelope((double)i/100.0);
        require(std::isfinite(v)&&v>=0.f&&v<=1.f,"Cubic envelope overshoot");
    }
    const auto edited=p.getEnvelopeNode(inserted);
    require(std::abs(edited.x-.48f)<.0001f &&
            std::abs(edited.y-.22f)<.0001f, "Time and gain edits not applied");
    require(p.evaluateEnvelope(.48) >= .219f &&
            p.evaluateEnvelope(.48) <= .221f, "Envelope must pass exactly through node");
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(),(int)saved.getSize());
    require(restored.getEnvelopeNodeCount()==9, "New envelope node count not restored");
    require(std::abs(restored.getEnvelopeSegmentCurve(inserted)-.7f)<.0001f,
            "Cubic envelope curvature lost during state restoration");
    const auto after=restored.getEnvelopeNode(inserted);
    require(std::abs(after.x-.48f)<.0001f &&
            std::abs(after.y-.22f)<.0001f, "Envelope node state not restored");
    require(restored.removeEnvelopeNode(inserted), "Envelope node removal failed");
    require(restored.getEnvelopeNodeCount()==8, "Envelope point removal count wrong");
    require(!restored.removeEnvelopeNode(0), "Endpoint should be protected");
    require(!restored.removeEnvelopeNode(7), "Last endpoint should be protected");
    for(int i=0;i<=256;++i)
    {
        const float v=restored.evaluateEnvelope(i/256.);
        require(std::isfinite(v) && v>=0.f && v<=1.f, "Variable envelope out of range");
    }
    // Existing sessions without envelopeCount must still use the legacy path.
    DISidechainProcessor legacy;
    require(legacy.getEnvelopeNodeCount()==8, "Legacy fallback count changed");
    require(std::abs(legacy.evaluateEnvelope(.5)-preserved)<.0001f, "Legacy curve changed");
    juce::MemoryBlock legacySaved;
    legacy.getStateInformation(legacySaved);
    DISidechainProcessor loadedLegacy;
    loadedLegacy.setStateInformation(legacySaved.getData(),(int)legacySaved.getSize());
    require(std::abs(loadedLegacy.evaluateEnvelope(.5)-preserved)<.0001f, "Legacy state migration changed envelope");
}

static void concurrentEnvelopeEditingStress()
{
    DISidechainProcessor processor;
    processor.prepareToPlay(48000., 128);
    set(processor, "trigger", 2); // MIDI only

    std::atomic<bool> editsDone {false};
    std::thread editorThread([&] {
        for (int pass=0;pass<300;++pass)
        {
            const float position=.15f+((pass%65)/100.f);
            const int inserted=processor.insertEnvelopeNode(position,.2f+(pass%6)*.1f);
            if(inserted>0)
            {
                processor.setEnvelopeSegmentCurve(inserted,((pass%11)-5)/5.f);
                processor.setEnvelopeNode(inserted,position,.15f+(pass%8)*.1f);
                processor.removeEnvelopeNode(inserted);
            }
            // Modifying the curve while audio is processing must remain
            // bounded, finite, and free of data races / invalid indices.
            processor.setEnvelopeNode(1,.1f,.15f+(pass%7)*.1f);
        }
        editsDone.store(true,std::memory_order_release);
    });

    bool valid=true;
    for (int pass=0;pass<400;++pass)
    {
        juce::AudioBuffer<float> buffer(2,128);
        buffer.clear();
        for (int ch=0;ch<2;++ch)
            for (int sample=0;sample<128;++sample)
                buffer.setSample(ch,sample,.3f);
        juce::MidiBuffer midi;
        if(pass%4==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),0);
        processor.processBlock(buffer,midi);
        for(int sample=0;sample<128;++sample)
        {
            const float result=buffer.getSample(0,sample);
            if(!std::isfinite(result)||std::abs(result)>1.f)valid=false;
        }
        const float envelope=processor.evaluateEnvelope((pass%101)/100.);
        if(!std::isfinite(envelope)||envelope<0.f||envelope>1.f)valid=false;
    }
    editorThread.join();
    require(editsDone.load(std::memory_order_acquire),
            "Concurrent envelope editor did not complete");
    require(valid,"Concurrent envelope editing produced invalid DSP output");
    processor.releaseResources();
}

static void liveSpectrumInputCapture()
{
    DISidechainProcessor processor;
    processor.prepareToPlay(48000.,256);
    set(processor,"trigger",2); // MIDI only — no external trigger needed.
    juce::AudioBuffer<float> input(2,256);
    juce::MidiBuffer midi;
    for(int block=0;block<16;++block)
    {
        for(int i=0;i<256;++i)
        {
            const float sample=.4f*std::sin(juce::MathConstants<float>::twoPi*
                                           440.f*(block*256+i)/48000.f);
            input.setSample(0,i,sample);
            input.setSample(1,i,sample);
        }
        midi.clear();
        processor.processBlock(input,midi);
    }
    require(processor.analyzerWritten.load()==4096,
            "Audio spectrum sample history was not published");
    float maxAbs=0.f;
    for(int i=0;i<DISidechainProcessor::analyzerSize;++i)
    {
        const float v=processor.analyzerHistory[i].load();
        require(std::isfinite(v),"Audio spectrum history contains nonfinite samples");
        maxAbs=juce::jmax(maxAbs,std::abs(v));
    }
    require(maxAbs>.3f,"Audio spectrum history did not capture the input signal");
    processor.releaseResources();
}

static float measureFrequencyMode(int mode,float hz,float depth)
{
    DISidechainProcessor processor;
    processor.prepareToPlay(48000.,256);
    set(processor,"trigger",2);
    set(processor,"mode",(float)mode);
    set(processor,"depth",depth);
    set(processor,"mix",1.f);
    set(processor,"sync",0.f);
    set(processor,"length",2000.f);
    set(processor,"smooth",.1f);
    set(processor,"low",120.f);
    set(processor,"high",1500.f);
    for(auto& node:processor.points)node.store(0.f);
    juce::AudioBuffer<float> buffer(2,256);
    juce::MidiBuffer midi;
    double energy=0.;
    int samples=0;
    for(int block=0;block<24;++block)
    {
        for(int i=0;i<256;++i)
        {
            const float v=.3f*std::sin(juce::MathConstants<float>::twoPi*
                                hz*(block*256+i)/48000.f);
            buffer.setSample(0,i,v);buffer.setSample(1,i,v);
        }
        midi.clear();
        if(block==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)110),0);
        processor.processBlock(buffer,midi);
        // Ignore the first 12 buffers to measure steady-state crossover.
        if(block>=12)for(int i=0;i<256;++i)
        {
            const float v=buffer.getSample(0,i);
            require(std::isfinite(v),"Frequency filter produced invalid sample");
            energy+=(double)v*v;++samples;
        }
    }
    return (float)std::sqrt(energy/juce::jmax(1,samples));
}

static void frequencyTargetRegression()
{
    // The selected filter must never color dry audio when depth is zero.
    for(int mode=0;mode<4;++mode)
        for(float freq:{65.f,430.f,6400.f})
        {
            const float dry=measureFrequencyMode(mode,freq,0.f);
            const float baseline=measureFrequencyMode(0,freq,0.f);
            require(std::abs(dry-baseline)<.0001f,
                    "Frequency target altered bypassed depth-zero signal");
        }

    const float lowDry=measureFrequencyMode(0,65.f,0.f);
    const float highDry=measureFrequencyMode(0,6400.f,0.f);
    const float midDry=measureFrequencyMode(0,430.f,0.f);
    const float lowModeBass=measureFrequencyMode(1,65.f,1.f);
    const float lowModeTreble=measureFrequencyMode(1,6400.f,1.f);
    require(lowModeBass<lowDry*.83f,"LOW mode did not reduce low-frequency audio");
    require(lowModeTreble>highDry*.82f,"LOW mode incorrectly reduced treble");

    const float highModeBass=measureFrequencyMode(2,65.f,1.f);
    const float highModeTreble=measureFrequencyMode(2,6400.f,1.f);
    require(highModeBass>lowDry*.82f,"HIGH mode incorrectly reduced bass");
    require(highModeTreble<highDry*.83f,"HIGH mode did not reduce treble");

    const float bandModeMid=measureFrequencyMode(3,430.f,1.f);
    const float bandModeTreble=measureFrequencyMode(3,6400.f,1.f);
    require(bandModeMid<midDry*.9f,"BAND mode failed to reduce target frequencies");
    require(bandModeTreble>highDry*.82f,"BAND mode incorrectly reduced treble");
}

static void audioSidechainTrigger()
{
    DISidechainProcessor p;
    set(p, "trigger", 1); // Audio only
    set(p, "mode", 0);    // Full spectrum
    set(p, "sync", 0);
    set(p, "length", 120);
    set(p, "smooth", 0.1f);
    set(p, "threshold", -25.0f);

    auto layout = p.getBusesLayout();
    layout.inputBuses.set(1, juce::AudioChannelSet::mono());
    require(p.setBusesLayout(layout), "Mono external sidechain layout rejected");
    p.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> buffer(3, 256);
    buffer.clear();
    for (int i = 0; i < 256; ++i)
    {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, 0.5f);
        if (i >= 32 && i < 48)
            buffer.setSample(2, i, 0.8f);
    }
    juce::MidiBuffer midi;
    const auto countBefore = p.triggerSequence.load();
    p.processBlock(buffer, midi);
    require(p.triggerSequence.load() > countBefore,
            "External audio transient was not detected");
    require(std::abs(buffer.getSample(0, 0) - 0.5f) < 0.001f,
            "Audio sidechain altered output before trigger");

    // Ducking follows a 120 ms envelope: the first eight samples remain near
    // unity. Confirm reduction later in the *same* triggered envelope.
    float minimum = 1.f;
    for (int block = 0; block < 28; ++block)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                buffer.setSample(ch, i, 0.5f);
        for (int i = 0; i < 256; ++i)
            buffer.setSample(2, i, 0.f);
        midi.clear();
        p.processBlock(buffer, midi);
        for (int i = 0; i < 256; ++i)
        {
            const float sample = buffer.getSample(0, i);
            require(std::isfinite(sample), "Invalid audio output after trigger");
            minimum = juce::jmin(minimum, sample);
        }
    }
    require(minimum < 0.4f, "External audio trigger did not duck over its duration");
    p.releaseResources();
}

static void processingAndState()
{
    DISidechainProcessor p;
    set(p, "sync", 0);
    set(p, "length", 10);
    set(p, "smooth", 0.1f);
    set(p, "trigger", 2);
    juce::MidiBuffer midi;
    for (int sideChannels : {0, 1, 2})
    {
        auto layout = p.getBusesLayout();
        layout.inputBuses.set(1, sideChannels == 0 ? juce::AudioChannelSet::disabled()
                               : sideChannels == 1 ? juce::AudioChannelSet::mono()
                                                   : juce::AudioChannelSet::stereo());
        require(p.setBusesLayout(layout), "Supported sidechain layout rejected");
        for (double rate : {44100.0, 48000.0, 96000.0})
        {
            for (int mode = 0; mode < 4; ++mode)
            {
                set(p, "mode", (float) mode);
                p.prepareToPlay(rate, 512);
                juce::AudioBuffer<float> buffer(2 + sideChannels, 512);
                buffer.clear();
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i) buffer.setSample(ch, i, 0.5f);
                midi.clear();
                midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
                const auto triggerCountBefore = p.triggerSequence.load();
                p.processBlock(buffer, midi);
                if (mode == 0)
                {
                    require(p.triggerSequence.load() > triggerCountBefore,
                            "MIDI note-on not reflected by trigger activity counter");
                    const auto phase = p.visualPhase.load();
                    const auto gainDb = p.visualGainReductionDb.load();
                    require(std::isfinite(phase) && phase >= 0.f && phase <= 1.f,
                            "UI envelope phase meter out of range");
                    require(std::isfinite(gainDb) && gainDb <= 0.f,
                            "UI gain reduction meter invalid");
                }
                float minimum = 1;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                    {
                        auto sample = buffer.getSample(ch, i);
                        require(std::isfinite(sample), "Nonfinite audio output");
                        minimum = juce::jmin(minimum, sample);
                    }
                if (mode == 0) require(minimum < 0.4f, "MIDI trigger no longer ducks audio");
                require(midi.isEmpty(), "MIDI input was not consumed");
                p.releaseResources();
            }
        }
    }
    p.points[3] = 0.37f;
    set(p, "mix", 0.23f);
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(), (int) saved.getSize());
    require(std::abs(restored.points[3].load() - 0.37f) < 0.0001f, "Envelope state lost");
    require(std::abs(restored.state.getRawParameterValue("mix")->load() - 0.23f) < 0.0001f, "Parameter state lost");
    editorLifecycle(restored);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        DISidechainProcessor processor;
        editorLifecycle(processor); // Hosts may open an editor before prepareToPlay.
        processor.prepareToPlay(48000, 512);
        editorLifecycle(processor);
        processor.releaseResources();
        processingAndState();
        audioSidechainTrigger();
        envelopeInterpolationRegression();
        editableEnvelopeRoundtrip();
        concurrentEnvelopeEditingStress();
        liveSpectrumInputCapture();
        frequencyTargetRegression();
        std::cout << "Editor lifecycle, attachments, MIDI, bus layouts and state: PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
