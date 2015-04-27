/****************************************************************
Connection management functions for BC127 modules.

This code is beerware; if you use it, please buy me (or any other
SparkFun employee) a cold beverage next time you run into one of
us at the local.

238 Jan 2014- Mike Hord, SparkFun Electronics

Code developed in Arduino 1.0.5, on an Arduino Pro Mini 5V.
****************************************************************/

#include "SparkFunbc127.h"
#include <Arduino.h>

// One of the neat features of the BC127 is the ability to control an audio
//  player remotely. This function will activate those features, programmatically.
BC127::opResult BC127::musicCommands(audioCmds command)
{
  switch(command)
  {
    case PAUSE:
      return stdCmd("MUSIC PAUSE");
    case PLAY:
      return stdCmd("MUSIC PLAY");
    case FORWARD:
      return stdCmd("MUSIC FORWARD");
    case BACK:
      return stdCmd("MUSIC BACKWARD");
    case STOP:
      return stdCmd("MUSIC STOP");
    case UP:
      return stdCmd("VOLUME UP");
    case DOWN:
      return stdCmd("VOLUME DOWN");
    default:
      return INVALID_PARAM;
  }
}

// In order to set the module as a source for streaming audio out to another
//  device, you must set the "CLASSIC_ROLE" parameter to 1, then write/reset to
//  make that setting active. This function handles this parameter setting.
BC127::opResult BC127::setClassicSource()
{
  return stdSetParam("CLASSIC_ROLE", "1");
}

// Of course, we also need some way to return the module to sink mode, if we
//  want to do that.
BC127::opResult BC127::setClassicSink()
{
  return stdSetParam("CLASSIC_ROLE", "0");
}

// BLEAdvertise() and BLENoAdvertise() turn advertising on and off for this
//  module. Advertising must be turned on for another BLE device to detect the
//  module, and the module *must* be a peripheral for advertising to work (see
//  the "BLEPeripheral()" function.
BC127::opResult BC127::BLEAdvertise()
{
  return stdCmd("ADVERTISING ON");
}

BC127::opResult BC127::BLENoAdvertise()
{
  return stdCmd("ADVERTISING OFF");
}

// Scan is very similar to inquiry, but for BLE devices rather than for classic.
//  Result format is slightly different, however- different enough to warrant
//  another whole function, IMO.
BC127::opResult BC127::BLEScan(int timeout)
{
  // We're going to assume that what's going to happen is a timeout with no
  //  valid input from the module.
  int result = 0;
  String buffer = "";
  String addressTemp;
  String EOL = String("\n\r");
  
  for (byte i = 0; i <5; i++) _addresses[i] = "";
  _numAddresses = 0;
    
  knownStart();
  
  // Now issue the inquiry command.
  _serialPort->print("SCAN "); _serialPort->print(timeout); _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  
  // Calculate a timeout value that's a tish longer than the module will
  //  use. This is our catch-all, so we don't sit in this loop forever waiting
  //  for input that will never come from the module.
  unsigned long loopTimeout = timeout*1300;
  
  // Oooookaaaayyy...now the fun part. A state machine that parses the input
  //  from the Bluetooth module!
  while (loopStart + loopTimeout > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));
    
    // If we've received the EOL string, we should parse the current buffer
    //  contents. There are three potential results to expect, here:
    // "OK" - The module has finished scanning (timed out) and the results we
    //   have are the only ones we'll ever get.
    // "ERROR" - Something went wrong and we're not scanning.
    //  SCAN <addr> <short_name> <role> <RSS>
    //  <addr> is a 12-digit hex value
    //  <short_name> is a string, surrounded by carets ( <like this> )
    //  <role> is advertising flags. BC127 devices will show up as 0A; single mode
    //    devices as 02.
    //  <RSS> is the signal strength. Anything better than -70dBm is likely to be
    //    quite okay for connecting.
    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("OK")) return (opResult)result;
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("SC")) // An address has been found!
      {
        addressTemp = buffer.substring(5,17);
        buffer = "";
        if (_numAddresses == 0) 
        {
          _addresses[0] = addressTemp;
          _numAddresses++;
          result = (opResult)1;
        }
        else // search the list for this address, and append if it's not in
             //  the list and the list isn't too long.
        {
          for (char i = 0; i < _numAddresses; i++)
          {
            if (addressTemp == _addresses[i])
            {
              addressTemp = "x";
              break;
            }
          }
          if (addressTemp != "x")
          {
            _addresses[_numAddresses++] = addressTemp;
            result++;
          }
          if (_numAddresses == 5) return (opResult)result;
        }
      }
    }
  }
  return TIMEOUT_ERROR;
}

