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
  g.setColour(juce::Colours::black.withAlpha(.3f));
  g.fillEllipse(c.x-radius+1.2f,c.y-radius+2.4f,2.f*radius,2.f*radius);
  g.setColour(juce::Colour(0xff101e29));
  g.fillEllipse(c.x-radius,c.y-radius,2.f*radius,2.f*radius);
  g.setColour(juce::Colour(0xff4b6779));
  g.drawEllipse(c.x-radius,c.y-radius,2.f*radius,2.f*radius,1.f);
  juce::Path track;
  track.addCentredArc(c.x,c.y,radius+4.f,radius+4.f,0.f,start,end,true);
  g.setColour(juce::Colour(0xff304454));
  g.strokePath(track,juce::PathStrokeType(2.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  juce::Path progress;
  progress.addCentredArc(c.x,c.y,radius+4.f,radius+4.f,0.f,start,angle,true);
  g.setColour(accent);
  g.strokePath(progress,juce::PathStrokeType(3.4f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  const float length=radius*.68f;
  const auto tip=c.translated(std::sin(angle)*length,-std::cos(angle)*length);
  juce::Path pointer;pointer.startNewSubPath(c);pointer.lineTo(tip);
  g.setColour(juce::Colour(0xffe7f7ff));
  g.strokePath(pointer,juce::PathStrokeType(2.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(accent);
  g.fillEllipse(c.x-2.f,c.y-2.f,4.f,4.f);
 }
 void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool over,bool down) override {
  const auto r=button.getLocalBounds().toFloat().reduced(1.5f);
  const bool active=button.getToggleState();
  const auto fill=active?lime():down?juce::Colour(0xff234a5e):over?juce::Colour(0xff17394b):juce::Colour(0xff102632);
  if(active){g.setColour(lime().withAlpha(.18f));g.fillRoundedRectangle(r.expanded(1.f),8.f);}
  juce::ColourGradient grad(fill.brighter(active?.1f:.08f),r.getX(),r.getY(),
                            fill.darker(.36f),r.getRight(),r.getBottom(),false);
  g.setGradientFill(grad);g.fillRoundedRectangle(r,7.f);
  g.setColour(active?lime():juce::Colour(0xff386277));
  g.drawRoundedRectangle(r,7.f,1.f);
 }
 void drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool) override {
  g.setColour(button.getToggleState()?juce::Colour(0xff06131b):juce::Colours::white);
  g.setFont(juce::FontOptions(13.f,juce::Font::bold));
  g.drawFittedText(button.getButtonText(),button.getLocalBounds().reduced(5),juce::Justification::centred,1);
 }
};