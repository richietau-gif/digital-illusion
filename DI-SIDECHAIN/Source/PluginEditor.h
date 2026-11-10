#pragma once
#include "PluginProcessor.h"
#include <memory>
#include <vector>

class PrecisionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PrecisionLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
};

class DISidechainEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DISidechainEditor(DISidechainProcessor&);
    ~DISidechainEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setWindowScaleFromParameter();
    juce::Rectangle<int> scaled(int x, int y, int w, int h) const;

    DISidechainProcessor& proc;
    PrecisionLookAndFeel lookAndFeel;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::unique_ptr<juce::ToggleButton> sync, bypass;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment, bypassAttachment;
    bool controlsReady = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
