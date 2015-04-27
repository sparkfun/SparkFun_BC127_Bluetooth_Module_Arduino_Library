/****************************************************************
Example code demonstrating the use of the Arduino Library for
the BlueCreation BC127 Bluetooth Module, used on the SparkFun
BC127 Breakout and Purpletooth Jamboree boards.

This code is beerware; if you use it, please buy me (or any other
SparkFun employee) a cold beverage next time you run into one of
us at the local.

20 Nov 2013- Mike Hord, SparkFun Electronics

Code developed in Arduino 1.0.5, on an Arduino Pro Mini 3.3V.
****************************************************************/

// Include the two libraries we need to use this; I'm using a software serial port
//  because time-sharing the hardware port with uploading code is a pain.
#include <SparkFunbc127.h>
#include <SoftwareSerial.h>

// Create a software serial port.
SoftwareSerial swPort(11,10);  // RX, TX
// Create a BC127 and attach the software serial port to it.
BC127 BTModu(&swPort);

void setup()
{
  // Serial port configuration. The software port should be at 9600 baud, as that
  //  is the default speed for the BC127.
  Serial.begin(9600);
  swPort.begin(9600);
}

// buffer *should* be a static within loop(), but Arduino freaks out about that,
//  so I'm making it a global. Le sigh.
String buffer = "";

void loop()
{
  // resetFlag tracks the state of the module. When true, the module is reset and
  //  ready to receive a connection. When false, the module either has an active
  //  connection or has lost its connection; checking connectionState() will verify
  //  which of those conditions we're in.
  static boolean resetFlag = false;
  
  // This is where we determine the state of the module. If resetFlag is false, and
  //  we have a CONNECT_ERROR, we need to restart the module to clear its pairing
  //  list so it can accept another connection.
  if (BTModu.connectionState() == BC127::CONNECT_ERROR  && !resetFlag)
  {  
    Serial.println("Connection lost! Resetting...");
    // Blast the existing settings of the BC127 module, so I know that the module is
    //  set to factory defaults...
    BTModu.restore();
    
    // ...but, before I restart, I need to set the device to be a SINK, so it will
    //  enable its audio output and dump the audio to the output.
    BTModu.setClassicSink();
    
    // Write, reset, to commit and effect the change to a source.
    BTModu.writeConfig();
    // Repeat the connection process if we've lost our connection.
    BTModu.reset();
    // Change the resetFlag, so we know we've restored the module to a usable state.
    resetFlag = true;
  }
  // If we ARE connected, we'll issue the "PLAY" command. Note that issuing this when
  //  we are already playing doesn't hurt anything.
  else BTModu.musicCommands(BC127::PLAY);
  
  // We want to look for a connection to be made to the module; once a connection
  //  has been made, we can clear the resetFlag.
  if (BTModu.connectionState() == BC127::SUCCESS) resetFlag = false;
}