BC127::opResult BC127::enterDataMode()
{
  return stdCmd("ENTER_DATA");
}

// Adequate to most situations, unless the user has adjust the CMD_TO value.
//  The default value of CMD_TO means that at least 400ms must elapse before
//  the $$$$ for exiting data mode will be recognized. You also need to wait
//  400ms AFTER issuing it, but that's handled by us waiting for the OK
//  response. 
BC127::opResult BC127::exitDataMode(int guardDelay)
{
  String buffer;
  String EOL = String("\n\r"); // This is just handy.
  
  delay(guardDelay);
  
  _serialPort->print("$$$$");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to exit data mode.
  while (loopStart + 2000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// connect by index
//  Attempts to connect to one of the Bluetooth devices which has an address
//  stored in the _addresses array.
BC127::opResult BC127::connect(char index, connType connection)
{
  if (index >= _numAddresses) return INVALID_PARAM;
  else return connect(_addresses[index], connection);
}

// connect by address
//  Attempts to connect to one of the Bluetooth devices which has an address
//  stored in the _addresses array.
BC127::opResult BC127::connect(String address, connType connection)
{
  // Before we go any further, we'll do a simple error check on the incoming
  //  address. We know that it should be 12 hex digits, all uppercase; to
  //  minimize execution time and code size, we'll only check that it's 12
  //  characters in length.
  if (address.length() != 12) return INVALID_PARAM;

  // We need a buffer to store incoming data.
  String buffer;
  String EOL = String("\n\r"); // This is just handy.
  
  // Convert our connType enum into the actual string we need to send to the
  //  BC127 module.
  switch(connection)
  {
    case SPP:
      buffer = " SPP";
      break;
    case BLE:
      buffer = " BLE";
      break;
    case A2DP:
      buffer = " A2DP";
      break;
    case AVRCP:
      buffer = " AVRCP";
      break;
    case HFP:
      buffer = " HFP";
      break;
    case PBAP:
      buffer = " PBAP";
      break;
    default:
      buffer = " SPP";
      break;
  }
  
  knownStart(); // Purge serial buffers on both the module and the Arduino.
  
  // Now issue the inquiry command.
  _serialPort->print("OPEN "); 
  _serialPort->print(address);
  _serialPort->print(buffer);
  _serialPort->print("\r");
  // We need to wait until the command finishes before we start looking for a
  //  response; that's what flush() does.
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the connect command. Bog-standard Arduino stuff.
  unsigned long connectStart = millis();
  
  buffer = "";

  // The timeout on this is 5 seconds; that may be a bit long.
  while (connectStart + 5000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));
    
    // At some point, the BC127 response string will contain the EOL string.
    //  Once that happens, we can figure out what the response looks like:
    //  "ERROR" - there's a syntax error in your message to the module; this is
    //    kind of unlikely, although it could happen if you call this function
    //    with an invalid address (something not entirely uppercase hex digits)
    //  "OPEN_ERROR" - most likely, the module can't find any devices with that
    //    address.
    //  "PAIR_ERROR" - the connection was refused by the remote module.
    //  "PAIR_OK" - the connection has been made, but the SPP channel is not
    //    yet open. We should probably just ignore this.
    //  "OPEN_OK" - ready to rock! This is when we should return success.
    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ERROR")) return MODULE_ERROR;
      if (buffer.startsWith("OPEN_ERROR")) return CONNECT_ERROR;
      if (buffer.startsWith("PAIR_ERROR")) return REMOTE_ERROR;
      if (buffer.startsWith("OPEN_OK")) return SUCCESS;
      buffer = "";    
    }
  }
  return TIMEOUT_ERROR;
}

