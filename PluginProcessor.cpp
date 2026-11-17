#include "PluginProcessor.h"
#include "PluginEditor.h"

RCompProcessor::RCompProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RCompProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "threshold", 1 }, "Threshold",
        NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "ratio", 1 }, "Ratio",
        NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.5f), 4.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "attack", 1 }, "Attack",
        NormalisableRange<float> (0.1f, 200.0f, 0.01f, 0.4f), 10.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "release", 1 }, "Release",
        NormalisableRange<float> (5.0f, 2000.0f, 0.1f, 0.35f), 150.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "knee", 1 }, "Knee",
        NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "makeup", 1 }, "Makeup",
        NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    return { params.begin(), params.end() };
}

void RCompProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    scopeCounter = 0;
}

void RCompProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    engine.setParams (get ("threshold"), get ("ratio"), get ("attack"),
                      get ("release"), get ("knee"), get ("makeup"));
    const float mix = get ("mix") * 0.01f;

    const int numSamples = buffer.getNumSamples();
    const bool stereo = buffer.getNumChannels() > 1;
    auto* L = buffer.getWritePointer (0);
    auto* R = stereo ? buffer.getWritePointer (1) : nullptr;

    float grMin = 0.0f;
    double sumSq = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dl = L[i];
        const float dr = stereo ? R[i] : dl;

        // Scope: one decimated mono sample every 64 input samples
        if (++scopeCounter >= 64)
        {
            scopeCounter = 0;
            const int h = scopeHead.load();
            scope[(size_t) h].store (0.5f * (dl + dr));
            scopeHead.store ((h + 1) % scopeSize);
        }

        engine.process (juce::jmax (std::abs (dl), std::abs (dr)));
        const float g = engine.gainLinear();
        grMin = juce::jmin (grMin, engine.grDb);

        L[i] = dl + (dl * g - dl) * mix;
        sumSq += (double) L[i] * L[i];
        if (stereo)
        {
            R[i] = dr + (dr * g - dr) * mix;
            sumSq += (double) R[i] * R[i];
        }
    }

    gainReductionDb.store (grMin);
    const int channels = stereo ? 2 : 1;
    outputRms.store (numSamples > 0
                         ? (float) std::sqrt (sumSq / (double) (numSamples * channels))
                         : 0.0f);
}

juce::AudioProcessorEditor* RCompProcessor::createEditor()
{
    return new RCompEditor (*this);
}

void RCompProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RCompProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Entry point used by juce_add_plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RCompProcessor();
}
