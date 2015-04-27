/****************************************************************
Core management functions for BC127 modules.

This code is beerware; if you use it, please buy me (or any other
SparkFun employee) a cold beverage next time you run into one of
us at the local.

238 Jan 2014- Mike Hord, SparkFun Electronics

Code developed in Arduino 1.0.5, on an Arduino Pro Mini 5V.
****************************************************************/

#include "SparkFunbc127.h"
#include <Arduino.h>

// Constructor. All we really need to do is link the user's Stream instance to
//  our local reference.
BC127::BC127(Stream *sp)
{
  _serialPort = sp;
  _numAddresses = -1;
}

// It may be useful to know the address of this module. This function will
//  pack it into a string for you.
BC127::opResult BC127::addressQuery(String &address)
{
  return stdGetParam("LOCAL_ADDR", &address);
}
  

// We need a baud rate setting handler. Let's make one! This is kinda tricksy,
//  though, b/c the baud rate change takes effect immediately, so the return
//  string from the baud rate change will be garbled. This will result in a
//  TIMEOUT_ERROR from that function (after all, the EOL won't be recognized,
//  since it'll be at the wrong baud rate). So, if we get a TIMEOUT_ERROR from
//  the module, we'll re-do the command at the new baud rate. If *that* fails.
//  we can assume something is radically wrong. 
BC127::opResult BC127::setBaudRate(baudRates newSpeed)
{
  // The BC127 wants a string, but the .begin() functions want an integer. We
  //  need a variable to hold both types.
  int intSpeed;
  String stringSpeed;
  
  // Convert our enum values into strings the module can use.
  switch(newSpeed)
  {
    case BC127::s9600bps:
      stringSpeed = "9600";
      break;
    case BC127::s19200bps:
      stringSpeed = "19200";
      break;
    case BC127::s38400bps:
      stringSpeed = "38400";
      break;
    case BC127::s57600bps:
      stringSpeed = "57600";
      break;
    case BC127::s115200bps:
      stringSpeed = "115200";
      break;
    default:
      return INVALID_PARAM;
  }
  
  // If we've made it to here, we had a valid input to the function and should
  //  send it off to the set parameter function.
  intSpeed = stringSpeed.toInt();  
  
  // So, there are three possibilities here: SUCCESS, MODULE_ERROR, and
  //  TIMEOUT_ERROR. SUCCESS indicates that you just set the baud rate to the
  //  same baud rate which is currently in use; MODULE_ERROR indicates that
  //  something weird happened to the string before it was sent (honestly, after
  //  I'm done hammering the dents out, I can't imagine that coming up), and
  //  TIMEOUT_ERROR indicates one of two things: an actual timeout, OR success
  //  but we couldn't read it b/c the baud rate was broken. Since we're using
  //  the inheritance of the Stream class to manipulate our serial ports, we
  //  can't change the baud rate. The user should probably just interpret 
  //  TIMEOUT_ERROR as success, and call it good.
  return stdSetParam("BAUD", stringSpeed);
}

// There are several commands that look for either OK or ERROR; let's abstract
//  support for those commands to one single private function, to save memory.
BC127::opResult BC127::stdCmd(String command)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart(); // Clear the serial buffer in the module and the Arduino.
  
  _serialPort->print(command);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
    
  // This is our timeout loop. We'll give the module 3 seconds.
  while ((startTime + 3000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() > 0) buffer.concat(char(_serialPort->read()));

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
BC127::opResult BC127::stdSetParam(String command, String param)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();  // Clear Arduino and module serial buffers.
  
  _serialPort->print("SET ");
  _serialPort->print(command);
  _serialPort->print("=");
  _serialPort->print(param);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to reset.
  while ((startTime + 2000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      else if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Also, do a get paramater generalization. This is, of course, a bit more
//  difficult; we need to return both the result (SUCCESS/ERROR) and the
//  string returned.
BC127::opResult BC127::stdGetParam(String command, String *param)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();  // Clear the serial buffers.
  
  _serialPort->print("GET ");
  _serialPort->print(command);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the get command. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to get the value.
  while (loopStart + 2000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("OK")) return SUCCESS;
      if (buffer.startsWith(command))
      {
        (*param) = buffer.substring(command.length()+1);
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
BC127::opResult BC127::BLEDisable()
{
  return stdSetParam("BLE_ROLE", "0");
}

BC127::opResult BC127::BLECentral()
{
  return stdSetParam("BLE_ROLE", "2");
}

BC127::opResult BC127::BLEPeripheral()
{
  return stdSetParam("BLE_ROLE", "1");
}

// Issue the "RESTORE" command over the serial port to the BC127. This will
//  reset the device to factory default settings, which is a good thing to do
//  once in a while.
BC127::opResult BC127::restore()
{
  return stdCmd("RESTORE");
}

// Issue the "WRITE" command over the serial port to the BC127. This will
//  save the current settings to NVM, so they will be applied after a reset
//  or power cycle.
BC127::opResult BC127::writeConfig()
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
BC127::opResult BC127::reset()
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();
  
  // Now issue the reset command.
  _serialPort->print("RESET");
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to reset.
  while ((resetStart + 2000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() > 0) 
    {
      char temp = _serialPort->read();
      buffer.concat(temp);
    }
    
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
BC127::opResult BC127::knownStart()
{
  String EOL = String("\n\r");
  String buffer = "";
  
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
  
  // This is our timeout loop. We're going to give our module 100ms to come up
  //  with a new character, and return with a timeout failure otherwise.
  while (buffer.endsWith(EOL) != true)
  {
    // Purge the serial data received from the module, along with any data in
    //  the buffer at the time this command was sent.
    if (_serialPort->available() >0) 
    {
      buffer.concat(char(_serialPort->read()));
      startTime = millis();
    }
    if ((startTime + 1000) < millis()) return TIMEOUT_ERROR;
  }
  if (buffer.startsWith("ERR")) return SUCCESS;
  else return SUCCESS;
}
