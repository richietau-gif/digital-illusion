#include "PluginEditor.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr int designWidth = 1100;
constexpr int designHeight = 700;

struct KnobSpec
{
    const char* id;
    const char* title;
    const char* suffix;
    int decimals;
};

constexpr std::array<KnobSpec, 17> knobs {{
    {"depth", "DUCK DEPTH", "", 2},
    {"mix", "DRY / WET", "", 2},
    {"threshold", "THRESHOLD", " dB", 1},
    {"length", "LENGTH", " ms", 0},
    {"timeOffset", "TIME OFFSET", "", 2},
    {"lookahead", "LOOKAHEAD", " ms", 0},
    {"attackProtection", "ATTACK PROTECT", "", 2},
    {"retrigger", "RETRIGGER", " ms", 0},
    {"smooth", "SMOOTHING", " ms", 1},
    {"cutoff", "CUTOFF", " Hz", 0},
    {"resonance", "RESONANCE", "", 2},
    {"filterDrive", "FILTER DRIVE", " dB", 1},
    {"filterMod", "FILTER ENV", "", 2},
    {"filterMix", "FILTER MIX", "", 2},
    {"colorDrive", "COLOR DRIVE", " dB", 1},
    {"colorMix", "COLOR MIX", "", 2},
    {"output", "OUTPUT", " dB", 1}
}};

constexpr std::array<const char*, 5> choiceTitles {{
    "TRIGGER", "DETECTOR", "RATE", "FILTER TYPE", "COLOR TYPE"
}};

float smoothstep(float x) noexcept
{
    x = juce::jlimit(0.0f, 1.0f, x);
    return x * x * (3.0f - 2.0f * x);
}
}

PrecisionLookAndFeel::PrecisionLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a2a3d));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff31495d));
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff55d9e9));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101c2a));
    setColour(juce::TextEditor::textColourId, juce::Colours::white);
    setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344e61));
    setColour(juce::ToggleButton::textColourId, juce::Colours::white);
}

void PrecisionLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float position, float startAngle, float endAngle,
                                           juce::Slider& slider)
{
    const float diameter = (float) juce::jmin(width, height);
    const float radius = diameter * 0.37f;
    const float cx = (float) x + (float) width * 0.5f;
    const float cy = (float) y + (float) height * 0.49f;
    const float angle = startAngle + position * (endAngle - startAngle);
    juce::Path rail, value;
    rail.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    value.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour(juce::Colour(0xff283a49));
    g.strokePath(rail, juce::PathStrokeType(4.5f));
    const auto accent = slider.getName() == "timeOffset" ? juce::Colour(0xffff9f42)
                                                          : juce::Colour(0xff48cedf);
    g.setColour(slider.isEnabled() ? accent : juce::Colour(0xff617480));
    g.strokePath(value, juce::PathStrokeType(4.5f));
    const float dx = std::sin(angle);
    const float dy = -std::cos(angle);
    g.setColour(juce::Colour(0xffe4f5f9));
    g.drawLine(cx + dx * radius * 0.48f, cy + dy * radius * 0.48f,
               cx + dx * radius * 0.82f, cy + dy * radius * 0.82f, 2.0f);
}

DISidechainEditor::DISidechainEditor(DISidechainProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lookAndFeel);

    for (const auto& spec : knobs)
    {
        auto slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                     juce::Slider::TextBoxBelow);
        slider->setName(spec.id);
        slider->setNumDecimalPlacesToDisplay(spec.decimals);
        slider->setTextValueSuffix(spec.suffix);
        const juce::String id(spec.id);
        if (id == "depth" || id == "mix" || id == "attackProtection"
            || id == "resonance" || id == "filterMix" || id == "colorMix"
            || id == "filterMod" || id == "timeOffset")
        {
            slider->textFromValueFunction = [](double value)
            {
                return juce::String(value * 100.0, 0) + " %";
            };
            slider->valueFromTextFunction = [](const juce::String& text)
            {
                return text.getDoubleValue() / 100.0;
            };
        }
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 18);
        slider->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe5eef6));
        slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0e1c2d));
        slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2e4356));
        slider->setTooltip(juce::String(spec.title));
        addAndMakeVisible(*slider);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.state, spec.id, *slider));
        sliders.push_back(std::move(slider));
    }

    struct Choice
    {
        const char* id;
        juce::StringArray options;
    };
    const std::array<Choice, 6> choices {{
        {"triggerMode", {"Audio", "MIDI", "Audio + MIDI"}},
        {"detector", {"Peak", "RMS", "Transient"}},
        {"division", {"1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar"}},
        {"filterModel", {"LP12", "LP24", "LP48", "HP12", "HP24", "HP48",
                          "Band Pass", "Notch", "Analog Ladder", "SVF Morph",
                          "Comb", "Formant / Vowel", "Aggressive Bass"}},
        {"colorModel", {"Warm Saturation", "Soft Clip", "Hard Clip",
                         "Asymmetric Drive", "Wavefold", "Digital Crush"}},
        {"uiScale", {"50%", "75%", "100%"}}
    }};

    for (const auto& choice : choices)
    {
        auto combo = std::make_unique<juce::ComboBox>();
        combo->addItemList(choice.options, 1);
        combo->setJustificationType(juce::Justification::centredLeft);
        combo->setTooltip(juce::String(choice.id));
        addAndMakeVisible(*combo);
        comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            proc.state, choice.id, *combo));
        combos.push_back(std::move(combo));
    }

    sync = std::make_unique<juce::ToggleButton>("BPM SYNC");
    bypass = std::make_unique<juce::ToggleButton>("BYPASS");
    addAndMakeVisible(*sync);
    addAndMakeVisible(*bypass);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.state, "sync", *sync);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.state, "bypass", *bypass);

    controlsReady = true;
    combos.back()->onChange = [this]
    {
        if (!controlsReady) return;
        const int index = combos.back()->getSelectedId();
        if (index >= 1 && index <= 3)
        {
            const int widths[] = {550, 825, 1100};
            const int heights[] = {350, 525, 700};
            setSize(widths[index - 1], heights[index - 1]);
        }
    };

    setResizable(true, true);
    setResizeLimits(550, 350, 1400, 900);
    setWindowScaleFromParameter();
    startTimerHz(20);
}

