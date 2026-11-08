#pragma once
#include <JuceHeader.h>
class DITheme final : public juce::LookAndFeel_V4 {
public:
 DITheme() {
  setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff18212b));
  setColour(juce::PopupMenu::textColourId,juce::Colours::white);
  setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xff26313d));
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider&) override {
  const auto b=juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height).reduced(8.f);
  const float radius=juce::jmin(b.getWidth(),b.getHeight())*.43f;
  const auto c=b.getCentre();
  const float a=start+position*(end-start);
  juce::Path arc;arc.addCentredArc(c.x,c.y,radius,radius,0,start,end,true);
  g.setColour(juce::Colour(0xff35414c));g.strokePath(arc,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  juce::Path active;active.addCentredArc(c.x,c.y,radius,radius,0,start,a,true);
  g.setColour(juce::Colour(0xffa5ff35));g.strokePath(active,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(juce::Colour(0xff111820));g.fillEllipse(c.x-radius+6,c.y-radius+6,2*(radius-6),2*(radius-6));
  g.setColour(juce::Colours::white);juce::Path tick;
  tick.startNewSubPath(c.x+std::sin(a)*(radius-17),c.y-std::cos(a)*(radius-17));
  tick.lineTo(c.x+std::sin(a)*(radius-7),c.y-std::cos(a)*(radius-7));
  g.strokePath(tick,juce::PathStrokeType(2.5f));
 }
 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) override {
  auto rect=b.getLocalBounds().toFloat().reduced(1.f);
  const bool selected=b.getToggleState();
  g.setColour(selected?juce::Colour(0xffa5ff35):down?juce::Colour(0xff354658):over?juce::Colour(0xff303d4a):juce::Colour(0xff222c38));
  g.fillRoundedRectangle(rect,7.f);
  g.setColour(selected?juce::Colour(0xffbfff75):juce::Colour(0xff384451));
  g.drawRoundedRectangle(rect,7.f,1.f);
 }
 void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) override {
  g.setColour(b.getToggleState()?juce::Colour(0xff0d1117):juce::Colours::white);
  g.setFont(juce::FontOptions(13.f,juce::Font::bold));
  g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5),juce::Justification::centred,1);
 }
};