// Runs the "INQUIRY" command, with user defined timeout. Returns the number of
//  devices found, up to 5. The response expected looks like this:
//    INQUIRY 20FABB010272 240404 -37db
//    INQUIRY A4D1D203A4F4 6A041C -91db
//    OK
// "ERROR" is, as always, a possible result. What we want to do is extract that
//  12-digit hex value and save it. We may get duplicates; we only want to keep
//  new addresses. The parameter "timeout" is not in seconds; it can be between
//  1 and 48 inclusive, and the timeout period will be 1.28*timeout. We'll set
//  an internal timeout period that is slightly longer than that, for safety.
BC127::opResult BC127::inquiry(int timeout)
{

  int result = 0;
  String buffer = "";
  String addressTemp;
  String EOL = String("\n\r");
  
  for (byte i = 0; i <5; i++) _addresses[i] = "";
  _numAddresses = -1;
    
  knownStart(); // Purge serial buffers on Arduino and module.
  
  // Now issue the inquiry command.
  _serialPort->print("INQUIRY "); 
  _serialPort->print(timeout);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the inquiry. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  
  // Calculate a reset timeout value that's a tish longer than the module will
  //  use. This is our catch-all, so we don't sit in this loop forever waiting
  //  for input that will never come from the module.
  unsigned long loopTimeout = timeout*1300;
  
  // Oooookaaaayyy...now the fun part. A state machine that parses the input
  //  from the Bluetooth module!
  while (loopStart + loopTimeout > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));
    
    // If we've received the EOL string, we should parse the current buffer
    //  contents. There are three potential results to expect, here:
    // "OK" - The module has finished scanning (timed out) and the results we
    //   have are the only ones we'll ever get.
    // "ERROR" - Something went wrong and we're not scanning.
    // "INQUIRY <addr> <class> <rss>" - A remote device has responded. <addr>
    //   will be 12 upper case hex digits, and is the remote device's address,
    //   to be used to refer to that device later on. <class> is the remote
    //   device class; for example, by default, the BC127 will return 240404,
    //   which corresponds to a Bluetooth headset. <rss> is the received signal
    //   strength; generally, -70dBm is a good link strength.
    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("OK")) return (opResult)result;
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("IN")) // An address has been found!
      {
        // Nab the address from the received string and store it in addressTemp.
        addressTemp = buffer.substring(8,20);
        buffer = "";   // Clear the buffer for next round of data collection.
        // If this is the first address, we need to do some things to ensure
        //  that we get the right result value. If it's not first...
        if (_numAddresses == -1) 
        {
          _addresses[0] = addressTemp;
          _numAddresses = 1;
          result = 1;
        }
        else // ...search the list for this address, and append if it's not in
             //  the list and the list isn't too long.
        {
          // If we find it, change the addressTemp value to 'x'.
          for (char i = 0; i <= _numAddresses; i++)
          {
            if (addressTemp == _addresses[i])
            {
              addressTemp = "x";
              break;
            }
          }
          // If we get here and the value isn't x, it was a new value, and can
          //  be stored.
          if (addressTemp != "x")
          {
            _addresses[_numAddresses++] = addressTemp;
            result++;
          }
          // If we get HERE, our address list is full and we should return.
          if (_numAddresses == 5) return (opResult)result;
        }
      }
    }
  }
  return TIMEOUT_ERROR;
}

