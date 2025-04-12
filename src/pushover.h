
#ifndef PUSHOVER_H
#define PUSHOVER_H

#include <Arduino.h> // Include Arduino library for String type

class Pushover 
{
public:
    bool pushNotification = true; // Set to true to enable notifications
    void sendNotification(String message);
};

#endif // PUSHOVER_H