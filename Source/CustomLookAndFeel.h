#pragma once

#include <JuceHeader.h>

using namespace juce;

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel() {}
    ~CustomLookAndFeel() {}

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
        Slider&) override
    {
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto lineW = jmin(8.0f, radius * 0.5f);
        auto knobRadius = radius - lineW * 2;
        auto arcRadius = radius - lineW * 0.5f;

        auto outlineColour = colourPalette[black];
        auto valueFillColour = colourPalette[green];
        auto knobFillColour = colourPalette[white];
        auto thumbColour = colourPalette[black];

        //bool sliderPolarity = (slider.getRange().getStart() / slider.getRange().getEnd()) == -1.0;

        Path backgroundArc;
        backgroundArc.addCentredArc(
            bounds.getCentreX(),
            bounds.getCentreY(),
            arcRadius,
            arcRadius,
            0.0f,
            rotaryStartAngle,
            rotaryEndAngle,
            true);
        g.setColour(outlineColour);
        g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

        Path valueArc;
        valueArc.addCentredArc(
            bounds.getCentreX(),
            bounds.getCentreY(),
            arcRadius,
            arcRadius,
            0.0f,
            rotaryStartAngle,
            toAngle,
            true);
        g.setColour(valueFillColour);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

        g.setColour(knobFillColour);
        g.fillEllipse(
            bounds.getCentreX() - knobRadius,
            bounds.getCentreY() - knobRadius,
            knobRadius * 2,
            knobRadius * 2);

        auto pointerDistance = knobRadius - lineW;
        auto pointerLength = pointerDistance * 0.33f;
        Path pointer;
        pointer.startNewSubPath(0.0f, -pointerLength);
        pointer.lineTo(0.0f, -pointerDistance);
        pointer.closeSubPath();
        pointer.applyTransform(AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));
        g.setColour(thumbColour);
        g.strokePath(pointer, PathStrokeType(lineW * 0.5f, PathStrokeType::curved, PathStrokeType::rounded));

    }
    
    enum colourIdx
    {
        grey,
        black,
        green,
        white,
    };

    Array<Colour> colourPalette = {
        Colour(0xff2a2e33),
        Colour(0xff1e2326),
        Colour(0xff00cc99),
        Colour(0xfff0f8ff),
    };

private:
    //54626f,36454f,00cc99,f0f8ff,91a3b0
};