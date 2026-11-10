#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class PrecisionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
 PrecisionLookAndFeel();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};

class DISidechainEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
 explicit DISidechainEditor(DISidechainProcessor&);
 ~DISidechainEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;

private:
 using Node=DISidechainProcessor::EnvelopeNode;
 static constexpr int maxNodes=DISidechainProcessor::maxEnvelopeNodes;
 struct CurveState {std::array<Node,maxNodes> nodes{};int count=0;};
 void timerCallback() override;
 void changeScale();
 juce::Rectangle<int> scaled(int,int,int,int) const;
 void pullNodes();
 void pushNodes();
 void recordUndo();
 void undo();
 void redo();
 void setSnapshot(int slot);
 void copySnapshot(int source,int destination);
 void movePreset(int delta);
 float graphX(float x) const;
 float graphY(float y) const;
 bool graphContains(juce::Point<int>) const;

 DISidechainProcessor& proc;
 PrecisionLookAndFeel lookAndFeel;
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::ComboBox>> combos;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
 std::unique_ptr<juce::ComboBox> presetBrowser, gridPicker;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> gridAttachment;
 std::unique_ptr<juce::Slider> curveAmountSlider, syncLengthSlider;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> syncLengthAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> curveAmountAttachment;
 std::unique_ptr<juce::TextButton> prevPreset,nextPreset, aButton,bButton, copyAB,savePreset,undoButton,redoButton;
 std::unique_ptr<juce::TextButton> filterPower,filterEnvelopePower,colorPower,curveShapeButton;
 std::unique_ptr<juce::TextButton> selectTool,bendTool,eraseTool,clearCurve;
 std::unique_ptr<juce::TextButton> settingsButton,powerButton,sourceInt,sourceExt,sourceMidi;
 std::unique_ptr<juce::ToggleButton> sync,bypass,attackProtectToggle;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment,bypassAttachment,filterAttachment,filterEnvelopeAttachment,colorAttachment,attackProtectAttachment;
 juce::ValueTree snapshots[2];
 CurveState snapshotCurves[2];
 std::unique_ptr<juce::FileChooser> presetFileChooser,presetLoadChooser;
 enum class EditTool { select, bend, erase };
 EditTool editTool=EditTool::select;
 void selectEditTool(EditTool);
 int activeSnapshot=0;
 CurveState curve;
 std::vector<CurveState> undoHistory,redoHistory;
 unsigned knownRevision=0;
 int draggedNode=-1;
 float dragStartCurve=0.f;
 bool changeCurve=false, controlsReady=false;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
