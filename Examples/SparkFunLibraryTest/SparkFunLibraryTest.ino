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

#include <SparkFunbc127.h>
#include <SoftwareSerial.h>

SoftwareSerial swPort(3,2);  // RX, TX
BC127 BTModu(&swPort);

void setup()
{
  Serial.begin(9600);
  Serial.println("Hello world!");
  swPort.begin(9600);
}

void loop()
{
  Serial.print("Restore result: "); Serial.println(BTModu.restore());
  Serial.print("Write result: "); Serial.println(BTModu.writeConfig());
  Serial.print("Reset result: "); Serial.println(BTModu.reset());
  BTClassicTesting();
  while(1);
}

void addrTest()
{
  String address;
  Serial.print("Address result: "); Serial.println(BTModu.addressQuery(address));
  Serial.println(address);
}

void BLETesting()
{
  int connectionResult = 0;
  Serial.print("BLE result: "); Serial.println(BTModu.BLECentral());
  BTModu.writeConfig();
  BTModu.reset();
  Serial.print("BLE scan result: "); Serial.println(BTModu.BLEScan(10));
  String address;
  for (char i = 0; i < 5; i++)
  {
    if (BTModu.getAddress(i, address))
    {
      if (address.startsWith("20FABB"))
      {
        Serial.print("BC127 found at index "); Serial.println(i);
        Serial.print("Connect result: ");
        connectionResult = BTModu.connect(address, BC127::BLE);
        Serial.println(connectionResult);
        break;
      }
    }
  }
  if (connectionResult == 0) Serial.println("No BC127 modules found!");
  else
  {
    if (connectionResult == 1)
    {
      swPort.println("Hello, world!");
      swPort.flush();
    }
    else Serial.println("Failure!");
  } 
}

void baudTest()
{
  Serial.println(BTModu.setBaudRate(BC127::s19200bps));
  swPort.begin(19200);
  Serial.println(BTModu.setBaudRate(BC127::s19200bps));
}

void BTClassicTesting()
{
  int connectionResult = 0;
  Serial.print("Inquiry result: "); Serial.println(BTModu.inquiry(10));
  String address;
  for (byte i = 0; i < 5; i++)
  {
    if (BTModu.getAddress(i, address))
    {
      Serial.print("BT device found at address ");
      Serial.println(address);
      if (address.startsWith("20FABB"))
      {
        Serial.print("BC127 found at index "); Serial.println(i);
        Serial.print("Connect result: ");
        connectionResult = BTModu.connect(address, BC127::SPP);
        Serial.println(connectionResult);
        break;
      }
    }
  }
  if (connectionResult == 0) Serial.println("No BC127 modules found!");
  else
  {
    if (connectionResult == 1)
    {
      Serial.print("Entering data mode...");
      if (BTModu.enterDataMode()) 
      {
        Serial.println("OK!");
        swPort.println("Hello, world!");
        swPort.flush();
        delay(500);
        Serial.print("Exiting data mode...");
        if (BTModu.exitDataMode()) Serial.println("OK!");
        else Serial.println("Failure!");
      }
      else Serial.println("Failure!");
    }
  } 
}
