#include <JuceHeader.h>

#include "DamnBaseBoost.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginAudioProcessor();
}