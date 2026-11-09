#pragma once
#include <JuceHeader.h>
#include <cmath>

// All visual rendering is vector-based: no external textures or GPU dependencies.
class DITheme final : public juce::LookAndFeel_V4 {
public:
 static juce::Colour cyan() noexcept {return juce::Colour(0xff16ddf3);}
 static juce::Colour lime() noexcept {return juce::Colour(0xff9cff25);}
 static juce::Colour orange() noexcept {return juce::Colour(0xffff931e);}
 static juce::Colour dark() noexcept {return juce::Colour(0xff07131d);}
 DITheme() {
  setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff0a1a26));
  setColour(juce::PopupMenu::textColourId,juce::Colours::white);
  setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff14556b));
  setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xff172c39));
  setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff0a1c28));
  setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff27546a));
  setColour(juce::ComboBox::arrowColourId,cyan());
  setColour(juce::ComboBox::textColourId,juce::Colours::white);
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float pos,float start,float end,juce::Slider& slider) override {
  // SIDEFX matte rotary control: no oval glass reflection or bulky bevels.
  const auto area=juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height);
  const auto c=area.getCentre();
  const float radius=juce::jmax(8.f,juce::jmin(area.getWidth(),area.getHeight())*.34f);
  const float angle=start+juce::jlimit(0.f,1.f,pos)*(end-start);
  juce::Colour accent=slider.findColour(juce::Slider::rotarySliderFillColourId);
  if(accent.isTransparent())accent=cyan();
  const bool enabled=slider.isEnabled();
  if(!enabled)accent=juce::Colour(0xff3d4b54);
  g.setColour(juce::Colours::black.withAlpha(.3f));
  g.fillEllipse(c.x-radius+1.2f,c.y-radius+2.4f,2.f*radius,2.f*radius);
  g.setColour(juce::Colour(0xff101e29));
  g.fillEllipse(c.x-radius,c.y-radius,2.f*radius,2.f*radius);
  g.setColour(enabled?juce::Colour(0xff4b6779):juce::Colour(0xff26323c));
  g.drawEllipse(c.x-radius,c.y-radius,2.f*radius,2.f*radius,1.f);
  juce::Path track;
  track.addCentredArc(c.x,c.y,radius+4.f,radius+4.f,0.f,start,end,true);
  g.setColour(enabled?juce::Colour(0xff304454):juce::Colour(0xff1e2a34));
  g.strokePath(track,juce::PathStrokeType(2.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  juce::Path progress;
  progress.addCentredArc(c.x,c.y,radius+4.f,radius+4.f,0.f,start,angle,true);
  g.setColour(accent);
  g.strokePath(progress,juce::PathStrokeType(3.4f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  const float length=radius*.68f;
  const auto tip=c.translated(std::sin(angle)*length,-std::cos(angle)*length);
  juce::Path pointer;pointer.startNewSubPath(c);pointer.lineTo(tip);
  g.setColour(enabled?juce::Colour(0xffe7f7ff):juce::Colour(0xff53616c));
  g.strokePath(pointer,juce::PathStrokeType(2.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(accent);
  g.fillEllipse(c.x-2.f,c.y-2.f,4.f,4.f);
 }
 void drawButtonBackground(juce::Graphics& g,juce::Button& button,
                           const juce::Colour&,bool over,bool down) override {
  const auto bounds=button.getLocalBounds().toFloat().reduced(1.f);
  const bool on=button.getToggleState();
  auto accent=button.findColour(juce::TextButton::buttonOnColourId);
  auto inactive=button.findColour(juce::TextButton::buttonColourId);
  if(accent.isTransparent())accent=lime();
  if(inactive.isTransparent())inactive=juce::Colour(0xff162633);
  const auto colour=on?accent:down?inactive.brighter(.24f):over?inactive.brighter(.12f):inactive;
  g.setColour(colour);
  g.fillRoundedRectangle(bounds,5.f);
  if(!on){
   g.setColour(juce::Colour(0xff2d4959));
   g.drawRoundedRectangle(bounds,5.f,1.f);
  }
 }
 void drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool) override {
  const bool on=button.getToggleState();
  auto colour=button.findColour(on?juce::TextButton::textColourOnId:
                                   juce::TextButton::textColourOffId);
  if(colour.isTransparent())colour=on?juce::Colour(0xff071018):juce::Colours::white;
  g.setColour(colour);
  const float size=juce::jlimit(8.5f,13.f,button.getHeight()*.47f);
  g.setFont(juce::FontOptions(size,juce::Font::bold));
  g.drawFittedText(button.getButtonText(),button.getLocalBounds().reduced(3,1),
                   juce::Justification::centred,1);
 }

};