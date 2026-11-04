#pragma once
#include "PluginProcessor.h"

/** DI SIDECHAIN editor. All children are constructed before setSize() invokes resized().
    The envelope is currently the existing eight-point DSP model. */
class DISidechainEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit DISidechainEditor(DISidechainProcessor&);
    ~DISidechainEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    void timerCallback() override;
    void updateSpectrum();
    void updatePoint(const juce::MouseEvent&);
    void updateBand(const juce::MouseEvent&);
    void setParameter(const char*, float);
    juce::Point<float> localDesignPoint(const juce::MouseEvent&) const;
    juce::Rectangle<float> graph {33.f, 162.f, 625.f, 250.f};
    DISidechainProcessor& proc;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> ca;
    std::unique_ptr<juce::ToggleButton> sync;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
    juce::LookAndFeel_V4 skin;
    juce::dsp::FFT spectrumFft {11};
    std::array<float,4096> spectrumWork {};
    std::array<float,24> spectrumBands {};
    uint64_t lastSpectrumPosition = 0;
    bool advanced = false;
    int bandHandle = -1;
    int selectedPoint = -1;
    bool snapToGrid = false;
    int curvatureSegment = -1;
    uint32_t lastTriggerSequence = 0;
    int triggerFlashTicks = 0;
    static constexpr float designWidth = 700.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
