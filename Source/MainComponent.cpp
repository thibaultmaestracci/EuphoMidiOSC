#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);
    //List Midi Devices
    static StringArray InputList = MidiInput::getDevices();
        for (int i = 0; i<InputList.size(); i++) {
            DBG("\r" + (String)i + " : "+ (String)InputList[i]);
        }
    
    
    //OSC
    oscReceiver.addListener (this);
    
        if (OSC_LISTEN_PORT > 0 && OSC_LISTEN_PORT < 65536)
        {
            if (oscSocket.bindToPort(OSC_LISTEN_PORT) && oscReceiver.connectToSocket(oscSocket))
            {
                
                DBG("Listen on "+ (String) OSC_LISTEN_PORT);
            }
            else DBG("NOT LISTENING on "+ (String) OSC_LISTEN_PORT);
        }
        else DBG("ERROR DEFINITION PORT "+ (String) OSC_LISTEN_PORT);
        
        
        
        
        sendOscMsg(OSCMessage("/xremote"));
        startTimer(10000);
        OSCMessage toto("/ch/01/mix/fader");
        toto.addString("-10");
        sendOscMsg(toto);
        
//        OSCMessage toto("node");
//        toto.addString("-show/showfile/chan16");
//        sendOscMsg(toto);
    
    
    
    //MIDI
    if(MIDI_DEVICE_INDEX>-1)
        {
        auto list = MidiInput::getDevices();
        auto newInput = list[MIDI_DEVICE_INDEX];
            DBG("MIDI DEVICE SELECTED : " + newInput);
        if (! deviceManager.isMidiInputEnabled (newInput))
            deviceManager.setMidiInputEnabled (newInput, true);
        
        deviceManager.addMidiInputCallback (newInput, this);
        midiOutput = MidiOutput::openDevice(MIDI_DEVICE_INDEX);
        }
    
    chMute.add(0);
    for (int i = 1 ; i<32; i++) {
        
        chMute.add(0);
        
        String toto = (i<10)?"0":"";
        sendOscMsg(OSCMessage("/ch/"+ toto +(String)i +"/mix/mute"));
    }
    
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    
    
    
    
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

bool MainComponent::sendOscMsg(juce::OSCMessage msg) {
   
    //DEBUG : on affiche les infos dans la console
    DBG("SENDING '" + msg.getAddressPattern().toString() + ((!msg.isEmpty()&& msg[0].isString())?("'  arg: '"+msg[0].getString()):"")  +  "'  TO " + targetIP + "@" + (String)OSC_LISTEN_PORT);
    
    
    //On vérifie que les infos d'envoi OSC sont bien remplies
    if(targetIP!="" && OSC_LISTEN_PORT>0 && OSC_LISTEN_PORT<65000){
    
        //On initialise l'objet OSCSender pour le préparer à l'envoie de message
        if (oscSender.connectToSocket(oscSocket,targetIP , OSC_LISTEN_PORT)){
            //on tente l'envoie
            if(! oscSender.send(msg)) {
                
                //En cas d'échec, on affiche un message d'erreur dans la console
                DBG("Error: could not send OSC message");
                return false;
        }
        else return true;
    }
        else {
            return false;
            DBG("Error: could not send OSC message NOT CONNECTED OK");
        }
    }
    else return false;
}

void MainComponent::oscMessageReceived(const OSCMessage &message)
{
    DBG("OscReceived : " + message.getAddressPattern().toString());
        if(!message.isEmpty() && message[0].isFloat32() ) DBG("with first arg FLOAT : " + (String)message[0].getFloat32());
        if(!message.isEmpty() && message[0].isInt32() ) DBG("with first arg INT : " + (String)message[0].getInt32());
        if(!message.isEmpty() && message[0].isString() ) DBG("with first arg STRING : " + message[0].getString());
    
    
    MessageManager::callAsync([=](){processOSCMessage(message);});
    //if(message.getAddressPattern().toString()=="/toto") SendMidi(MidiMessage::noteOn(1, 10, (float)0));
    //if(message.getAddressPattern().matches("fader")) DBG("OIUIIIIIIII");
}

