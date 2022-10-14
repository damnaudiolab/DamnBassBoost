#include <JuceHeader.h>

#include "DamnBassBoost.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginAudioProcessor();
}