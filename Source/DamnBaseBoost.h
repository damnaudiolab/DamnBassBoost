/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

  name:             DamnBaseBoost

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

using namespace juce;

//==============================================================================
class PluginAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================
    PluginAudioProcessor()
        : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo())
            .withOutput("Output", AudioChannelSet::stereo())
        ), parameters(*this, nullptr, Identifier("damnbaseboost"),
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
                    NormalisableRange<float>(1.0f, 20.0f, 0.01f, 0.5f),
                    5.0f),

                std::make_unique<AudioParameterFloat>(
                    "boostFreq",
                    "BoostFreq",
                    NormalisableRange<float>(20.0f, 300.0f, 0.05f, 0.5f),
                    60.0f),

                std::make_unique<AudioParameterFloat>(
                    "boostLevel",
                    "BoostLevel",
                    NormalisableRange<float>(1.0f, 10.0f, 0.01f),
                    1.0f),

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
        boostLevel = parameters.getRawParameterValue("boostLevel");
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
        boostLpf.setDrive(*boostLevel);

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
    const String getName() const override { return "DamnBaseBoost"; }
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

            preGainSliderAttachment.reset(new SliderAttachment(valueTreeState, "preGain", preGainSlider));
            preGainSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(preGainSlider);

            speedSliderAttachment.reset(new SliderAttachment(valueTreeState, "speed", speedSlider));
            speedSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(speedSlider);

            ratioSliderAttachment.reset(new SliderAttachment(valueTreeState, "ratio", ratioSlider));
            ratioSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(ratioSlider);

            boostFreqSliderAttachment.reset(new SliderAttachment(valueTreeState, "boostFreq", boostFreqSlider));
            boostFreqSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(boostFreqSlider);

            boostLevelSliderAttachment.reset(new SliderAttachment(valueTreeState, "boostLevel", boostLevelSlider));
            boostLevelSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(boostLevelSlider);

            amountSliderAttachment.reset(new SliderAttachment(valueTreeState, "amount", amountSlider));
            amountSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(amountSlider);

            postGainSliderAttachment.reset(new SliderAttachment(valueTreeState, "postGain", postGainSlider));
            postGainSlider.setSliderStyle(Slider::RotaryVerticalDrag);
            addAndMakeVisible(postGainSlider);

            setSize(width, height);
        }

        ~PluginAudioProcessorEditor() override {};

        void paint(Graphics& g) override
        {
            g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
        }

        void resized() override
        {

        }

    private:
        int width = 640;
        int height = 240;

        PluginAudioProcessor& audioProcessor;

        typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

        AudioProcessorValueTreeState& valueTreeState;

        /*
        Slider _Slider;
        std::unique_ptr<SliderAttachment> _SliderAttachment;
        */

        Slider preGainSlider;
        std::unique_ptr<SliderAttachment> preGainSliderAttachment;
        Slider speedSlider;
        std::unique_ptr<SliderAttachment> speedSliderAttachment;
        Slider ratioSlider;
        std::unique_ptr<SliderAttachment> ratioSliderAttachment;
        Slider boostFreqSlider;
        std::unique_ptr<SliderAttachment> boostFreqSliderAttachment;
        Slider boostLevelSlider;
        std::unique_ptr<SliderAttachment> boostLevelSliderAttachment;
        Slider amountSlider;
        std::unique_ptr<SliderAttachment> amountSliderAttachment;
        Slider postGainSlider;
        std::unique_ptr<SliderAttachment> postGainSliderAttachment;

        //Rectangle<int> Area{ 0, 0, width, height };
        Rectangle<int> preGainArea{ 0, 0, width / 7, height };
        Rectangle<int> speedArea{ width / 7, 0, width * 2 / 7, height };
        Rectangle<int> ratioArea{ width * 2 / 7, 0, width * 3 / 7, height };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessorEditor)
    };

    AudioProcessorValueTreeState parameters;

    std::atomic<float>* preGain = nullptr;
    std::atomic<float>* speed = nullptr;
    std::atomic<float>* ratio = nullptr;
    std::atomic<float>* boostFreq = nullptr;
    std::atomic<float>* boostLevel = nullptr;
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
