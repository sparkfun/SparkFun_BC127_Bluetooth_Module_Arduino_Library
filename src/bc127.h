/****************************************************************
Core header file for BC127 module support

This code is beerware; if you use it, please buy me (or any other
SparkFun employee) a cold beverage next time you run into one of
us at the local.

238 Jan 2014- Mike Hord, SparkFun Electronics

Code developed in Arduino 1.0.5, on an Arduino Pro Mini 5V.
****************************************************************/

#ifndef BC127_h
#define BC127_h

#include <Arduino.h>
#include <SoftwareSerial.h>


class BC127 
{
  public:
    // Let's make an enum for different types of connections. The 
    // BC127 module can support a lot of different types of connections
    // but we'll only actually use a few of them.
    enum connType {SPP, BLE, A2DP, HFP, AVRCP, PBAP, ANY};

    // Now, make a data type for function results.
    enum opResult {REMOTE_ERROR = -5, CONNECT_ERROR, INVALID_PARAM,
                 TIMEOUT_ERROR, MODULE_ERROR, DEFAULT_ERR, SUCCESS};

    // enum for the various audio commands we can use on the module.
    enum audioCmds {PLAY, PAUSE, FORWARD, BACK, UP, DOWN, STOP};
    
    // enum for the various valid baud rates. Rather than doing it
    //  with strings, we'll use the enum, to discourage people from 
    //  experimenting with out-of-bounds speeds.
    enum baudRates {s9600bps, s19200bps, s38400bps, 
                    s57600bps, s115200bps};
    
    BC127(Stream* sp);
    opResult reset();
    opResult restore();
    opResult writeConfig();
    opResult inquiry(int timeout);
    opResult connect(char index, connType connection);
    opResult connect(String address, connType connection);
    opResult getAddress(char index, String &address);
    opResult exitDataMode(int guardDelay=420);
    opResult enterDataMode();
    opResult BLEDisable();
    opResult BLECentral();
    opResult BLEPeripheral();
    opResult BLEAdvertise();
    opResult BLENoAdvertise();
    opResult BLEScan(int timeout);
    opResult setBaudRate(baudRates newSpeed);
    opResult musicCommands(audioCmds command);
    opResult addressQuery(String &address);
    opResult setClassicSink();
    opResult setClassicSource();
    opResult stdGetParam(String command, String *param);
    opResult stdSetParam(String command, String param);
    opResult stdCmd(String command);
    opResult connectionState();
  private:
    BC127();
    int _baudRate;
    String _addresses[5];
    char _numAddresses;
    Stream *_serialPort;
    opResult knownStart();
};


#endif

