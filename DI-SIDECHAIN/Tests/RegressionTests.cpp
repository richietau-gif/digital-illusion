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
        require(editor->getWidth() == 1120 && editor->getHeight() == 760, "Initial editor size");
        require(editor->getNumChildComponents() == 54, "Missing editor controls");
        int sliders = 0, combos = 0, buttons = 0;
        for (auto* child : editor->getChildren())
        {
            require(!child->getBounds().isEmpty(), "Control was not laid out during construction");
            sliders += dynamic_cast<juce::Slider*>(child) != nullptr;
            combos += dynamic_cast<juce::ComboBox*>(child) != nullptr;
            buttons += dynamic_cast<juce::ToggleButton*>(child) != nullptr;
        }
        require(sliders == 21 && combos == 4 && buttons == 2, "Control types changed");
        editor->setSize(1300, 760);
        editor->setSize(1120, 760);
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

static void factoryEnvelopePresetsRegression()
{
    DISidechainProcessor processor;
    const float expectedFirstDip[]={.02f,0.f,.35f,.15f,.08f,.05f};
    float shapeChecksums[6]={};
    for(int preset=0;preset<6;++preset){
        processor.applyEnvelopePreset(preset);
        require(processor.getEnvelopeNodeCount()==7,
                "Factory envelope did not create seven editable points");
        require(std::abs(processor.getEnvelopeNode(0).x)<.00001f,
                "Preset curve must start exactly at beat zero");
        require(std::abs(processor.getEnvelopeNode(6).x-1.f)<.00001f,
                "Preset curve must end at normalized beat one");
        for(int i=0;i<7;++i){
            const auto node=processor.getEnvelopeNode(i);
            require(std::isfinite(node.x)&&std::isfinite(node.y)&&
                    node.y>=0.f&&node.y<=1.f,
                    "Factory preset contained invalid envelope points");
            if(i>0)require(node.x>processor.getEnvelopeNode(i-1).x,
                            "Preset x coordinates are not strictly increasing");
            if(i>0&&i<6)shapeChecksums[preset]+=node.y*(float)(i+1);
        }
        require(std::abs(processor.getEnvelopeNode(1).y-expectedFirstDip[preset])<.001f,
                "Factory preset loaded unexpected envelope shape");
    }
    for(int first=0;first<6;++first)
        for(int second=first+1;second<6;++second)
            require(std::abs(shapeChecksums[first]-shapeChecksums[second])>.001f,
                    "Two factory presets are indistinguishable");
    processor.applyEnvelopePreset(5);
    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(),(int)saved.getSize());
    require(restored.getEnvelopeNodeCount()==7,
            "Factory shape was not persisted to Ableton project state");
    for(int i=0;i<7;++i){
        const auto original=processor.getEnvelopeNode(i);
        const auto reloaded=restored.getEnvelopeNode(i);
        require(std::abs(original.x-reloaded.x)<.0001f &&
                std::abs(original.y-reloaded.y)<.0001f,
                "Preset shape changed during host state restoration");
    }
    // Changing a node must retain editability after preset selection.
    restored.setEnvelopeNode(3,.51f,.42f);
    require(std::abs(restored.getEnvelopeNode(3).y-.42f)<.001f,
            "Factory preset curve is not editable");
}

