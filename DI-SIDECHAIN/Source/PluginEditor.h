#pragma once
#include "PluginProcessor.h"
class DISidechainEditor : public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit DISidechainEditor(DISidechainProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
private:
 void timerCallback() override {repaint();}
 void updatePoint(const juce::MouseEvent&);
 DISidechainProcessor& proc;
 juce::Rectangle<int> graph;
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::Label>> labels;
 std::vector<std::unique_ptr<juce::ComboBox>> combos;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> ca;
 std::unique_ptr<juce::ToggleButton> sync;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
