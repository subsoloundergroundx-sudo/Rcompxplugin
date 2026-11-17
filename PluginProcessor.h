#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "RCompEngine.h"

class RCompProcessor : public juce::AudioProcessor
{
public:
    RCompProcessor();
    ~RCompProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
    }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "rcomp.x"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Shared with the editor (read-only from the GUI side)
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<float> outputRms { 0.0f };

    // Decimated input waveform ring buffer for the scope
    static constexpr int scopeSize = 256;
    std::array<std::atomic<float>, scopeSize> scope {};
    std::atomic<int> scopeHead { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rcomp::Engine engine;
    int scopeCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RCompProcessor)
};
