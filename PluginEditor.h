#pragma once

#include "PluginProcessor.h"

// 8-bit look: flat colours, hard pixel edges, no gradients or shadows.
namespace pixel
{
    constexpr juce::uint32 bg      = 0xff0d0d14;
    constexpr juce::uint32 panel   = 0xff1a1d2b;
    constexpr juce::uint32 frame   = 0xffe8e8e8;
    constexpr juce::uint32 dim     = 0xff2c3142;
    constexpr juce::uint32 green   = 0xff5cff5c;
    constexpr juce::uint32 yellow  = 0xffffe03d;
    constexpr juce::uint32 red     = 0xffff4040;
}

class PixelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PixelLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

// VU window with the input waveform scrolling in from the right,
// plus segmented VU and gain-reduction bars.
class RCompVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit RCompVisualizer (RCompProcessor& p);

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }
    void drawSegmentBar (juce::Graphics& g, juce::Rectangle<float> area,
                         float fraction, const juce::String& label);

    RCompProcessor& proc;
    float vuSmooth = 0.0f;
};

class RCompEditor : public juce::AudioProcessorEditor
{
public:
    explicit RCompEditor (RCompProcessor&);
    ~RCompEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RCompProcessor& processor;
    PixelLookAndFeel laf;
    RCompVisualizer visualizer;

    static constexpr int numKnobs = 7;
    juce::Slider knobs[numKnobs];
    juce::Label labels[numKnobs];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[numKnobs];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RCompEditor)
};
