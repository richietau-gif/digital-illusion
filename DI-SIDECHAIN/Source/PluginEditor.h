#pragma once
#include "PluginProcessor.h"
#include "DITheme.h"
class DISidechainEditor : public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit DISidechainEditor(DISidechainProcessor&);
 ~DISidechainEditor() override {setLookAndFeel(nullptr);}
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
private:
 void timerCallback() override {
  refreshFilterButtons();
  refreshFxButtons();
  syncUiPreferences();
  if(sequenceCount)
   selectedStep=juce::jlimit(0,juce::jlimit(1,16,(int)sequenceCount->getValue())-1,selectedStep);
  repaint();
 }
 void updatePoint(const juce::MouseEvent&);
 DITheme theme;
 DISidechainProcessor& proc;
 std::array<std::unique_ptr<juce::TextButton>,4> filterButtons;
 void refreshFilterButtons();
 void refreshFxButtons();
 void syncUiPreferences();
 void selectFilter(int);
 juce::Rectangle<int> graph, stepStrip, detailGraph, sequencerPanel, filterPanel, outputPanel, setupBounds;
 static constexpr int designWidth=1536, designHeight=950;
 float uiScale=1.f;
 int renderQuality=1;
 std::unique_ptr<juce::TextButton> copyStep, pasteStep, resetStep, undoButton, redoButton, snapButton;
 struct ShapeEdit {int step;bool sequence;di::ShapeSequencer::Shape before,after;};
 std::vector<ShapeEdit> undoStack, redoStack;
 di::ShapeSequencer::Shape dragBefore{};
 bool draggingShape=false;
 int dragStep=0;
 int dragNode=-1;
 bool dragSequence=false;
 di::ShapeSequencer::Shape captureShape(int,bool) const;
 void applyShape(int,bool,const di::ShapeSequencer::Shape&);
 void pushEdit(int,bool,const di::ShapeSequencer::Shape&,const di::ShapeSequencer::Shape&);
 void undoEdit();
 void redoEdit();
 di::ShapeSequencer::Shape clipboard{};
 bool hasClipboard=false;
 void paintShape(juce::Graphics&, juce::Rectangle<int>, int, bool);
 void paintPanel(juce::Graphics&, juce::Rectangle<int>, const juce::String&);
 void updateDetailPoint(const juce::MouseEvent&);
 void applyFactoryPreset(int);
 void applyShapePreset(int);
 void showPresetMenu();
 void saveUserPreset();
 void loadUserPreset();
 void showShapeMenu();
 void showSetupMenu();
 void selectUiScale(int);
 void selectRenderQuality(int);
 void captureAB(int slot);
 void recallAB(int slot);
 void paintFrequencyResponse(juce::Graphics&,juce::Rectangle<int>);
 int selectedStep=0;
 int selectedLane=0;
 std::array<std::unique_ptr<juce::TextButton>,3> laneTabs;
 std::unique_ptr<juce::ToggleButton> sequenceToggle;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sequenceAttachment;
 std::unique_ptr<juce::Slider> sequenceCount;
 std::unique_ptr<juce::Slider> shapeOffset;
 std::array<std::unique_ptr<juce::Slider>,2> laneSliders;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,2> laneAttachments;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeOffsetAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sequenceCountAttachment;
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::Label>> labels;
 std::vector<std::unique_ptr<juce::ComboBox>> combos;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> ca;
 std::unique_ptr<juce::ComboBox> fxTypeBox;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fxTypeAttachment;
 std::unique_ptr<juce::TextButton> fxToggle, saturatorToggle;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> saturatorAttachment;
 std::unique_ptr<juce::TextButton> presetButton, shapePresetButton, sequencePrev, sequenceNext, setupButton;
 std::array<std::unique_ptr<juce::TextButton>,2> abButtons;
 std::array<juce::MemoryBlock,2> abSnapshots;
 std::array<bool,2> abStored{{false,false}};
 juce::String presetName {"Custom"};
 std::unique_ptr<juce::FileChooser> presetChooser;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxToggleAttachment;
 std::array<std::unique_ptr<juce::Slider>,2> fxSliders;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,2> fxAttachments;
 std::unique_ptr<juce::ToggleButton> sync;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainEditor)
};