static void factoryPresetGuiRegression()
{
    DISidechainProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    juce::ComboBox* presetCombo=nullptr;
    juce::ToggleButton* filterCheckbox=nullptr;
    for(auto* child:editor->getChildren()){
        if(child->getComponentID()=="envelopePreset")
            presetCombo=dynamic_cast<juce::ComboBox*>(child);
        if(auto* toggle=dynamic_cast<juce::ToggleButton*>(child))
            if(toggle->getButtonText()=="FILTER")filterCheckbox=toggle;
    }
    require(presetCombo!=nullptr,"Factory preset dropdown missing from UI");
    require(filterCheckbox!=nullptr,"FILTER checkbox missing from UI");
    presetCombo->setSelectedId(6,juce::sendNotificationSync);
    require(processor.getEnvelopeNodeCount()==7 &&
            std::abs(processor.getEnvelopeNode(1).y-.05f)<.001f,
            "Selecting Wobble did not change the processed envelope");
    require(editor->getWidth()==1120&&editor->getHeight()==760,
            "Wide always-on filter layout must be fixed 1120x760");
    int visibleSliders=0, filterButtons=0, slopeButtons=0;
    juce::TextButton* bandPass=nullptr;
    for(auto* child:editor->getChildren()){
        if(auto* slider=dynamic_cast<juce::Slider*>(child)){
            if(slider->isVisible())++visibleSliders;
            require(slider->getBounds().getRight()<=editor->getWidth(),
                    "A knob extends beyond the wide editor");
            require(slider->getBounds().getBottom()<=editor->getHeight(),
                    "A knob extends below the wide editor");
        }
        const auto id=child->getComponentID();
        if(id.startsWith("filter-type-")){
            ++filterButtons;
            if(id=="filter-type-2")bandPass=dynamic_cast<juce::TextButton*>(child);
        }
        if(id.startsWith("filter-slope-"))++slopeButtons;
    }
    require(visibleSliders==21,
            "Filter and sidechain knobs must always be simultaneously visible");
    require(filterButtons==4&&slopeButtons==2&&bandPass!=nullptr,
            "Filter must expose four compact type and two slope buttons");
    bandPass->triggerClick();
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    require(juce::roundToInt(processor.state.getRawParameterValue("outputFilterType")->load())==2,
            "Band Pass filter button did not update processor parameter");
    filterCheckbox->triggerClick();
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    require(editor->getWidth()==1120&&editor->getHeight()==760,
            "FILTER toggle must not resize the new wide editor");
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

static float measureFrequencyMode(int mode,float hz,float depth,
                                  float lowBand=1.f,float midBand=1.f,float highBand=1.f)
{
    DISidechainProcessor processor;
    processor.prepareToPlay(48000.,256);
    set(processor,"trigger",2);
    set(processor,"mode",(float)mode);
    set(processor,"depth",depth);
    set(processor,"multiLowDepth",lowBand);
    set(processor,"multiMidDepth",midBand);
    set(processor,"multiHighDepth",highBand);
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

static void multibandFrequencyRegression()
{
    // Each band's depth independently changes the corresponding spectrum.
    // With zero global depth, even MULTI must be 100% transparent.
    for(float hz:{65.f,430.f,6400.f})
        require(std::abs(measureFrequencyMode(4,hz,0.f)-
                         measureFrequencyMode(0,hz,0.f))<.0001f,
                "MULTI altered sound with global depth at zero");
    const float bassDry=measureFrequencyMode(4,65.f,0.f);
    const float midDry=measureFrequencyMode(4,430.f,0.f);
    const float trebleDry=measureFrequencyMode(4,6400.f,0.f);
    const float bassDuck=measureFrequencyMode(4,65.f,1.f,1.f,0.f,0.f);
    const float midDuck=measureFrequencyMode(4,430.f,1.f,0.f,1.f,0.f);
    const float highDuck=measureFrequencyMode(4,6400.f,1.f,0.f,0.f,1.f);
    require(bassDuck<bassDry*.85f,"MULTI LOW depth did not reduce bass");
    require(midDuck<midDry*.85f,"MULTI MID depth did not reduce mids");
    require(highDuck<trebleDry*.85f,"MULTI HIGH depth did not reduce treble");

    const float bassKeep=measureFrequencyMode(4,65.f,1.f,0.f,1.f,1.f);
    const float highKeep=measureFrequencyMode(4,6400.f,1.f,1.f,1.f,0.f);
    require(bassKeep>bassDuck*1.2f,"MULTI LOW depth cannot preserve bass");
    require(highKeep>highDuck*1.2f,"MULTI HIGH depth cannot preserve highs");

    DISidechainProcessor p;
    set(p,"mode",4.f);
    set(p,"multiLowDepth",.35f);
    set(p,"multiMidDepth",.7f);
    set(p,"multiHighDepth",.9f);
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(),(int)saved.getSize());
    for(auto [id,value]:{std::pair<const char*,float>{"mode",4.f},
                        {"multiLowDepth",.35f},{"multiMidDepth",.7f},
                        {"multiHighDepth",.9f}})
        require(std::abs(restored.state.getRawParameterValue(id)->load()-value)<.01f,
                "MULTI band parameters were not saved with host state");
}

static void rhythmicDivisionRegression()
{
    // No playhead: engine must use its documented 120 BPM fallback.
    // Legacy parameter indices remain unchanged; new divisions are appended.
    struct Case { int choice; double beats; };
    const Case cases[] = {
        {0,.125}, {1,.25}, {2,.5}, {3,1.}, {4,2.}, {5,4.}, {6,8.},
        {7,1./6.}, {8,1./3.}, {9,2./3.},
        {10,.375}, {11,.75}, {12,1.5}
    };
    for (const auto& test : cases)
    {
        DISidechainProcessor p;
        set(p,"trigger",2);
        set(p,"sync",1);
        set(p,"playbackMode",0);
        set(p,"division",(float)test.choice);
        p.prepareToPlay(48000.,256);
        juce::AudioBuffer<float> buffer(2,256);
        buffer.clear();
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),0);
        p.processBlock(buffer,midi);
        const double expected=256./(48000.*(.5*test.beats));
        const auto actual=p.visualPhase.load();
        require(std::isfinite(actual) && std::abs(actual-expected)<.0001,
                "Tempo-synced division length differs from 120 BPM reference");
        p.releaseResources();
    }
}

