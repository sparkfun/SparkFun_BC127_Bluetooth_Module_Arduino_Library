#include <bc127.h>
#include <SoftwareSerial.h>

SoftwareSerial swPort(3,2);
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
  BLETesting();
  while(1);
}

void BLETesting()
{
  int connectionResult = 0;
  Serial.print("BLE result: "); Serial.println(BTModu.BLECentral());
  BTModu.writeConfig();
  BTModu.reset();
  Serial.print("BLE scan result: "); Serial.println(BTModu.BLEScan(10));
  String address;
  for (byte i = 0; i < 5; i++)
  {
    if (BTModu.getAddress(i, &address))
    {
      if (address.startsWith("20FABB"))
      {
        Serial.print("BC127 found at index "); Serial.println(i);
        Serial.print("Connect result: ");
        connectionResult = BTModu.connect(address, BLE);
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

void BTClassicTesting()
{
  int connectionResult = 0;
  Serial.print("Inquiry result: "); Serial.println(BTModu.inquiry(10));
  String address;
  for (byte i = 0; i < 5; i++)
  {
    if (BTModu.getAddress(i, &address))
    {
      if (address.startsWith("20FABB"))
      {
        Serial.print("BC127 found at index "); Serial.println(i);
        Serial.print("Connect result: ");
        connectionResult = BTModu.connect(address, SPP);
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
