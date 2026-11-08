#pragma once
#include "PluginProcessor.h"

/** DI SIDECHAIN editor. All children are constructed before setSize() invokes resized().
    The envelope is currently the existing eight-point DSP model. */
class DIStudioSkin : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,
                          juce::Slider::SliderStyle,juce::Slider&) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
};

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
    juce::Rectangle<float> graph {286.f, 213.f, 795.f, 265.f};
    DISidechainProcessor& proc;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> ca;
    std::unique_ptr<juce::ToggleButton> sync;
    std::unique_ptr<juce::ToggleButton> outputFilterToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> outputFilterAttachment;
    std::unique_ptr<juce::ComboBox> envelopePreset;
    std::array<std::unique_ptr<juce::TextButton>,4> filterTypeButtons;
    std::array<std::unique_ptr<juce::TextButton>,2> filterSlopeButtons;
    std::array<std::unique_ptr<juce::TextButton>,6> presetButtons;
    std::array<std::unique_ptr<juce::TextButton>,2> curveViewButtons;
    std::array<std::unique_ptr<juce::TextButton>,2> filterBankButtons;
    std::unique_ptr<juce::ToggleButton> filterEnvelopeLinkButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filterEnvelopeLinkAttachment;
    bool filterView=false;
    int editedFilterBank=0;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
    DIStudioSkin skin;
    juce::dsp::FFT spectrumFft {11};
    std::array<float,4096> spectrumWork {};
    std::array<float,24> spectrumBands {};
    uint64_t lastSpectrumPosition = 0;
    int bandHandle = -1;
    int selectedPoint = -1;
    bool snapToGrid = false;
    int curvatureSegment = -1;
    uint32_t lastTriggerSequence = 0;
    int triggerFlashTicks = 0;
    static constexpr float designWidth = 1380.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