static void playbackModeRegression()
{
    DISidechainProcessor processor;
    processor.prepareToPlay(48000., 256);
    set(processor,"trigger",2); // MIDI only
    set(processor,"sync",0);
    set(processor,"length",20);
    set(processor,"smooth",.1f);
    set(processor,"depth",1.f);
    set(processor,"playbackMode",1); // Loop without host position: free-running fallback
    juce::AudioBuffer<float> buffer(2,256);
    juce::MidiBuffer midi;
    float minimum=1.f,maximum=0.f;
    for(int block=0;block<25;++block)
    {
        for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)buffer.setSample(ch,i,.5f);
        midi.clear();
        processor.processBlock(buffer,midi);
        for(int i=0;i<256;++i)
        {
            const float v=buffer.getSample(0,i);
            require(std::isfinite(v),"LOOP produced invalid audio");
            minimum=juce::jmin(minimum,v);
            maximum=juce::jmax(maximum,v);
        }
    }
    require(minimum<.20f,"LOOP mode did not periodically duck without MIDI");
    require(maximum>.35f,"LOOP mode did not release to unity");

    set(processor,"playbackMode",0); // One Shot should end at unity
    processor.prepareToPlay(48000., 256);
    for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)buffer.setSample(ch,i,.5f);
    midi.clear();processor.processBlock(buffer,midi);
    require(std::abs(buffer.getSample(0,255)-.5f)<.001f,
            "ONE SHOT produced unexpected reduction without a trigger");
    processor.releaseResources();
}

static void midiBreakbeatNoteFilter()
{
    DISidechainProcessor p;
    p.prepareToPlay(48000.,256);
    set(p,"trigger",2); // MIDI only
    set(p,"sync",0);
    set(p,"length",250);
    set(p,"smooth",.1f);
    set(p,"midiNote",36);
    set(p,"midiChannel",10);
    juce::AudioBuffer<float> buffer(2,256);
    auto process=[&](juce::MidiBuffer& events){
        for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)
            buffer.setSample(ch,i,.25f);
        p.processBlock(buffer,events);
    };
    juce::MidiBuffer events;
    const auto begin=p.triggerSequence.load();
    events.addEvent(juce::MidiMessage::noteOn(10,38,(juce::uint8)110),11);
    events.addEvent(juce::MidiMessage::noteOn(9,36,(juce::uint8)110),17);
    process(events);
    require(p.triggerSequence.load()==begin,"Unexpected note or channel triggered");
    events.addEvent(juce::MidiMessage::noteOn(10,36,(juce::uint8)110),37);
    events.addEvent(juce::MidiMessage::noteOn(10,36,(juce::uint8)110),171);
    process(events);
    require(p.triggerSequence.load()==begin+2,"Irregular MIDI note-on triggers were skipped");
    require(p.visualPhase.load()>0.f&&p.visualPhase.load()<1.f,
            "Envelope phase did not advance from the last MIDI trigger");
    set(p,"midiNote",-1);
    set(p,"midiChannel",0);
    events.addEvent(juce::MidiMessage::noteOn(3,45,(juce::uint8)100),92);
    process(events);
    require(p.triggerSequence.load()==begin+3,"Any-note/any-channel mode did not trigger");
    p.releaseResources();
}

