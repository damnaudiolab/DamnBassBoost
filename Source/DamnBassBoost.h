/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

  name:             DamnBassBoost

  dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_plugin_client, 
                    juce_audio_processors, juce_audio_utils, juce_core, juce_data_structures, juce_dsp, 
                    juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
  exporters:        VS2022, XCODE_MAC

  moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

  type:             AudioProcessor
  mainClass:        PluginAudioProcessor

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include "CustomLookAndFeel.h"
#include "ParameterUtil.h"

using namespace juce;

//==============================================================================
class PluginAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================
    PluginAudioProcessor()
        : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo())
            .withOutput("Output", AudioChannelSet::stereo())
        ), parameters(*this, nullptr, Identifier("damnbassboost"),
            {
                std::make_unique<AudioParameterFloat>(
                    "preGain",
                    "PreGain",
                    NormalisableRange<float>(-48.0f, 12.0f, 0.01f, 2.0f),
                    0.0f),

                std::make_unique<AudioParameterFloat>(
                    "speed",
                    "Speed",
                    NormalisableRange<float>(100.0f, 2500.0f, 0.1f, 0.7f),
                    1000.0f),

                std::make_unique<AudioParameterFloat>(
                    "ratio",
                    "Ratio",
                    NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f),
                    5.0f),

                std::make_unique<AudioParameterFloat>(
                    "boostFreq",
                    "BoostFreq",
                    NormalisableRange<float>(20.0f, 300.0f, 0.01f, 0.5f),
                    60.0f),

                std::make_unique<AudioParameterFloat>(
                    "boostDrive",
                    "BoostDrive",
                    NormalisableRange<float>(0.0f, 12.0f, 0.01f),
                    0.0f),

                std::make_unique<AudioParameterFloat>(
                    "amount",
                    "Amount",
                    NormalisableRange<float>(0.0f, 100.0f, 0.01f),
                    50.0f),

                std::make_unique<AudioParameterFloat>(
                    "postGain",
                    "PostGain",
                    NormalisableRange<float>(-48.0f, 12.0f, 0.01f, 2.0f),
                    0.0f),
            }
        )
    {
        preGain = parameters.getRawParameterValue("preGain");
        speed = parameters.getRawParameterValue("speed");
        ratio = parameters.getRawParameterValue("ratio");
        boostFreq = parameters.getRawParameterValue("boostFreq");
        boostDrive = parameters.getRawParameterValue("boostDrive");
        amount = parameters.getRawParameterValue("amount");
        postGain = parameters.getRawParameterValue("postGain");
    }

    ~PluginAudioProcessor() override
    {
    }

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;
        spec.sampleRate = sampleRate;

        preAmp.prepare(spec);
        processors.prepare(spec);
        mixDryWet.prepare(spec);
        postAmp.prepare(spec);

        processors.get<preCompIndex>().setThreshold(-120.0f);

        auto& boostLpf = processors.get<boostLpfIndex>();
        boostLpf.setMode(dsp::LadderFilterMode::LPF12);
        boostLpf.setResonance(0.7f);

        mixDryWet.setMixingRule(dsp::DryWetMixingRule::linear);
        mixDryWet.setWetMixProportion(0.5f);
    }

    void releaseResources() override
    {
        // When playback stops, you can use this as an opportunity to free up any
        // spare memory, etc.
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        ScopedNoDenormals noDenormals;
        auto totalNumInputChannels = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        // In case we have more outputs than inputs, this code clears any output
        // channels that didn't contain input data, (because these aren't
        // guaranteed to be empty - they may contain garbage).
        // This is here to avoid people getting screaming feedback
        // when they first compile a plugin, but obviously you don't need to keep
        // this code if your algorithm always overwrites all the output channels.
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());

        // This is the place where you'd normally do the guts of your plugin's
        // audio processing...
        // Make sure to reset the state if your inner loop is processing
        // the samples and the outer loop is handling the channels.
        // Alternatively, you can process the samples with the channels
        // interleaved by keeping the same state.

        preAmp.setGainDecibels(*preGain);

        auto& preComp = processors.get<preCompIndex>();

        preComp.setAttack(*speed);
        preComp.setRelease(*speed);
        preComp.setRatio(*ratio);

        auto& boostLpf = processors.get<boostLpfIndex>();
        boostLpf.setCutoffFrequencyHz(*boostFreq);
        float boostGainDecibels = *boostDrive;
        boostLpf.setDrive(Decibels::decibelsToGain(boostGainDecibels));

        auto& boostAmp = processors.get<boostAmpIndex>();
        boostAmp.setGainLinear(*amount);

        postAmp.setGainDecibels(*postGain + 6.0f);

        dsp::AudioBlock<float> audioBlock(buffer);

        dsp::ProcessContextReplacing<float> context(audioBlock);

        preAmp.process(context);
        mixDryWet.pushDrySamples(context.getInputBlock());
        processors.process(context);
        mixDryWet.mixWetSamples(context.getOutputBlock());
        postAmp.process(context);
    }

    //==============================================================================
    AudioProcessorEditor* createEditor() override {
        return new PluginAudioProcessorEditor(*this, parameters);
    }

    bool hasEditor() const override { return true; }

    //==============================================================================
    const String getName() const override { return "DamnBassBoost"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}

    //==============================================================================
    void getStateInformation(MemoryBlock& destData) override
    {
        // You should use this method to store your parameters in the memory block.
        // You could do that either as raw data, or use the XML or ValueTree classes
        // as intermediaries to make it easy to save and load complex data.
        auto state = parameters.copyState();
        std::unique_ptr<XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        // You should use this method to restore your parameters from this memory block,
        // whose contents will have been created by the getStateInformation() call.
        std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName(parameters.state.getType()))
                parameters.replaceState(ValueTree::fromXml(*xmlState));
    }

    //==============================================================================
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        // This is the place where you check if the layout is supported.
        // In this template code we only support mono or stereo.
        if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
            return false;

        // This checks if the input layout matches the output layout
        if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
            return false;

        return true;
    }

