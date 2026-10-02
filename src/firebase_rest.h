#pragma once

#include <Arduino.h>

// Realtime Database REST istemcisi; her istege Firebase Auth idToken'i (?auth=) ekler.
class FirebaseRest {
public:
    explicit FirebaseRest(const String& baseUrl);

    // Basarisiz istekte veya deger yoksa (null) -1 doner.
    int getInt(const String& path);
    // HTTP kodunu doner; basarili GET'te payload doldurulur.
    int getRaw(const String& path, String& payload);

    // Asagidakiler HTTP kodunu doner (basari 200), token alinamazsa -1.
    int setInt(const String& path, int value);
    int setString(const String& path, const String& value);
    int pushString(const String& path, const String& value);
    int setJson(const String& path, const String& json);

private:
    String _baseUrl;
    String _idToken;
    bool _hasToken = false;
    unsigned long _tokenExpiresAt = 0;
    unsigned long _nextLoginAllowedAt = 0;

    bool ensureToken();
    bool login();
    int request(const char* method, const String& path, const String* body, String* response);
};