static float outputFilterSignalRms(float freq,bool enabled,int type,float amount,
                                  float resonance=.70710678f,float drive=0.f,int slope=0)
{
    DISidechainProcessor p;
    p.prepareToPlay(48000.,256);
    set(p,"trigger",2);
    set(p,"sync",0);
    set(p,"length",2000);
    set(p,"smooth",.1f);
    set(p,"depth",0.f); // Filter has its own envelope: independent of ducking.
    set(p,"outputFilterMin",250.f);
    set(p,"outputFilterMax",16000.f);
    set(p,"outputFilterAmount",amount);
    set(p,"outputFilterType",(float)type);
    set(p,"outputFilterResonance",resonance);
    set(p,"outputFilterDrive",drive);
    set(p,"outputFilterSlope",(float)slope);
    set(p,"outputFilterOn",enabled?1.f:0.f);
    for(auto& point:p.points)point.store(0.f);
    juce::AudioBuffer<float> buffer(2,256);
    juce::MidiBuffer midi;
    double sum=0.;
    int count=0;
    for(int block=0;block<48;++block)
    {
        for(int i=0;i<256;++i)
        {
            const float v=.35f*std::sin(juce::MathConstants<float>::twoPi*
                                      freq*(block*256+i)/48000.f);
            buffer.setSample(0,i,v);
            buffer.setSample(1,i,v);
        }
        midi.clear();
        if(block==0)midi.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)110),0);
        p.processBlock(buffer,midi);
        if(block>=24)
            for(int i=0;i<256;++i)
            {
                const float value=buffer.getSample(0,i);
                require(std::isfinite(value),"Output envelope filter produced nonfinite audio");
                sum+=(double)value*value;++count;
            }
    }
    p.releaseResources();
    return (float)std::sqrt(sum/(double)count);
}

static void liveWobbleCutoffRegression()
{
    DISidechainProcessor p;
    p.prepareToPlay(48000.,256);
    set(p,"trigger",2);
    set(p,"playbackMode",0); // one shot
    set(p,"sync",0);
    set(p,"length",500);
    set(p,"depth",0); // modulation must work without volume ducking
    set(p,"outputFilterOn",1);
    set(p,"outputFilterType",0); // low pass
    set(p,"outputFilterMin",120);
    set(p,"outputFilterMax",12000);
    set(p,"outputFilterAmount",1);
    for(int i=0;i<8;++i)p.points[i]=(float)i/7.f;
    juce::AudioBuffer<float> buffer(2,256);
    juce::MidiBuffer events;
    double early=0.,late=0.;
    int earlyCount=0,lateCount=0;
    for(int block=0;block<92;++block)
    {
        for(int i=0;i<256;++i)
        {
            const float sample=.2f*std::sin(juce::MathConstants<float>::twoPi*
                                  4000.f*(block*256+i)/48000.f);
            buffer.setSample(0,i,sample);
            buffer.setSample(1,i,sample);
        }
        events.clear();
        if(block==0)events.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)100),0);
        p.processBlock(buffer,events);
        for(int i=0;i<256;++i)
        {
            const int position=block*256+i;
            const float v=buffer.getSample(0,i);
            require(std::isfinite(v),"Live cutoff automation produced invalid output");
            if(position>=1500&&position<3500){early+=v*v;++earlyCount;}
            if(position>=19000&&position<21000){late+=v*v;++lateCount;}
        }
    }
    require(earlyCount>0&&lateCount>0,"Wobble measurement windows not processed");
    const float quiet=(float)std::sqrt(early/earlyCount);
    const float open=(float)std::sqrt(late/lateCount);
    require(open>quiet*3.f,"Envelope movement did not sweep output filter cutoff");
    p.releaseResources();
}

