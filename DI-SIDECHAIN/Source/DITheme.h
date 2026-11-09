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
  const auto area=juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height);
  const float radius=juce::jmax(8.f,juce::jmin(area.getWidth(),area.getHeight())*.38f);
  const auto c=area.getCentre().translated(0.f,-1.f);
  const float a=start+pos*(end-start);
  juce::Colour accent=slider.findColour(juce::Slider::rotarySliderFillColourId);
  if(accent.isTransparent())accent=cyan();
  g.setColour(juce::Colours::black.withAlpha(.55f));
  g.fillEllipse(c.x-radius+1.f,c.y-radius+4.f,radius*2,radius*2);
  juce::ColourGradient face(juce::Colour(0xff344955),c.x-radius,c.y-radius,
                            juce::Colour(0xff07111a),c.x+radius,c.y+radius,true);
  g.setGradientFill(face);g.fillEllipse(c.x-radius,c.y-radius,radius*2,radius*2);
  g.setColour(juce::Colour(0xff476578));g.drawEllipse(c.x-radius,c.y-radius,radius*2,radius*2,1.2f);
  juce::Path track;track.addCentredArc(c.x,c.y,radius+3.f,radius+3.f,0.f,start,end,true);
  g.setColour(juce::Colour(0xff28414c));g.strokePath(track,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  juce::Path active;active.addCentredArc(c.x,c.y,radius+3.f,radius+3.f,0.f,start,a,true);
  g.setColour(accent.withAlpha(.17f));g.strokePath(active,juce::PathStrokeType(10.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(accent);g.strokePath(active,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  const float needleRadius=radius*.62f;
  const float ix=c.x+std::sin(a)*needleRadius,iy=c.y-std::cos(a)*needleRadius;
  juce::Path needle;needle.startNewSubPath(c);needle.lineTo(ix,iy);
  g.setColour(juce::Colour(0xffe6f8ff));g.strokePath(needle,juce::PathStrokeType(2.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(juce::Colour(0xffd9f6fd).withAlpha(.35f));
  g.drawEllipse(c.x-radius*.73f,c.y-radius*.75f,radius*1.46f,radius*.85f,1.f);
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