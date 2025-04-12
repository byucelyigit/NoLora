#include "relay.h"
#include <Arduino.h>


Relay::Relay(int _relayNo, int _pinNo)
{
    //kodun bu kısmında serial print kullandığımda kod çöktü. O yüzden ilgili kısımları kalıcı olarak kaldırdım.
  relayNo = _relayNo;
  pin = _pinNo;
  status = RELAY_OFF;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void Relay::TurnOn()
{
  digitalWrite(pin, LOW);
  status = RELAY_ON;
}

void Relay::TurnOff()
{
  digitalWrite(pin, HIGH);
  status = RELAY_OFF;
}