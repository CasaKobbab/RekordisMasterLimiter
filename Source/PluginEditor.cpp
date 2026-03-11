#include "PluginProcessor.h"
#include "PluginEditor.h"

RekordisMasterLimiterAudioProcessorEditor::RekordisMasterLimiterAudioProcessorEditor (RekordisMasterLimiterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Configure Sliders
    auto setupSlider = [this](juce::Slider& slider, const juce::String& label) {
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentWhite);
        slider.setLookAndFeel(&customLookAndFeel);
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);
    };

    setupSlider(driveSlider, "Drive");
    setupSlider(thresholdSlider, "Threshold");
    setupSlider(warmthSlider, "Warmth");
    
    setupSlider(attackSlider, "Attack");
    setupSlider(releaseSlider, "Release");
    setupSlider(ceilingSlider, "Ceiling");
    setupSlider(makeupSlider, "Makeup");

    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "drive", driveSlider);
    thresholdAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "threshold", thresholdSlider);
    attackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "attack", attackSlider);
    releaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "release", releaseSlider);
    warmthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "warmth", warmthSlider);
    ceilingAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "ceiling", ceilingSlider);
    makeupAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "makeup_gain", makeupSlider);

    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::Background_png, BinaryData::Background_pngSize);

    setSize (660, 420);
    startTimerHz(60); 
}

RekordisMasterLimiterAudioProcessorEditor::~RekordisMasterLimiterAudioProcessorEditor()
{
    driveSlider.setLookAndFeel(nullptr);
    thresholdSlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    releaseSlider.setLookAndFeel(nullptr);
    warmthSlider.setLookAndFeel(nullptr);
    ceilingSlider.setLookAndFeel(nullptr);
    makeupSlider.setLookAndFeel(nullptr);
}

void RekordisMasterLimiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw the high-res Obsidian Metal Background
    if (backgroundImage.isValid()) {
        g.drawImage(backgroundImage, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    } else {
        g.fillAll(juce::Colours::black);
    }

    // Title
    g.setColour (juce::Colour(0xffe2c08d)); // Gold
    g.setFont (30.0f);
    g.drawFittedText ("REKORDIS", getLocalBounds().removeFromTop(60).withTrimmedBottom(10), juce::Justification::centred, 1);
    g.setFont (12.0f);
    g.setColour (juce::Colour(0xffa09070));
    g.drawFittedText ("MASTER LIMITER", getLocalBounds().removeFromTop(80).withTrimmedTop(50), juce::Justification::centred, 1);

    // Labels
    g.setColour(juce::Colour(0xffa09070));
    g.setFont(12.0f);
    
    // Draw labels exactly 15 pixels above the top of each slider bounds
    auto drawLabel = [&](juce::Component& comp, const juce::String& text) {
        auto bounds = comp.getBounds();
        auto labelBounds = juce::Rectangle<int>(bounds.getX(), bounds.getY() - 25, bounds.getWidth(), 20);
        g.drawFittedText(text, labelBounds, juce::Justification::centred, 1);
    };

    drawLabel(driveSlider, "DRIVE");
    drawLabel(thresholdSlider, "THRESHOLD");
    drawLabel(warmthSlider, "WARMTH");
    
    drawLabel(attackSlider, "ATTACK");
    drawLabel(releaseSlider, "RELEASE");
    drawLabel(ceilingSlider, "CEILING");
    drawLabel(makeupSlider, "MAKEUP");

    // VU Meter Section
    auto vuBounds = juce::Rectangle<int>(440, 120, 180, 160);
    
    // Outer Bezel
    g.setColour(juce::Colour(0xff111111).withAlpha(0.8f));
    g.fillRoundedRectangle(vuBounds.toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff222222));
    g.drawRoundedRectangle(vuBounds.toFloat(), 6.0f, 2.0f);
    
    // Meter Screen
    auto screenBounds = vuBounds.reduced(8);
    // Amber glow gradient
    juce::ColourGradient screenGrad (juce::Colour(0xff3f280a), screenBounds.getX(), screenBounds.getY(), 
                                     juce::Colour(0xff110800), screenBounds.getX(), screenBounds.getBottom(), false);
    g.setGradientFill(screenGrad);
    g.fillRoundedRectangle(screenBounds.toFloat(), 3.0f);
    
    // Screen inner shadow / vignette
    juce::ColourGradient vignette(juce::Colours::transparentBlack, screenBounds.getCentreX(), screenBounds.getCentreY(), juce::Colours::black.withAlpha(0.8f), screenBounds.getX(), screenBounds.getY(), true);
    g.setGradientFill(vignette);
    g.fillRoundedRectangle(screenBounds.toFloat(), 3.0f);

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRoundedRectangle(screenBounds.toFloat(), 3.0f, 1.5f);
    
    // Meter Scale Markings
    g.setColour(juce::Colour(0xffe2c08d).withAlpha(0.6f));
    for (int i = 0; i <= 24; i += 3) {
        float ang = juce::jmap((float)-i, -24.0f, 0.0f, -1.0f, 1.0f);
        juce::Point<float> centerLine(screenBounds.getCentreX(), screenBounds.getBottom() - 15);
        float len = 80.0f;
        float tickLen = (i % 6 == 0) ? 10.0f : 5.0f; // longer tick every 6 dB
        
        juce::Point<float> p1(centerLine.x + len * std::sin(ang), centerLine.y - len * std::cos(ang));
        juce::Point<float> p2(centerLine.x + (len - tickLen) * std::sin(ang), centerLine.y - (len - tickLen) * std::cos(ang));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.5f);
        
        if (i % 6 == 0) {
            g.setFont(10.0f);
            juce::Point<float> pT(centerLine.x + (len - 20) * std::sin(ang), centerLine.y - (len - 20) * std::cos(ang));
            g.drawText(juce::String(i), pT.x - 10, pT.y - 12, 20, 20, juce::Justification::centred);
        }
    }
    
    // VU Needle
    float targetGR = audioProcessor.gainReductionLevel.load();
    // smooth needle physics
    currentGR = currentGR * 0.7f + targetGR * 0.3f;

    float needleAngle = juce::jmap(currentGR, -24.0f, 0.0f, -1.0f, 1.0f); // map reduction to radians
    // Clamp needle angle to prevent it flipping out of bounds
    needleAngle = juce::jlimit(-1.05f, 1.05f, needleAngle);
    
    juce::Point<float> centerLine(screenBounds.getCentreX(), screenBounds.getBottom() - 10);
    float length = 90.0f;
    juce::Point<float> endPoint(centerLine.x + length * std::sin(needleAngle), 
                                centerLine.y - length * std::cos(needleAngle));
                                
    // Needle shadow
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine(centerLine.x + 2, centerLine.y + 2, endPoint.x + 2, endPoint.y + 2, 2.5f);
                                
    // Glow of the needle
    g.setColour(juce::Colour(0xffff4422)); 
    juce::Line<float> needle(centerLine, endPoint);
    g.drawLine(needle, 2.0f);
    
    // Glass Reflection over the VU meter
    juce::ColourGradient glassGrad(juce::Colours::white.withAlpha(0.15f), screenBounds.getX(), screenBounds.getY(), juce::Colours::transparentWhite, screenBounds.getX(), screenBounds.getY() + screenBounds.getHeight() * 0.5f, false);
    g.setGradientFill(glassGrad);
    g.fillRoundedRectangle(screenBounds.withHeight(screenBounds.getHeight() * 0.5f).toFloat(), 3.0f);

    // Base of the needle (drawn over glass reflection for depth)
    g.setColour(juce::Colour(0xff111111));
    g.fillEllipse(centerLine.x - 8, centerLine.y - 8, 16, 16);
    g.setColour(juce::Colour(0xff333333));
    g.drawEllipse(centerLine.x - 8, centerLine.y - 8, 16, 16, 1.0f);
}

void RekordisMasterLimiterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().withTrimmedTop(140).withTrimmedLeft(40); // Increased top offset to give room for labels
    int knobSize = 85; // This specifies width, height will be knob + textbox (~120px)
    int sliderHeight = 110; 
    int spacing = 35; 

    auto row1 = area.removeFromTop(140);
    driveSlider.setBounds(row1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));
    row1.removeFromLeft(spacing);
    thresholdSlider.setBounds(row1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));
    row1.removeFromLeft(spacing);
    warmthSlider.setBounds(row1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));

    auto row2 = area.removeFromTop(140);
    attackSlider.setBounds(row2.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));
    row2.removeFromLeft(spacing);
    releaseSlider.setBounds(row2.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));
    row2.removeFromLeft(spacing);
    ceilingSlider.setBounds(row2.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));
    row2.removeFromLeft(spacing);
    makeupSlider.setBounds(row2.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, sliderHeight));
}

void RekordisMasterLimiterAudioProcessorEditor::timerCallback()
{
    // Repaint VU meter area bounds to prevent full window repaint
    repaint(juce::Rectangle<int>(440, 120, 180, 160)); 
}
