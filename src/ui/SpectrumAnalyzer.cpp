#include "SpectrumAnalyzer.h"
#include "../PluginProcessor.h"
#include <algorithm>

namespace ui {
namespace {

float frequencyToX(float frequency, juce::Rectangle<float> bounds, float maxFrequency) {
    constexpr float minFrequency = 20.0f;
    const float logMin = std::log10(minFrequency);
    const float logMax = std::log10(maxFrequency);
    const float normalized = (std::log10(frequency) - logMin) / juce::jmax(logMax - logMin, 1.0e-6f);
    return bounds.getX() + normalized * bounds.getWidth();
}

float dbToY(float db, juce::Rectangle<float> bounds, float minDb, float maxDb) {
    const float normalized = juce::jmap(db, maxDb, minDb, 0.0f, 1.0f);
    return bounds.getY() + normalized * bounds.getHeight();
}

} // namespace

SpectrumAnalyzer::SpectrumAnalyzer(EQInfinityAudioProcessor& processor)
    : processor_(processor), fft_(FFTOrder), window_(FFTSize, juce::dsp::WindowingFunction<float>::hann, true) {}

void SpectrumAnalyzer::update(double sampleRate, juce::Rectangle<float> plotBounds) {
    bounds_ = plotBounds;

    if (bounds_.getWidth() < 4.0f || bounds_.getHeight() < 4.0f || sampleRate <= 0.0)
        return;

    pullFromProcessor();
    buildPathFromHistory(preHistory_, preWritePosition_, preFftBuffer_, prePath_, sampleRate);
    buildPathFromHistory(postHistory_, postWritePosition_, postFftBuffer_, postPath_, sampleRate);
}

void SpectrumAnalyzer::draw(juce::Graphics& g) const {
    g.saveState();
    g.reduceClipRegion(bounds_.toNearestInt());

    g.setColour(juce::Colour::fromRGB(255, 164, 94).withAlpha(0.35f));
    g.strokePath(prePath_, juce::PathStrokeType(1.2f));

    g.setColour(juce::Colour::fromRGB(118, 227, 255).withAlpha(0.65f));
    g.strokePath(postPath_, juce::PathStrokeType(1.5f));

    g.restoreState();
}

void SpectrumAnalyzer::pullFromProcessor() {
    std::array<float, TempBlockSize> temp{};

    while (true) {
        const int numRead = processor_.popPreAnalyzerSamples(temp.data(), TempBlockSize);
        if (numRead <= 0)
            break;

        for (int i = 0; i < numRead; ++i) {
            preHistory_[static_cast<std::size_t>(preWritePosition_)] = temp[static_cast<std::size_t>(i)];
            preWritePosition_ = (preWritePosition_ + 1) % FFTSize;
        }
    }

    while (true) {
        const int numRead = processor_.popPostAnalyzerSamples(temp.data(), TempBlockSize);
        if (numRead <= 0)
            break;

        for (int i = 0; i < numRead; ++i) {
            postHistory_[static_cast<std::size_t>(postWritePosition_)] = temp[static_cast<std::size_t>(i)];
            postWritePosition_ = (postWritePosition_ + 1) % FFTSize;
        }
    }
}

void SpectrumAnalyzer::buildPathFromHistory(const std::array<float, FFTSize>& history, int writePosition,
                                            std::array<float, FFTSize * 2>& fftBuffer, juce::Path& path,
                                            double sampleRate) {
    for (int i = 0; i < FFTSize; ++i) {
        const int historyIndex = (writePosition + i) % FFTSize;
        fftBuffer[static_cast<std::size_t>(i)] = history[static_cast<std::size_t>(historyIndex)];
    }

    window_.multiplyWithWindowingTable(fftBuffer.data(), FFTSize);
    std::fill(fftBuffer.begin() + FFTSize, fftBuffer.end(), 0.0f);
    fft_.performFrequencyOnlyForwardTransform(fftBuffer.data());

    const float maxFrequency = static_cast<float>(juce::jmin(sampleRate * 0.495, 20000.0));
    path.clear();

    bool started = false;
    for (int bin = 1; bin < FFTSize / 2; ++bin) {
        const float frequency =
            (static_cast<float>(bin) * static_cast<float>(sampleRate)) / static_cast<float>(FFTSize);
        if (frequency < 20.0f || frequency > maxFrequency)
            continue;

        const float magnitude = fftBuffer[static_cast<std::size_t>(bin)] / static_cast<float>(FFTSize / 2);
        const float db = juce::jlimit(minDb_, maxDb_, juce::Decibels::gainToDecibels(magnitude, minDb_));

        const float x = frequencyToX(frequency, bounds_, maxFrequency);
        const float y = dbToY(db, bounds_, minDb_, maxDb_);

        if (!started) {
            path.startNewSubPath(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }
}

} // namespace ui
