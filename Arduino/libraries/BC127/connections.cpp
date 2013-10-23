#include "bc127.h"
#include <Arduino.h>

// One of the neat features of the BC127 is the ability to control an audio
//  player remotely. This function will activate those features, programmatically.
opResult BC127::musicCommands(audioCmds command)
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
  }
}

// BLEAdvertise() and BLENoAdvertise() turn advertising on and off for this
//  module. Advertising must be turned on for another BLE device to detect the
//  module, and the module *must* be a peripheral for advertising to work (see
//  the "BLEPeripheral()" function.
opResult BC127::BLEAdvertise()
{
  return stdCmd("ADVERTISING ON");
}

opResult BC127::BLENoAdvertise()
{
  return stdCmd("ADVERTISING OFF");
}

// Scan is very similar to inquiry, but for BLE devices rather than for classic.
//  Result format is slightly different, however- different enough to warrant
//  another whole function, IMO.
opResult BC127::BLEScan(int timeout)
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
  _serialPort->print("SCAN "); _serialPort->println(timeout);
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // Calculate a reset timeout value that's a tish longer than the module will
  //  use. This is our catch-all, so we don't sit in this loop forever waiting
  //  for input that will never come from the module.
  unsigned long resetTimeout = timeout*1300;
  
  // Oooookaaaayyy...now the fun part. A state machine that parses the input
  //  from the Bluetooth module!
  while (resetStart + resetTimeout > millis())
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

opResult BC127::enterDataMode()
{
  return stdCmd("ENTER_DATA");
}

// Adequate to most situations, unless the user has adjust the CMD_TO value.
//  The default value of CMD_TO means that at least 400ms must elapse before
//  the $$$$ for exiting data mode will be recognized. You also need to wait
//  400ms AFTER issuing it, but that's handled by us waiting for the OK
//  response. 
opResult BC127::exitDataMode()
{
  return exitDataMode(420);
}

opResult BC127::exitDataMode(int guardDelay)
{
  String buffer;
  String EOL = String("\n\r"); // This is just handy.
  
  delay(guardDelay);
  
  _serialPort->print("$$$$");
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
      if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// connect by index
//  Attempts to connect to one of the Bluetooth devices which has an address
//  stored in the _addresses array.
opResult BC127::connect(char index, connType connection)
{
  if (index >= _numAddresses) return INVALID_PARAM;
  else return connect(_addresses[index], connection);
}

// connect by address
//  Attempts to connect to one of the Bluetooth devices which has an address
//  stored in the _addresses array.
opResult BC127::connect(String address, connType connection)
{
  // Before we go any further, we'll do a simple error check on the incoming
  //  address. We know that it should be 12 hex digits, all uppercase; to
  //  minimize execution time and code size, we'll only check that it's 12
  //  characters in length.
  if (address.length() != 12) return INVALID_PARAM;

  // We need a buffer to store incoming data.
  String buffer;
  String EOL = String("\n\r"); // This is just handy.
  
  switch(connection)
  {
    case SPP:
      buffer = " SPP";
      break;
    case BLE:
      buffer = " BLE";
      break;
    default:
      buffer = " SPP";
      break;
  }
  
  knownStart();
  
  // Now issue the inquiry command.
  _serialPort->print("OPEN "); 
  _serialPort->print(address);
  _serialPort->println(buffer);
  // We need to wait until the command finishes before we start looking for a
  //  response; that's what flush() does.
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  buffer = "";

  // The timeout on this is 5 seconds; that may be a bit long.
  while (resetStart + 5000 > millis())
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
opResult BC127::inquiry(int timeout)
{
  // We're going to assume that what's going to happen is a timeout with no
  //  valid input from the module.
  int result = TIMEOUT_ERROR;
  String buffer = "";
  String addressTemp;
  String EOL = String("\n\r");
  
  for (byte i = 0; i <5; i++) _addresses[i] = "";
  _numAddresses = 0;
    
  knownStart();
  
  // Now issue the inquiry command.
  _serialPort->print("INQUIRY "); _serialPort->println(timeout);
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // Calculate a reset timeout value that's a tish longer than the module will
  //  use. This is our catch-all, so we don't sit in this loop forever waiting
  //  for input that will never come from the module.
  unsigned long resetTimeout = timeout*1300;
  
  // Oooookaaaayyy...now the fun part. A state machine that parses the input
  //  from the Bluetooth module!
  while (resetStart + resetTimeout > millis())
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
        addressTemp = buffer.substring(8,20);
        buffer = "";
        if (_numAddresses == 0) 
        {
          _addresses[0] = addressTemp;
          _numAddresses++;
          result = 1;
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

// Gets an address from the array of stored addresses. The return value allows
//  the user to check on whether there was in fact a valid address at the
//  requested index.
opResult BC127::getAddress(byte index, String *address)
{
  if (index >= _numAddresses)
  {
    String tempString = "";
    *address = tempString;
    return INVALID_PARAM;
  }
  else *address = _addresses[index];
  return SUCCESS;
}