static void resonantDriveStabilityRegression()
{
    // Stress state-variable stages and antialiased drive at every supported
    // output mode and sample rate without host-side allocations.
    for(double sampleRate:{44100.,48000.,96000.})
      for(int type=0;type<4;++type)
        for(int slope=0;slope<2;++slope)
        {
            DISidechainProcessor p;
            p.prepareToPlay(sampleRate,128);
            set(p,"trigger",2);
            set(p,"playbackMode",1);
            set(p,"sync",0);
            set(p,"length",23);
            set(p,"depth",0.f);
            set(p,"outputFilterOn",1.f);
            set(p,"outputFilterType",(float)type);
            set(p,"outputFilterSlope",(float)slope);
            set(p,"outputFilterResonance",11.8f);
            set(p,"outputFilterDrive",1.f);
            set(p,"outputFilterMin",32.f);
            set(p,"outputFilterMax",19000.f);
            set(p,"outputFilterAmount",1.f);
            juce::AudioBuffer<float> buffer(2,128);
            juce::MidiBuffer midi;
            for(int block=0;block<45;++block)
            {
                for(int i=0;i<128;++i)
                {
                    const float v=.7f*std::sin(juce::MathConstants<float>::twoPi*
                         560.f*(block*128+i)/(float)sampleRate);
                    buffer.setSample(0,i,v);
                    buffer.setSample(1,i,v*.8f);
                }
                midi.clear();
                p.processBlock(buffer,midi);
                for(int ch=0;ch<2;++ch)
                    for(int i=0;i<128;++i)
                    {
                        const float v=buffer.getSample(ch,i);
                        require(std::isfinite(v)&&std::abs(v)<100.f,
                           "High resonance/drive produced unstable audio");
                    }
            }
            p.releaseResources();
        }
}

static void outputEnvelopeFilterRegression()
{
    const float bassDry=outputFilterSignalRms(100.f,false,0,1.f);
    const float trebleDry=outputFilterSignalRms(5000.f,false,0,1.f);
    require(std::abs(bassDry-.35f/std::sqrt(2.f))<.02f,
            "Disabled output filter altered low-frequency audio");
    require(std::abs(trebleDry-.35f/std::sqrt(2.f))<.02f,
            "Disabled output filter altered high-frequency audio");
    const float bassLP=outputFilterSignalRms(100.f,true,0,1.f);
    const float trebleLP=outputFilterSignalRms(5000.f,true,0,1.f);
    require(bassLP>bassDry*.78f,"Lowpass incorrectly removes bass");
    require(trebleLP<trebleDry*.42f,"Envelope-controlled lowpass did not reduce treble");
    const float bassHP=outputFilterSignalRms(100.f,true,1,1.f);
    const float trebleHP=outputFilterSignalRms(5000.f,true,1,1.f);
    require(bassHP<bassDry*.7f,"Envelope-controlled highpass did not reduce bass");
    require(trebleHP>trebleDry*.75f,"Highpass incorrectly removes treble");
    // Amount 0 keeps the cutoff at MAX, so almost all bass gets through.
    const float staticBass=outputFilterSignalRms(100.f,true,0,0.f);
    require(staticBass>bassLP*.9f,"Static cutoff modulation setting is unstable");
    // Band pass must select the cutoff region while rejecting either extreme.
    const float midBP=outputFilterSignalRms(250.f,true,2,1.f);
    const float subBP=outputFilterSignalRms(40.f,true,2,1.f);
    const float airBP=outputFilterSignalRms(6500.f,true,2,1.f);
    require(midBP>subBP*1.7f&&midBP>airBP*1.7f,
            "Band Pass failed to isolate target frequency band");
    const float midNotch=outputFilterSignalRms(250.f,true,3,1.f);
    const float airNotch=outputFilterSignalRms(6500.f,true,3,1.f);
    require(midNotch<airNotch*.7f,"Notch failed to suppress its center frequency");
    const float midResonant=outputFilterSignalRms(250.f,true,0,1.f,4.f);
    require(midResonant>midBP*1.5f,"Resonance control failed to boost cutoff region");
    const float shallow=outputFilterSignalRms(6500.f,true,0,1.f,.7071f,0.f,0);
    const float steep=outputFilterSignalRms(6500.f,true,0,1.f,.7071f,0.f,1);
    require(steep<shallow*.75f,"24 dB filter did not attenuate treble more than 12 dB");
    const float driven=outputFilterSignalRms(5000.f,true,1,1.f,.7071f,.9f,0);
    require(std::isfinite(driven)&&std::abs(driven-trebleDry)>.005f,
            "Drive did not change output or produced invalid audio");
    DISidechainProcessor p;
    set(p,"outputFilterOn",1.f);
    set(p,"outputFilterType",1.f);
    set(p,"outputFilterMin",260.f);
    set(p,"outputFilterMax",9000.f);
    set(p,"outputFilterAmount",.75f);
    set(p,"outputFilterResonance",5.f);
    set(p,"outputFilterDrive",.6f);
    set(p,"outputFilterSlope",1.f);
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(),(int)saved.getSize());
    for(auto [id,expected]:{std::pair<const char*,float>{"outputFilterOn",1.f},
                            {"outputFilterType",1.f},{"outputFilterMin",260.f},
                            {"outputFilterMax",9000.f},{"outputFilterAmount",.75f},
                            {"outputFilterResonance",5.f},{"outputFilterDrive",.6f},
                            {"outputFilterSlope",1.f}})
        require(std::abs(restored.state.getRawParameterValue(id)->load()-expected)<.05f,
                "Output filter parameter lost when restoring an Ableton set");
}

