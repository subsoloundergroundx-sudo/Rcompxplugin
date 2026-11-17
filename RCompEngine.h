#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Compressor DSP with Threshold, Ratio, Attack,
// Release, Knee and Makeup controls.
namespace rcomp
{
    // Static transfer function: gain change in dB (<= 0) for an input level in dB.
    inline float staticGainDb (float inDb, float thresholdDb, float ratio, float kneeDb)
    {
        const float slope = 1.0f / ratio - 1.0f;   // 0 at ratio 1, negative otherwise
        const float over  = inDb - thresholdDb;

        if (kneeDb <= 1.0e-4f)
            return over > 0.0f ? slope * over : 0.0f;

        if (2.0f * over < -kneeDb)  return 0.0f;
        if (2.0f * over >  kneeDb)  return slope * over;

        const float x = over + 0.5f * kneeDb;       // quadratic soft-knee region
        return slope * x * x / (2.0f * kneeDb);
    }

    class Engine
    {
    public:
        void prepare (double sr)
        {
            sampleRate = sr;
            envDb = -120.0f;
        }

        void setParams (float thresholdDb, float ratioValue, float attackMs,
                        float releaseMs, float kneeDb, float makeupDb)
        {
            threshold = thresholdDb;
            ratio     = juce::jmax (1.0f, ratioValue);
            knee      = kneeDb;
            makeupLin = juce::Decibels::decibelsToGain (makeupDb);

            const float sr = (float) sampleRate;
            attackCoef  = std::exp (-1.0f / (0.001f * juce::jmax (0.1f, attackMs)  * sr));
            releaseCoef = std::exp (-1.0f / (0.001f * juce::jmax (1.0f, releaseMs) * sr));
        }

        // Feed the linked (max of L/R) absolute sample value.
        void process (float absLevel)
        {
            const float levelDb = juce::Decibels::gainToDecibels (absLevel, -120.0f);
            const float coef    = levelDb > envDb ? attackCoef : releaseCoef;
            envDb = coef * envDb + (1.0f - coef) * levelDb;
            grDb  = staticGainDb (envDb, threshold, ratio, knee);
        }

        float gainLinear() const
        {
            return juce::Decibels::decibelsToGain (grDb) * makeupLin;
        }

        float grDb = 0.0f;   // current gain reduction in dB (<= 0)

    private:
        double sampleRate = 44100.0;
        float threshold = -18.0f, ratio = 4.0f, knee = 6.0f, makeupLin = 1.0f;
        float attackCoef = 0.0f, releaseCoef = 0.0f, envDb = -120.0f;
    };
}
