#pragma once

#include <JuceHeader.h>

using namespace juce;

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel() {}
    ~CustomLookAndFeel() {}

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        Slider&) override
    {

    };
};