static uint32_t measureDetectorTiming(float attackMs,float releaseMs,bool secondTransient)
{
    DISidechainProcessor p;
    auto buses=p.getBusesLayout();
    buses.inputBuses.set(1,juce::AudioChannelSet::mono());
    require(p.setBusesLayout(buses),"Detector timing test cannot enable sidechain");
    set(p,"trigger",1);
    set(p,"threshold",-12.f);
    set(p,"retrigger",1.f);
    set(p,"detectorAttackMs",attackMs);
    set(p,"detectorReleaseMs",releaseMs);
    p.prepareToPlay(48000.,256);
    juce::AudioBuffer<float> buffer(3,256);
    juce::MidiBuffer midi;
    for(int block=0;block<20;++block)
    {
        buffer.clear();
        for(int i=0;i<256;++i){
            buffer.setSample(0,i,.5f);
            buffer.setSample(1,i,.5f);
        }
        if(block==0||(secondTransient&&block==10))
            for(int i=16;i<46;++i)buffer.setSample(2,i,.9f);
        midi.clear();
        p.processBlock(buffer,midi);
    }
    const auto count=p.triggerSequence.load();
    p.releaseResources();
    return count;
}

static void detectorAttackReleaseRegression()
{
    require(measureDetectorTiming(0.f,4.16f,false)==1,
            "Instantaneous detector attack missed external kick");
    require(measureDetectorTiming(30.f,4.16f,false)==0,
            "Slow detector attack did not reject short transient");
    require(measureDetectorTiming(0.f,4.16f,true)==2,
            "Fast detector release failed to rearm between kicks");
    require(measureDetectorTiming(0.f,150.f,true)==1,
            "Slow detector release retriggered before its level fell");
    DISidechainProcessor p;
    set(p,"detectorAttackMs",8.f);
    set(p,"detectorReleaseMs",65.f);
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(),(int)saved.getSize());
    require(std::abs(restored.state.getRawParameterValue("detectorAttackMs")->load()-8.f)<.05f,
            "Detector attack failed state serialization");
    require(std::abs(restored.state.getRawParameterValue("detectorReleaseMs")->load()-65.f)<.05f,
            "Detector release failed state serialization");
}

