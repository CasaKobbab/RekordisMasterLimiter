#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Custom LookAndFeel for that Epic Mystery Retro vibe
class ObsidianLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ObsidianLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour(0xffe2c08d)); // Gold text
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, juce::Colour(0xff554433));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Knob Shadow
        g.setColour (juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse (rx + 3, ry + 3, rw, rw);

        // Knob Body (Dark metallic gradient)
        juce::ColourGradient grad (juce::Colour(0xff2a2a2c), centreX, ry, juce::Colour(0xff0c0c0e), centreX, ry + rw, false);
        g.setGradientFill (grad);
        g.fillEllipse (rx, ry, rw, rw);

        // Knob Outline
        g.setColour (juce::Colour(0xff3a3a3d));
        g.drawEllipse (rx, ry, rw, rw, 1.5f);

        // Indicator (Gold/Amber)
        juce::Path p;
        auto pointerLength = radius * 0.35f;
        auto pointerThickness = 3.0f;
        p.addRoundedRectangle (-pointerThickness * 0.5f, -radius + 3.0f, pointerThickness, pointerLength, 1.5f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        g.setColour (juce::Colour(0xffe2c08d));
        g.fillPath (p);
    }
};

class RekordisMasterLimiterAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    RekordisMasterLimiterAudioProcessorEditor (RekordisMasterLimiterAudioProcessor&);
    ~RekordisMasterLimiterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    RekordisMasterLimiterAudioProcessor& audioProcessor;
    ObsidianLookAndFeel customLookAndFeel;

    juce::Slider driveSlider;
    juce::Slider thresholdSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider warmthSlider;
    juce::Slider ceilingSlider;
    juce::Slider makeupSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ceilingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupAttach;

    juce::Image backgroundImage;
    float currentGR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RekordisMasterLimiterAudioProcessorEditor)
};
