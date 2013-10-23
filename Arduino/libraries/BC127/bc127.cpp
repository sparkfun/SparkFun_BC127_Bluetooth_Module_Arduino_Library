#include "bc127.h"
#include <Arduino.h>

// Constructor. All we really need to do is link the user's Stream instance to
//  our local reference.
BC127::BC127(Stream *sp)
{
  _serialPort = sp;
}

// There are several commands that look for either OK or ERROR; let's abstract
//  support for those commands to one single private function, to save memory.
opResult BC127::stdCmd(String command)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();
  
  _serialPort->println(command);
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // This is our timeout loop. We'll give the module 3 seconds.
  while (resetStart + 3000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Similar to the command function, let's do a set parameter genrealization.
opResult BC127::stdSetParam(String command, String param)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();
  
  _serialPort->print("SET ");
  _serialPort->print(command);
  _serialPort->print("=");
  _serialPort->println(param);
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to reset.
  while (resetStart + 2000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Also, do a get paramater generalization. This is, of course, a bit more
//  difficult; we need to return both the result (SUCCESS/ERROR) and the
//  string returned.
opResult BC127::stdGetParam(String command, String *param)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();
  
  _serialPort->print("GET ");
  _serialPort->println(command);
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to reset.
  while (resetStart + 2000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("OK")) return SUCCESS;
      if (buffer.startsWith(command))
      {
        *param = buffer.substring(command.length());
        (*param).trim();
      }
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// The BLE role of the device is important: it can be either Central, Peripheral,
//   or disabled. We've provided one function for each of these. Note that to
//   get a change of mode to "take", a write/reset cycle is required.
opResult BC127::BLEDisable()
{
  return stdSetParam("BLE_ROLE", "0");
}

opResult BC127::BLECentral()
{
  return stdSetParam("BLE_ROLE", "2");
}

opResult BC127::BLEPeripheral()
{
  return stdSetParam("BLE_ROLE", "1");
}

// Issue the "RESTORE" command over the serial port to the BC127. This will
//  reset the device to factory default settings, which is a good thing to do
//  once in a while.
opResult BC127::restore()
{
  return stdCmd("RESTORE");
}

// Issue the "WRITE" command over the serial port to the BC127. This will
//  save the current settings to NVM, so they will be applied after a reset
//  or power cycle.
opResult BC127::writeConfig()
{
  return stdCmd("WRITE");
}

// Issue the "RESET" command over the serial port to the BC127. If it works, 
//  we expect to see a string that looks something like this:
//    BlueCreation Copyright 2013
//    Melody Audio V5.0 RC9
//    Ready
// If there is some sort of error, the module will respond with
//    ERROR
// We'll buffer characters until we see an EOL (\n\r), then check the string.
opResult BC127::reset()
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();
  
  // Now issue the reset command.
  _serialPort->println("RESET");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to reset.
  while (resetStart + 2000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("Re")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Create a known state for the module to start from. If a partial command is
//  already in the module's buffer, we can purge it by sending an EOL to the
//  the module. If not, we'll just get an error.
opResult BC127::knownStart()
{
  String EOL = String("\n\r");
  String buffer = "";
  
  _serialPort->print(EOL);
  _serialPort->flush();
  
  while (buffer.endsWith(EOL) != true)
  {
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));
  }
  if (buffer.startsWith("ERR")) return SUCCESS;
  else return SUCCESS;
}