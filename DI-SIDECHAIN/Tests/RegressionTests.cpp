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
        require(editor->getWidth() == 850 && editor->getHeight() == 550, "Initial editor size");
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
        editor->setSize(850, 550);
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
    p.processBlock(buffer, midi);
    require(std::abs(buffer.getSample(0, 0) - 0.5f) < 0.001f,
            "Audio sidechain altered output before trigger");

    // At 120 ms envelope length the initial few samples remain near unity.
    // Observe the full envelope instead of incorrectly expecting significant
    // reduction eight samples after the sidechain transient.
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
    require(minimum < 0.4f, "External audio trigger did not duck within envelope duration");
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
                p.processBlock(buffer, midi);
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
        std::cout << "Editor lifecycle, attachments, MIDI, bus layouts and state: PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