static void filteredExternalAudioTriggerRegression()
{
    auto countTriggers=[](float sourceHz,float hpHz){
        DISidechainProcessor processor;
        auto buses=processor.getBusesLayout();
        buses.inputBuses.set(1,juce::AudioChannelSet::mono());
        require(processor.setBusesLayout(buses),"Filtered mono sidechain layout rejected");
        set(processor,"trigger",1);
        set(processor,"playbackMode",0);
        set(processor,"threshold",-18.f);
        set(processor,"detectorHP",hpHz);
        processor.prepareToPlay(48000.,256);
        juce::AudioBuffer<float> buffer(3,256);
        juce::MidiBuffer midi;
        for(int block=0;block<12;++block)
        {
            buffer.clear();
            for(int i=0;i<256;++i){
                buffer.setSample(0,i,.4f);
                buffer.setSample(1,i,.4f);
                buffer.setSample(2,i,.45f*std::sin(
                    juce::MathConstants<float>::twoPi*sourceHz*
                    (block*256+i)/48000.f));
            }
            midi.clear();
            processor.processBlock(buffer,midi);
        }
        const auto count=processor.triggerSequence.load();
        processor.releaseResources();
        return count;
    };
    require(countTriggers(70.f,0.f)>0,
            "Unfiltered legacy external sidechain no longer responds to bass");
    require(countTriggers(70.f,300.f)==0,
            "Detector HP failed to reject low-frequency interference");
    require(countTriggers(1000.f,300.f)>0,
            "Detector HP incorrectly rejected an above-cutoff transient");

    DISidechainProcessor processor;
    set(processor,"detectorHP",275.f);
    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    DISidechainProcessor restored;
    restored.setStateInformation(saved.getData(),(int)saved.getSize());
    require(std::abs(restored.state.getRawParameterValue("detectorHP")->load()-275.f)<.01f,
            "Detector HP cutoff was not restored from host state");
}

static void stereoExternalAudioTriggerRegression()
{
    DISidechainProcessor p;
    auto layout=p.getBusesLayout();
    layout.inputBuses.set(1,juce::AudioChannelSet::stereo());
    require(p.setBusesLayout(layout),"Stereo sidechain layout was rejected");
    set(p,"trigger",1); // Audio only: ignores all incoming MIDI
    set(p,"playbackMode",0); // One Shot
    set(p,"sync",0);
    set(p,"length",100);
    set(p,"smooth",.1f);
    set(p,"retrigger",1);
    set(p,"threshold",-24.f);
    set(p,"mode",0);
    p.prepareToPlay(48000.,256);
    juce::AudioBuffer<float> buffer(4,256);
    juce::MidiBuffer midi;
    const auto triggerBefore=p.triggerSequence.load();
    for(int block=0;block<9;++block)
    {
        buffer.clear();
        for(int i=0;i<256;++i)
        {
            buffer.setSample(0,i,.5f);
            buffer.setSample(1,i,.5f);
        }
        // Three irregular external kick transients, alternating L/R.
        // Audio input channels 2,3 must never leak into program audio.
        if(block==0)for(int i=32;i<64;++i)buffer.setSample(2,i,.9f);
        if(block==3)for(int i=70;i<102;++i)buffer.setSample(3,i,.9f);
        if(block==7)for(int i=22;i<54;++i)buffer.setSample(2,i,.9f);
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)110),5);
        p.processBlock(buffer,midi);
        for(int channel=0;channel<2;++channel)
            for(int i=0;i<256;++i)
            {
                const float out=buffer.getSample(channel,i);
                require(std::isfinite(out),"Stereo audio sidechain produced nonfinite output");
                require(out>=-0.001f&&out<=.5001f,
                        "Stereo audio sidechain leaked into or amplified the main signal");
            }
    }
    require(p.triggerSequence.load()==triggerBefore+3,
            "Stereo external audio transients did not trigger exactly three times");
    p.releaseResources();
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
        stereoExternalAudioTriggerRegression();
        filteredExternalAudioTriggerRegression();
        detectorAttackReleaseRegression();
        outputEnvelopeFilterRegression();
        resonantDriveStabilityRegression();
        liveWobbleCutoffRegression();
        envelopeInterpolationRegression();
        factoryEnvelopePresetsRegression();
        factoryPresetGuiRegression();
        editableEnvelopeRoundtrip();
        concurrentEnvelopeEditingStress();
        liveSpectrumInputCapture();
        frequencyTargetRegression();
        multibandFrequencyRegression();
        midiBreakbeatNoteFilter();
        playbackModeRegression();
        rhythmicDivisionRegression();
        std::cout << "Editor lifecycle, attachments, MIDI, bus layouts and state: PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
