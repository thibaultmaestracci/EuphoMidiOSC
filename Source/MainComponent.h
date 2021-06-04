#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component,
                       private MidiInputCallback,
                       private OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>,
                       private Timer

{
    
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DatagramSocket oscSocket;
    
    OSCReceiver oscReceiver;
    OSCSender oscSender;

    AudioDeviceManager deviceManager;
    //MidiOutput *midiOutput;
    std::unique_ptr<MidiOutput> midiOutput;
    
    String targetIP = "192.168.42.221";
    int OSC_LISTEN_PORT = 10023;
    int MIDI_DEVICE_INDEX = 1;
    
    Array<bool> chOn;
    Array<bool> chSolo;
    
    //OSC
    bool sendOscMsg(OSCMessage msg);
    void oscMessageReceived (const OSCMessage& message) override;
    void oscBundleReceived (const OSCBundle& bundle) override;
    
    void processOSCMessage(OSCMessage message);
    
    //Timer
    void timerCallback() override;
    
    //MIDI
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void addMessageToProcess(const MidiMessage& message, const String& source);
    void processMessage (const MidiMessage& message, const String& source);
    void ProcessMidiMessage(const MidiMessage& message);
    void SendMidi(MidiMessage message);
    class IncomingMessageCallback   : public CallbackMessage
    {
    public:
        IncomingMessageCallback (MainComponent* o, const MidiMessage& m, const String& s);
        
        void messageCallback() override;
        
        MainComponent* owner;
        MidiMessage message;
        String source;
    };
    
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
