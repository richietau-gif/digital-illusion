#pragma once
#include "PluginProcessor.h"
class DISidechainEditor : public juce::AudioProcessorEditor, private juce::Timer {
public: explicit DISidechainEditor(DISidechainProcessor&); void paint(juce::Graphics&) override; void resized() override;
private: void timerCallback() override { repaint(); } DISidechainProcessor& proc; std::vector<std::unique_ptr<juce::Slider>> sliders; std::vector<std::unique_ptr<juce::ComboBox>> combos; std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa; std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> ca; std::unique_ptr<juce::ToggleButton> sync; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncA; JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
