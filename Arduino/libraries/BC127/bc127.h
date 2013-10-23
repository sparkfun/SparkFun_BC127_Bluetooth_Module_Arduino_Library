#ifndef BC127_h
#define BC127_h

#include <Arduino.h>
#include <SoftwareSerial.h>

// Let's make an enum for different types of connections. The BC127 module can
//  support a lot of different types of connections, but we'll only actually use
//  a few of them.
enum connType {SPP, BLE, A2DP, HFP, AVRCP};

// Now, make a data type for function results.
enum opResult {REMOTE_ERROR = -5, CONNECT_ERROR, INVALID_PARAM,
             TIMEOUT_ERROR, MODULE_ERROR, DEFAULT_ERR, SUCCESS};
/*
#define REMOTE_ERROR   -5
#define CONNECT_ERROR  -4
#define INVALID_PARAM  -3
#define TIMEOUT_ERROR  -2
#define MODULE_ERROR   -1
#define SUCCESS         1
*/

// Here's an enum for the various audio commands we can use on the module.
enum audioCmds {PLAY, PAUSE, FORWARD, BACK, UP, DOWN, STOP};

class BC127 
{
  public:
    BC127();
    BC127(Stream* sp);
    opResult reset();
    opResult restore();
    opResult writeConfig();
    opResult inquiry(int timeout);
    opResult connect(char index, connType connection);
    opResult connect(String address, connType connection);
    opResult getAddress(byte index, String *address);
    opResult exitDataMode();
    opResult exitDataMode(int guardDelay);
    opResult enterDataMode();
    opResult BLEDisable();
    opResult BLECentral();
    opResult BLEPeripheral();
    opResult BLEAdvertise();
    opResult BLENoAdvertise();
    opResult BLEScan(int timeout);
    opResult musicCommands(audioCmds command);
  private:
    int _baudRate;
    String _addresses[5];
    char _numAddresses;
    Stream *_serialPort;
    opResult knownStart();
    opResult stdCmd(String command);
    opResult stdGetParam(String command, String *param);
    opResult stdSetParam(String command, String param);
};


#endif