DISidechainEditor::~DISidechainEditor()
{
    stopTimer();
    controlsReady = false;
    // Destroy attachments before components and remove our custom look-and-feel.
    syncAttachment.reset();
    bypassAttachment.reset();
    sliderAttachments.clear();
    comboAttachments.clear();
    setLookAndFeel(nullptr);
}

void DISidechainEditor::setWindowScaleFromParameter()
{
    const auto* parameter = proc.state.getRawParameterValue("uiScale");
    const int selected = parameter != nullptr
        ? juce::jlimit(0, 2, juce::roundToInt(parameter->load())) : 1;
    const int widths[] = {550, 825, 1100};
    const int heights[] = {350, 525, 700};
    setSize(widths[selected], heights[selected]);
}

juce::Rectangle<int> DISidechainEditor::scaled(int x, int y, int w, int h) const
{
    const float factor = juce::jmin((float) getWidth() / designWidth,
                                    (float) getHeight() / designHeight);
    const float ox = ((float) getWidth() - designWidth * factor) * 0.5f;
    const float oy = ((float) getHeight() - designHeight * factor) * 0.5f;
    return {juce::roundToInt(ox + x * factor), juce::roundToInt(oy + y * factor),
            juce::roundToInt(w * factor), juce::roundToInt(h * factor)};
}

void DISidechainEditor::resized()
{
    if (!controlsReady || sliders.size() != knobs.size() || combos.size() != 6 || !sync || !bypass)
        return;

    bypass->setBounds(scaled(710, 15, 100, 33));
    sync->setBounds(scaled(810, 15, 115, 33));
    combos.back()->setBounds(scaled(934, 16, 140, 31));

    const int selectorWidth = 208;
    for (size_t i = 0; i < 5; ++i)
        combos[i]->setBounds(scaled(24 + (int) i * 211, 331, selectorWidth - 10, 29));

    for (size_t i = 0; i < sliders.size(); ++i)
    {
        const int column = (int) (i % 6);
        const int row = (int) (i / 6);
        sliders[i]->setBounds(scaled(29 + column * 178, 405 + row * 95, 153, 62));
    }
}

void DISidechainEditor::timerCallback()
{
    repaint(scaled(22, 64, 1056, 250));
    repaint(scaled(620, 16, 82, 35));
}

void DISidechainEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090f19));
    const float factor = juce::jmin((float) getWidth() / designWidth,
                                    (float) getHeight() / designHeight);
    const float ox = ((float) getWidth() - designWidth * factor) * 0.5f;
    const float oy = ((float) getHeight() - designHeight * factor) * 0.5f;
    g.addTransform(juce::AffineTransform::translation(ox, oy).scaled(factor));

    auto section = [&g](juce::Rectangle<float> r)
    {
        g.setColour(juce::Colour(0xff111d2b));
        g.fillRoundedRectangle(r, 9.0f);
        g.setColour(juce::Colour(0xff243649));
        g.drawRoundedRectangle(r, 9.0f, 1.0f);
    };
    g.setColour(juce::Colour(0xffe6f1f8));
    g.setFont(juce::Font(juce::FontOptions(23.0f).withStyle("Bold")));
    g.drawText("PRECISION SIDECHAIN", 24, 11, 460, 34, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff57cfe1));
    g.setFont(juce::FontOptions(10.5f));
    g.drawText("DIGITAL ILLUSION  /  PRECISION DUCK + FILTER", 27, 43, 520, 18,
               juce::Justification::centredLeft);

    const float activity = juce::jlimit(0.0f, 1.0f, proc.triggerActivity.load());
    const float peak = juce::jlimit(0.0f, 1.0f, proc.meter.load());
    g.setColour(juce::Colour(0xff33495a));
    g.fillRoundedRectangle(604.0f, 29.0f, 88.0f, 6.0f, 3.0f);
    g.setColour(juce::Colour(0xff57d9e9));
    g.fillRoundedRectangle(604.0f, 29.0f, 88.0f * peak, 6.0f, 3.0f);
    g.setColour(activity > 0.07f ? juce::Colour(0xffa8f76c) : juce::Colour(0xff354b51));
    g.fillEllipse(582.0f, 25.0f, 13.0f, 13.0f);

    section({22.0f, 72.0f, 1056.0f, 226.0f});
    g.setColour(juce::Colour(0xffd4e5f2));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("GAIN ENVELOPE  /  TRIGGER ALIGNED", 40, 83, 370, 20,
               juce::Justification::left);

    constexpr float graphLeft = 43.0f, graphRight = 1054.0f;
    constexpr float graphTop = 111.0f, graphBottom = 235.0f;
    g.setColour(juce::Colour(0xff243344));
    for (int i = 0; i <= 4; ++i)
    {
        const float x = graphLeft + (graphRight - graphLeft) * (float) i / 4.0f;
        const float y = graphTop + (graphBottom - graphTop) * (float) i / 4.0f;
        g.drawLine(x, graphTop, x, graphBottom, 0.7f);
        g.drawLine(graphLeft, y, graphRight, y, 0.7f);
    }

    const auto read = [this](const char* id, float fallback) -> float
    {
        const auto* v = proc.state.getRawParameterValue(id);
        return v != nullptr ? v->load() : fallback;
    };
    const float depth = juce::jlimit(0.0f, 1.0f, read("depth", 1.0f));
    const float offset = juce::jlimit(-1.0f, 1.0f, read("timeOffset", 0.0f));
    const float attack = juce::jlimit(0.0f, 1.0f, read("attackProtection", 0.0f));
    juce::Path curve;
    for (int i = 0; i <= 256; ++i)
    {
        const float phase = (float) i / 256.0f;
        const float envelope = smoothstep(phase + offset);
        const float protectedEnv = juce::jmax(envelope, attack * (1.0f - phase));
        const float gain = 1.0f - depth * (1.0f - protectedEnv);
        const float x = graphLeft + phase * (graphRight - graphLeft);
        const float y = graphBottom - gain * (graphBottom - graphTop);
        if (i == 0) curve.startNewSubPath(x, y);
        else curve.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xffff9b34));
    g.strokePath(curve, juce::PathStrokeType(3.2f));

    g.setColour(juce::Colour(0xff56d4e5));
    g.drawLine(graphLeft, graphTop, graphLeft, graphBottom, 1.5f);
    g.setColour(juce::Colour(0xffa6bccc));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("TRIGGER", 44, 239, 110, 18, juce::Justification::left);
    g.drawText("OFFSET " + juce::String(juce::roundToInt(offset * 100.0f)) + "%",
               807, 82, 240, 19, juce::Justification::right);

    const float cutoff = juce::jlimit(20.0f, 20000.0f, read("cutoff", 800.0f));
    const float cutoffX = graphLeft + (std::log(cutoff / 20.0f) / std::log(1000.0f))
                                        * (graphRight - graphLeft);
    g.setColour(juce::Colour(0xff1b3444));
    g.fillRoundedRectangle(graphLeft, 270.0f, graphRight - graphLeft, 8.0f, 4.0f);
    g.setColour(juce::Colour(0xff4ac7db));
    g.fillRoundedRectangle(graphLeft, 270.0f, juce::jmax(0.0f, cutoffX - graphLeft),
                           8.0f, 4.0f);
    g.setColour(juce::Colour(0xffc2d8e7));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("FILTER CUTOFF MARKER", 43, 280, 280, 14, juce::Justification::left);

    g.setColour(juce::Colour(0xffa0b8ca));
    g.setFont(juce::FontOptions(10.5f));
    for (size_t i = 0; i < choiceTitles.size(); ++i)
        g.drawText(choiceTitles[i], 28 + (int) i * 211, 309, 200, 18,
                   juce::Justification::left);

    section({22.0f, 367.0f, 1056.0f, 301.0f});
    const char* groupTitles[] = {"ENVELOPE / TIMING", "DETECTOR / FILTER",
                                 "FILTER / COLOR / OUTPUT"};
    for (int row = 0; row < 3; ++row)
    {
        const int baseY = 376 + row * 95;
        g.setColour(juce::Colour(0xff83a5bb));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(groupTitles[row], 39, baseY - 7, 450, 13, juce::Justification::left);
        for (int col = 0; col < 6; ++col)
        {
            const size_t index = (size_t) (row * 6 + col);
            if (index >= knobs.size()) break;
            g.setColour(juce::Colour(0xffd9e7f0));
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(knobs[index].title, 29 + col * 178, baseY + 7, 153, 16,
                       juce::Justification::centred);
        }
    }
    g.setColour(juce::Colour(0xff7f9bb0));
    g.setFont(juce::FontOptions(10.5f));
    g.drawText("VST3  /  v1.0 RC  •  SINGLE ENVELOPE", 28, 672, 680, 17,
               juce::Justification::left);
}
