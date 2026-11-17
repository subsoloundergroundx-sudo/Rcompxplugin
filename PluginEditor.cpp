#include "PluginEditor.h"

namespace
{
    struct KnobDef { const char* paramId; const char* caption; const char* suffix; };

    const KnobDef knobDefs[] = {
        { "threshold", "THRESH",  " dB" },
        { "ratio",     "RATIO",   ":1"  },
        { "attack",    "ATTACK",  " ms" },
        { "release",   "RELEASE", " ms" },
        { "knee",      "KNEE",    " dB" },
        { "makeup",    "MAKEUP",  " dB" },
        { "mix",       "MIX",     " %"  },
    };

    juce::Font pixelFont (float size, bool bold = true)
    {
        return juce::Font (juce::FontOptions ("Consolas", size,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }
}

//==============================================================================
PixelLookAndFeel::PixelLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,       juce::Colour (pixel::frame));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (pixel::bg));
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colour (pixel::frame));
}

void PixelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAng, float endAng,
                                         juce::Slider&)
{
    const float px = 3.0f;   // one "pixel" of the 8-bit grid
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float s = juce::jmin (area.getWidth(), area.getHeight());
    auto body = juce::Rectangle<float> (s, s).withCentre (area.getCentre()).getSmallestIntegerContainer().toFloat();
    const auto c = body.getCentre();

    // Flat body with hard border
    g.setColour (juce::Colour (pixel::panel));
    g.fillRect (body);
    g.setColour (juce::Colour (pixel::frame));
    g.drawRect (body, 2.0f);

    // Stepped scale ring made of pixel squares
    const int steps = 11;
    const float r = s * 0.5f - 9.0f;
    for (int i = 0; i < steps; ++i)
    {
        const float t = (float) i / (float) (steps - 1);
        const float a = startAng + t * (endAng - startAng);
        const bool lit = t <= sliderPos + 0.0001f;

        juce::uint32 col = pixel::dim;
        if (lit)
            col = t < 0.6f ? pixel::green : (t < 0.85f ? pixel::yellow : pixel::red);

        const juce::Point<float> p (c.x + std::sin (a) * r, c.y - std::cos (a) * r);
        g.setColour (juce::Colour (col));
        g.fillRect (juce::Rectangle<float> (px * 1.5f, px * 1.5f).withCentre (p));
    }

    // Pointer: pixel dots from centre outwards
    const float a = startAng + sliderPos * (endAng - startAng);
    g.setColour (juce::Colour (pixel::frame));
    for (int k = 1; k <= 3; ++k)
    {
        const float d = (float) k * r * 0.28f;
        const juce::Point<float> p (c.x + std::sin (a) * d, c.y - std::cos (a) * d);
        g.fillRect (juce::Rectangle<float> (px, px).withCentre (p));
    }
}

//==============================================================================
RCompVisualizer::RCompVisualizer (RCompProcessor& p) : proc (p)
{
    startTimerHz (30);
}

void RCompVisualizer::drawSegmentBar (juce::Graphics& g, juce::Rectangle<float> area,
                                      float fraction, const juce::String& label)
{
    auto labelArea = area.removeFromLeft (40.0f);
    g.setColour (juce::Colour (pixel::frame));
    g.setFont (pixelFont (12.0f));
    g.drawText (label, labelArea, juce::Justification::centredLeft, false);

    const int segs = 28;
    const float segW = area.getWidth() / (float) segs;
    const int lit = juce::roundToInt (juce::jlimit (0.0f, 1.0f, fraction) * (float) segs);

    for (int i = 0; i < segs; ++i)
    {
        const float t = (float) i / (float) segs;
        juce::uint32 col = pixel::dim;
        if (i < lit)
            col = t < 0.6f ? pixel::green : (t < 0.85f ? pixel::yellow : pixel::red);

        g.setColour (juce::Colour (col));
        g.fillRect (area.getX() + (float) i * segW + 1.0f, area.getY(),
                    segW - 2.0f, area.getHeight());
    }
}

