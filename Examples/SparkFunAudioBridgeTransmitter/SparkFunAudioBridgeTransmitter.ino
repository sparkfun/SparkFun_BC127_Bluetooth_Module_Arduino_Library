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

String address = "20FABB0101CF"; // Remote module's address. If I were an optimist,
                                 //  I'd scan for BC127 modules and treat any one I
                                 //  found as the remote. Let's hard code for safety.

void setup()
{
  // Serial port configuration. The software port should be at 9600 baud, as that
  //  is the default speed for the BC127.
  Serial.begin(9600);
  swPort.begin(9600);
}

void loop()
{
  // Loop doesn't have to do much...just monitor the connection and try and restore
  //  it if it's lost.
  if (BTModu.connectionState() == BC127::CONNECT_ERROR)
  {
    // Blast the existing settings of the BC127 module, so I know that the module is
    //  set to factory defaults...
    BTModu.restore();
    
    // ...but, before I restart, I need to set the device to be a SOURCE, so it will
    //  enable its audio input and forward the data to the remote.
    BTModu.setClassicSource();
    
    // Write, reset, to commit and effect the change to a source.
    BTModu.writeConfig();
    BTModu.reset();
    
    // Now, attempt to connect. There are timeouts on these operations, so we won't
    //  sit forever.
    BTModu.connect(address, BC127::A2DP);
    BTModu.connect(address, BC127::AVRCP);
    // If we DID connect, we want to use the "PLAY" command to start the devices
    //  streaming audio. If we didn't, well, who cares? No harm in a spurious "PLAY".
    BTModu.musicCommands(BC127::PLAY);
  }
}

