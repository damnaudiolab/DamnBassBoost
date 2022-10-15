#pragma once

#include <JuceHeader.h>

using namespace juce;

class SliderInitializer
{
    using sliderSettings = std::tuple<
        Slider::SliderStyle, 
        Slider::TextEntryBoxPosition,
        int, 
        int, 
        bool, 
        bool, 
        Justification::Flags>;

public:
    SliderInitializer() {}
    ~SliderInitializer() {}

    void setup(Slider::SliderStyle style, Slider::TextEntryBoxPosition textBoxPos, int textBoxWidth, int textBoxHeight, bool drawTextBoxOutline, bool onleft, Justification::Flags textBoxJustification)
    {
        sliderSettings settings = std::make_tuple(style, textBoxPos, textBoxWidth, textBoxHeight, drawTextBoxOutline, onleft, textBoxJustification);
        defaultSettings = std::make_unique<sliderSettings>(settings);
    }

    void init(Slider& target, Label& targetLabel, const char* name, const char* suffix, Rectangle<int> bounds, NotificationType notificationType = dontSendNotification)
    {
        target.setSliderStyle(std::get<sliderStyleIdx>(*defaultSettings.get()));

        target.setTextBoxStyle(
            std::get<textEntryBoxPositionIdx>(*defaultSettings.get()), 
            false, 
            std::get<textEntryBoxWidthIdx>(*defaultSettings.get()), 
            std::get<textEntryBoxHeightIdx>(*defaultSettings.get())
        );

        if (std::get<drawTextBoxOutlineIdx>(*defaultSettings.get()))
            target.setColour(Slider::textBoxOutlineColourId, Colours::black);

        if (strlen(suffix) > 0) 
            target.setTextValueSuffix(suffix);

        target.setBounds(bounds);

        targetLabel.setJustificationType(std::get<justificationIdx>(*defaultSettings.get()));

        targetLabel.attachToComponent(&target, std::get<onLeftIdx>(*defaultSettings.get()));

        targetLabel.setText(name, notificationType);
    }

private:
    /*
    Slider::SliderStyle defaultSliderStyle;
    Slider::TextEntryBoxPosition defaultTextEntryBoxPosition;
    int defaultTextEntryBoxWidth;
    int defaultTextEntryBoxHeight;
    String& defaultTextValueSuffix = String();
    Rectangle<int> defaultBounds = Rectangle<int>();
    bool defaultDrawTextBoxOutline;
    String& defaultLabelText = String();
    NotificationType defaultNotificationType;
    bool defaultOnLeft;
    Justification::Flags defaultJustification;
    */
   
    std::unique_ptr<sliderSettings> defaultSettings = nullptr;
    
    enum paramIndex
    {
        sliderStyleIdx,
        textEntryBoxPositionIdx,
        textEntryBoxWidthIdx,
        textEntryBoxHeightIdx,
        drawTextBoxOutlineIdx,
        onLeftIdx,
        justificationIdx,
    };
};