void RCompVisualizer::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll (juce::Colour (pixel::bg));

    // VU window frame
    auto frame = b.reduced (6.0f);
    g.setColour (juce::Colour (pixel::frame));
    g.drawRect (frame, 3.0f);

    auto inner = frame.reduced (8.0f);
    g.setColour (juce::Colour (0xff06070c));
    g.fillRect (inner);

    // Input waveform entering from the right, drawn as pixel columns
    auto wave = inner.removeFromTop (inner.getHeight() * 0.58f);
    const int N = RCompProcessor::scopeSize;
    const float colW = wave.getWidth() / (float) N;
    const float mid = std::floor (wave.getCentreY());
    const float half = wave.getHeight() * 0.5f;
    const int head = proc.scopeHead.load();

    // Centre line
    g.setColour (juce::Colour (pixel::dim));
    g.fillRect (wave.getX(), mid, wave.getWidth(), 1.0f);

    g.setColour (juce::Colour (pixel::green));
    for (int i = 0; i < N; ++i)
    {
        const int idx = (head + i) % N;   // oldest -> newest, left -> right
        const float v = juce::jlimit (-1.0f, 1.0f, proc.scope[(size_t) idx].load());
        const float yv = std::floor (mid - v * half);
        const float top = juce::jmin (mid, yv);
        const float h = juce::jmax (2.0f, std::abs (yv - mid));
        g.fillRect (std::floor (wave.getX() + (float) i * colW), top,
                    juce::jmax (1.0f, colW), h);
    }

    // VU ballistics from output RMS
    const float rms = proc.outputRms.load();
    const float db = juce::Decibels::gainToDecibels (rms, -60.0f);
    const float vuTarget = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
    vuSmooth = juce::jmax (vuTarget, vuSmooth * 0.85f);

    // Gain reduction fraction (0 dB -> 0, -24 dB -> 1)
    const float gr = juce::jlimit (0.0f, 1.0f, -proc.gainReductionDb.load() / 24.0f);

    auto rest = inner.reduced (0.0f, 4.0f);
    auto vuRow = rest.removeFromTop (rest.getHeight() * 0.5f).reduced (0.0f, 2.0f);
    auto grRow = rest.reduced (0.0f, 2.0f);

    drawSegmentBar (g, vuRow, vuSmooth, "VU");
    drawSegmentBar (g, grRow, gr, "GR");
}

//==============================================================================
RCompEditor::RCompEditor (RCompProcessor& p)
    : AudioProcessorEditor (&p), processor (p), visualizer (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (visualizer);

    for (int i = 0; i < numKnobs; ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
        k.setTextValueSuffix (knobDefs[i].suffix);
        addAndMakeVisible (k);

        labels[i].setText (knobDefs[i].caption, juce::dontSendNotification);
        labels[i].setJustificationType (juce::Justification::centred);
        labels[i].setColour (juce::Label::textColourId, juce::Colour (pixel::frame));
        labels[i].setFont (pixelFont (11.0f));
        addAndMakeVisible (labels[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                            (processor.apvts, knobDefs[i].paramId, k);
    }

    setSize (640, 420);
}

RCompEditor::~RCompEditor()
{
    setLookAndFeel (nullptr);
}

void RCompEditor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll (juce::Colour (pixel::bg));

    // Header bar: flat panel with a pixel underline
    auto header = b.removeFromTop (44.0f);
    g.setColour (juce::Colour (pixel::panel));
    g.fillRect (header);
    g.setColour (juce::Colour (pixel::green));
    g.fillRect (header.removeFromBottom (3.0f));

    g.setColour (juce::Colour (pixel::frame));
    g.setFont (pixelFont (20.0f));
    g.drawText ("RCOMP.X", header, juce::Justification::centred, false);
}

void RCompEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (44);

    visualizer.setBounds (area.removeFromTop (210).reduced (14, 10));

    auto knobRow = area.reduced (8, 4);
    const int cellW = knobRow.getWidth() / numKnobs;

    for (int i = 0; i < numKnobs; ++i)
    {
        auto cell = knobRow.removeFromLeft (cellW);
        labels[i].setBounds (cell.removeFromTop (18));
        knobs[i].setBounds (cell.reduced (2, 0));
    }
}