void MainComponent::oscBundleReceived(const juce::OSCBundle &bundle) {
    
}


void MainComponent::processOSCMessage(OSCMessage message)
{
    StringArray AdressArray;
    AdressArray.addTokens(message.getAddressPattern().toString(), "/", "");
    
    DBG(AdressArray[2]);
    
    if(message.getAddressPattern().toString().contains("fader")) SendMidi(MidiMessage::pitchWheel(AdressArray[2].getIntValue(), (int)(message[0].getFloat32()*16368)));
    
    if(message.getAddressPattern().toString().contains("mute")) chMute.set(AdressArray[2].getIntValue(), (message[0].getInt32())?true:false);
}
void MainComponent::timerCallback() {
    sendOscMsg(OSCMessage("/xremote"));
}




// ******* MIDI PART *******


void MainComponent::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    
    addMessageToProcess (message, source->getName());
}

MainComponent::IncomingMessageCallback::IncomingMessageCallback(MainComponent* o, const MidiMessage& m, const String& s):owner (o), message (m), source (s)
{
    
}
void MainComponent::IncomingMessageCallback::messageCallback()
{
    
    if (owner != nullptr)
        owner->processMessage (message, source);
}

void MainComponent::addMessageToProcess(const juce::MidiMessage &message, const juce::String &source)
{
    (new IncomingMessageCallback (this, message, source))->post();
}
void MainComponent::processMessage(const juce::MidiMessage &message, const juce::String &source)
{
    
    MessageManager::callAsync([=](){ProcessMidiMessage(message);});
    
}

void MainComponent::ProcessMidiMessage(const juce::MidiMessage &message)
{
    DBG("MidiMessage Received : " + String::toHexString (message.getRawData(), message.getRawDataSize()));
    DBG("-> "+ message.getDescription());
    if(message.isNoteOn()) DBG("--> "+ (String)message.getNoteNumber());
    
 
    if(message.isNoteOn()&&message.getChannel()==1) {
        String valueToSend;
        switch (message.getNoteNumber()) {
            case 16:
                valueToSend = (chMute[1])?"ON":"OFF";
                DBG(valueToSend);
                sendOscMsg(OSCMessage("/ch/01/mix/on", valueToSend));
                break;
                
            default:
                break;
        }
        
    }
    
    if(message.isPitchWheel()){
        
        sendOscMsg(OSCMessage("/ch/0"+ (String)message.getChannel()+"/mix/fader", (float)message.getPitchWheelValue()/16368));
    }
    
//    for (int i = 0; i<midiTriggers.size(); i++) {
//        if((String::toHexString (message.getRawData(), message.getRawDataSize())==String::toHexString (midiTriggers[i].message.getRawData(), midiTriggers[i].message.getRawDataSize())) && midiTriggers[i].oscToSend.isNotEmpty())
//        {
//            DBG("Trigger Midi Detected. Trying to send : " + midiTriggers[i].oscToSend);
//            if (oscSenderMaster.connect (QLAB_MASTER_IP, QLAB_MASTER_PORT))
//                if(! oscSenderMaster.send(midiTriggers[i].oscToSend)) DBG("Error: could not send OSC message");
//
//            if (oscSenderSpare.connect (QLAB_SPARE_IP, QLAB_SPARE_PORT))
//                if(! oscSenderSpare.send(midiTriggers[i].oscToSend)) DBG("Error: could not send OSC message");
//
//        }
//    }
}

void MainComponent::SendMidi(juce::MidiMessage message)
{
    midiOutput->sendMessageNow(message);
    DBG("Trying to send Midi Message : " + String::toHexString (message.getRawData(), message.getRawDataSize()));
}
