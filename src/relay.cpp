#include "relay.h"
#include <Arduino.h>

void (*stateChangeCallback)(int, bool) = nullptr;

Relay::Relay(int _relayNo, int _pinNo)
{
    //kodun bu kısmında serial print kullandığımda kod çöktü. O yüzden ilgili kısımları kalıcı olarak kaldırdım.
  relayNo = _relayNo;
  pin = _pinNo;
  status = RELAY_OFF;
  oldStatus = RELAY_OFF;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void Relay::SetStateChangeCallback(void (*callback)(int, bool, int)) {
    stateChangeCallback = callback;
}

void Relay::TurnOn(int reason)
{
  digitalWrite(pin, LOW);
  status = RELAY_ON;
  if(oldStatus == RELAY_OFF) {
      // Only call the callback if the state has changed from OFF to ON
      if (stateChangeCallback) {
          stateChangeCallback(relayNo, true, reason);
      }
  }
  oldStatus = RELAY_ON;
}

void Relay::TurnOff(int reason)
{
  digitalWrite(pin, HIGH);
  status = RELAY_OFF;
  if (oldStatus == RELAY_ON) {
        // Only call the callback if the state has changed from ON to OFF
        // and the callback is set
    if (stateChangeCallback) {
        stateChangeCallback(relayNo, false, reason);
    }
  }
  oldStatus = RELAY_OFF;
}