private:
    class PluginAudioProcessorEditor : public AudioProcessorEditor
    {
    public:
        PluginAudioProcessorEditor(PluginAudioProcessor& p,
            AudioProcessorValueTreeState& vts)
            : AudioProcessorEditor(&p), audioProcessor(p), valueTreeState(vts)
        {
            /*
            _SliderAttachment.reset(new SliderAttachment(valueTreeState, "", _Slider));
            //_Slider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(_Slider);
            */
            
            sliderInitializer.setup(
                Slider::RotaryVerticalDrag,
                Slider::TextBoxBelow,
                knobLabelWidth,
                knobLabelHeight,
                false,
                false,
                Justification::centred
            );

            preGainSliderAttachment.reset(new SliderAttachment(valueTreeState, "preGain", preGainSlider));
            sliderInitializer.init(preGainSlider, preGainSliderLabel, "Input", "dB", preGainArea.reduced(knobSpacing));
            addAndMakeVisible(preGainSlider);
            addAndMakeVisible(preGainSliderLabel);

            speedSliderAttachment.reset(new SliderAttachment(valueTreeState, "speed", speedSlider));
            sliderInitializer.init(speedSlider, speedSliderLabel, "Speed", "ms", speedArea.reduced(knobSpacing));
            addAndMakeVisible(speedSlider);
            addAndMakeVisible(speedSliderLabel);

            ratioSliderAttachment.reset(new SliderAttachment(valueTreeState, "ratio", ratioSlider));
            sliderInitializer.init(ratioSlider, ratioSliderLabel, "Ratio", " : 1", ratioArea.reduced(knobSpacing));
            addAndMakeVisible(ratioSlider);
            addAndMakeVisible(ratioSliderLabel);

            boostFreqSliderAttachment.reset(new SliderAttachment(valueTreeState, "boostFreq", boostFreqSlider));
            sliderInitializer.init(boostFreqSlider, boostFreqSliderLabel, "Freq", "Hz", boostFreqArea.reduced(knobSpacing));
            addAndMakeVisible(boostFreqSlider);
            addAndMakeVisible(boostFreqSliderLabel);

            boostDriveSliderAttachment.reset(new SliderAttachment(valueTreeState, "boostDrive", boostDriveSlider));
            sliderInitializer.init(boostDriveSlider, boostDriveSliderLabel, "Drive", "dB", boostDriveArea.reduced(knobSpacing));
            addAndMakeVisible(boostDriveSlider);
            addAndMakeVisible(boostDriveSliderLabel);

            amountSliderAttachment.reset(new SliderAttachment(valueTreeState, "amount", amountSlider));
            sliderInitializer.init(amountSlider, amountSliderLabel, "Amount", " %", amountArea.reduced(knobSpacing));
            addAndMakeVisible(amountSlider);
            addAndMakeVisible(amountSliderLabel);

            postGainSliderAttachment.reset(new SliderAttachment(valueTreeState, "postGain", postGainSlider));
            sliderInitializer.init(postGainSlider, postGainSliderLabel, "Output", "dB", postGainArea.reduced(knobSpacing));
            addAndMakeVisible(postGainSlider);
            addAndMakeVisible(postGainSliderLabel);

            /*
            speedSliderAttachment.reset(new SliderAttachment(valueTreeState, "speed", speedSlider));
            speedSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            speedSlider.setTextBoxStyle(Slider::TextBoxBelow, false, knobLabelWidth, knobLabelHeight);
            speedSlider.setTextValueSuffix(" ms");
            speedSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            speedSlider.setBounds(speedArea.reduced(knobSpacing));
            addAndMakeVisible(speedSlider);
            speedSliderLabel.setText("Speed", dontSendNotification);
            speedSliderLabel.attachToComponent(&speedSlider, false);
            speedSliderLabel.setJustificationType(Justification::centred);
            addAndMakeVisible(speedSliderLabel);

            ratioSliderAttachment.reset(new SliderAttachment(valueTreeState, "ratio", ratioSlider));
            ratioSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            ratioSlider.setTextBoxStyle(Slider::TextBoxBelow, false, knobLabelWidth, knobLabelHeight);
            ratioSlider.setTextValueSuffix(" : 1");
            ratioSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            ratioSlider.setBounds(ratioArea.reduced(knobSpacing));
            addAndMakeVisible(ratioSlider);
            ratioSliderLabel.setText("Ratio", dontSendNotification);
            ratioSliderLabel.attachToComponent(&ratioSlider, false);
            ratioSliderLabel.setJustificationType(Justification::centred);
            addAndMakeVisible(ratioSliderLabel);

            boostFreqSliderAttachment.reset(new SliderAttachment(valueTreeState, "boostFreq", boostFreqSlider));
            boostFreqSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            boostFreqSlider.setTextBoxStyle(Slider::TextBoxBelow, false, knobLabelWidth, knobLabelHeight);
            boostFreqSlider.setTextValueSuffix(" Hz");
            boostFreqSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            boostFreqSlider.setBounds(boostFreqArea.reduced(knobSpacing));
            addAndMakeVisible(boostFreqSlider);
            boostFreqSliderLabel.setText("Frequency", dontSendNotification);
            boostFreqSliderLabel.attachToComponent(&boostFreqSlider, false);
            boostFreqSliderLabel.setJustificationType(Justification::centred);
            addAndMakeVisible(boostFreqSliderLabel);

            boostDriveSliderAttachment.reset(new SliderAttachment(valueTreeState, "boostDrive", boostDriveSlider));
            boostDriveSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            boostDriveSlider.setTextBoxStyle(Slider::TextBoxBelow, false, knobLabelWidth, knobLabelHeight);
            boostDriveSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            boostDriveSlider.setBounds(boostDriveArea.reduced(knobSpacing));
            addAndMakeVisible(boostDriveSlider);
            boostDriveSliderLabel.setText("Drive", dontSendNotification);
            boostDriveSliderLabel.attachToComponent(&boostDriveSlider, false);
            boostDriveSliderLabel.setJustificationType(Justification::centred);
            addAndMakeVisible(boostDriveSliderLabel);

            amountSliderAttachment.reset(new SliderAttachment(valueTreeState, "amount", amountSlider));
            amountSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            amountSlider.setTextBoxStyle(Slider::TextBoxBelow, false, knobLabelWidth, knobLabelHeight);
            amountSlider.setTextValueSuffix(" %");
            amountSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            amountSlider.setBounds(amountArea.reduced(knobSpacing));
            addAndMakeVisible(amountSlider);
            amountSliderLabel.setText("Amount", dontSendNotification);
            amountSliderLabel.attachToComponent(&amountSlider, false);
            amountSliderLabel.setJustificationType(Justification::centred);
            addAndMakeVisible(amountSliderLabel);

            postGainSliderAttachment.reset(new SliderAttachment(valueTreeState, "postGain", postGainSlider));
            postGainSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            postGainSlider.setTextBoxStyle(Slider::TextBoxBelow, false, knobLabelWidth, knobLabelHeight);
            postGainSlider.setTextValueSuffix(" dB");
            postGainSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            postGainSlider.setBounds(postGainArea.reduced(knobSpacing));
            addAndMakeVisible(postGainSlider);
            postGainSliderLabel.setText("Output", dontSendNotification);
            postGainSliderLabel.attachToComponent(&postGainSlider, false);
            postGainSliderLabel.setJustificationType(Justification::centred);
            addAndMakeVisible(postGainSliderLabel);
            */

            logo = Drawable::createFromImageData(BinaryData::logo_svg, BinaryData::logo_svgSize);
            addAndMakeVisible(logo.get());
            logo->setTransformToFit(headerArea.reduced(20).toFloat(), RectanglePlacement::centred);

            setSize(width, height);
        }

        ~PluginAudioProcessorEditor() override {};

        void paint(Graphics& g) override
        {
            g.fillAll(customLookAndFeel.colourPalette[CustomLookAndFeel::grey]);
            g.setColour(customLookAndFeel.colourPalette[CustomLookAndFeel::black]);
            g.fillRect(headerArea);
        }

        void resized() override
        {

        }

    private:
        CustomLookAndFeel customLookAndFeel;

        SliderInitializer sliderInitializer;

        int width = 800;
        int height = 270;

        int knobWidth = width / 7;
        int knobLabelWidth = width / 10;
        int knobLabelHeight = height / 10;
        int knobHeight = height * 3 / 5 - knobLabelHeight;
        int knobPosY = height - knobHeight;
        int headerHeight = knobPosY - knobLabelHeight;
        int knobSpacing = (knobWidth / 20);

        PluginAudioProcessor& audioProcessor;

        typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

        AudioProcessorValueTreeState& valueTreeState;

        /*
        Slider _Slider;
        std::unique_ptr<SliderAttachment> _SliderAttachment;
        */

        Slider preGainSlider;
        Label preGainSliderLabel;
        std::unique_ptr<SliderAttachment> preGainSliderAttachment;
        Slider speedSlider;
        Label speedSliderLabel;
        std::unique_ptr<SliderAttachment> speedSliderAttachment;
        Slider ratioSlider;
        Label ratioSliderLabel;
        std::unique_ptr<SliderAttachment> ratioSliderAttachment;
        Slider boostFreqSlider;
        Label boostFreqSliderLabel;
        std::unique_ptr<SliderAttachment> boostFreqSliderAttachment;
        Slider boostDriveSlider;
        Label boostDriveSliderLabel;
        std::unique_ptr<SliderAttachment> boostDriveSliderAttachment;
        Slider amountSlider;
        Label amountSliderLabel;
        std::unique_ptr<SliderAttachment> amountSliderAttachment;
        Slider postGainSlider;
        Label postGainSliderLabel;
        std::unique_ptr<SliderAttachment> postGainSliderAttachment;

        std::unique_ptr<Drawable> logo;

        //Rectangle<int> Area{ 0, 0, width, height };
        Rectangle<int> headerArea{ 0, 0, width, headerHeight };

        Rectangle<int> preGainArea{ 0, knobPosY, knobWidth, knobHeight };
        Rectangle<int> speedArea{ knobWidth, knobPosY, knobWidth, knobHeight };
        Rectangle<int> ratioArea{ knobWidth * 2, knobPosY, knobWidth, knobHeight };
        Rectangle<int> boostFreqArea{ knobWidth * 3, knobPosY, knobWidth, knobHeight };
        Rectangle<int> boostDriveArea{ knobWidth * 4, knobPosY, knobWidth, knobHeight };
        Rectangle<int> amountArea{ knobWidth * 5, knobPosY, knobWidth, knobHeight };
        Rectangle<int> postGainArea{ knobWidth * 6, knobPosY, knobWidth, knobHeight };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessorEditor)
    };

    AudioProcessorValueTreeState parameters;

    std::atomic<float>* preGain = nullptr;
    std::atomic<float>* speed = nullptr;
    std::atomic<float>* ratio = nullptr;
    std::atomic<float>* boostFreq = nullptr;
    std::atomic<float>* boostDrive = nullptr;
    std::atomic<float>* amount = nullptr;
    std::atomic<float>* postGain = nullptr;
    
    enum
    {
        preCompIndex,
        boostLpfIndex,
        boostAmpIndex,
    };

    dsp::Gain<float> preAmp;

    dsp::ProcessorChain<
        dsp::Compressor<float>,
        dsp::LadderFilter<float>,
        dsp::Gain<float>
    > processors;

    dsp::DryWetMixer<float> mixDryWet;

    dsp::Gain<float> postAmp;

    dsp::ProcessSpec spec;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginAudioProcessor)
};