// Gets an address from the array of stored addresses. The return value allows
//  the user to check on whether there was in fact a valid address at the
//  requested index.
BC127::opResult BC127::getAddress(char index, String &address)
{
  if (index+1 > _numAddresses)
  {
    String tempString = "";
    address = tempString;
    return INVALID_PARAM;
  }
  else address = _addresses[index];
  return SUCCESS;
}

// There are times when it is useful to be able to know whether or not the
//  module is connected; this function will tell you whether or not the module
//  is connected with a certain protocol, or if it is connected at all. This is
//  particularly challenging; the strings coming from the module are so fast,
//  even at 9600 baud, that you don't really have time to parse them. The only
//  solution I've been able to come up with is to just let the buffer overflow
//  and give up on identifying connections by type.
BC127::opResult BC127::connectionState()
{
  String buffer;
  String EOL = String("\n\r");
  
  opResult retVal = TIMEOUT_ERROR;
  
  knownStart();
  
  _serialPort->print("STATUS");
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
  
  // This is our timeout loop. We'll give the module 500 milliseconds.
  //  Note: if you have more than one active connection this WILL overflow a
  //  software serial buffer. Working under this assumption, we're going to
  //  try and deal with both the overflow and no overflow case gracefully. I'm
  //  also removing the ability to check on a specific connection type, since
  //  that's what causes the overflow.
  while ((startTime + 500) > millis())
  {  
    // Grow the current buffered data, until we receive the EOL string.  
    while (_serialPort->available() > 0) 
    {
      char temp = _serialPort->read();
      buffer.concat(temp);
      if (temp = '\r') break;
    }  

    // Parse the current line of text from the module and see what we find out.
    if (buffer.endsWith(EOL))
    {
      // If the current line starts with "STATE", we need more parsing. This is
      //  also the only guaranteed result.
      if (buffer.startsWith("ST"))
      {
        // If "CONNECTED" is in the received string, we know we're connected,
        //  but not if we're connected with the particular profile we're
        //  interested in. So, we need to consider a bit further.
        if (buffer.substring(13, 15) == "ED") retVal = SUCCESS;
        // If "CONNECTED" *isn't* there, we want to return an appropriate error.
        else retVal = CONNECT_ERROR;
      }
      // If we ARE connected, we'll get a list of different link types. We'll
      //  want to parse over those and see if the profile we want is in it. If
      //  it is, we can call it a success; if not, do nothing.
      
      // NB This is commented out, because we'll always overflow the soft serial
      //  buffer if we have more than one type of connection open. I'm leaving
      //  it in just in case somebody wants to use it in a "hardware-only" mode.
      /*
      if (buffer.startsWith("LI"))
      {
        switch(connection)
        {
          case SPP:
            if (buffer.substring(17,19) == "SP") retVal = SUCCESS;
            break;
          case BLE:
            if (buffer.substring(17,19) == "BL") retVal = SUCCESS;
            break;
          case A2DP:
            if (buffer.substring(17,19) == "A2") retVal = SUCCESS;
            break;
          case HFP:
            if (buffer.substring(17,19) == "HF") retVal = SUCCESS;
            break;
          case AVRCP:
            if (buffer.substring(17,19) == "AV") retVal = SUCCESS;
            break;
          case PBAP:
            if (buffer.substring(17,19) == "PB") retVal = SUCCESS;
            break;
          case ANY:
          default:
            break;
        }
      }
      */
      // If by some miracle we *do* get to this point without a buffer overflow,
      //  we're safe to return without a buffer purge.
      if (buffer.startsWith("OK")) return retVal;
      buffer = "";
    }
  }
  // Okay, now we need to clean up our input buffer on the serial port. After
  //  all, we can be pretty sure that an overflow happened, and there's crap in
  //  the buffer.
  while (_serialPort->available() > 0) _serialPort->read();
  return retVal;
}
