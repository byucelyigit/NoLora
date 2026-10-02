#include "firebase_rest.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const unsigned long TOKEN_REFRESH_MARGIN_MS = 5UL * 60UL * 1000UL;
static const unsigned long LOGIN_RETRY_DELAY_MS = 30UL * 1000UL;
static const uint16_t HTTP_TIMEOUT_MS = 6000;

FirebaseRest::FirebaseRest(const String& baseUrl) : _baseUrl(baseUrl) {
    if (!_baseUrl.endsWith("/")) {
        _baseUrl += "/";
    }
}

bool FirebaseRest::login() {
    WiFiClientSecure client;
    // Sertifika dogrulamasi yok; eski Firebase kutuphanesiyle ayni davranis.
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);

    String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=";
    url += FIREBASE_API_KEY;

    if (!http.begin(client, url)) {
        Serial.println("[FB] Login begin failed");
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    JsonDocument req;
    req["email"] = FIREBASE_USER_EMAIL;
    req["password"] = FIREBASE_USER_PASSWORD;
    req["returnSecureToken"] = true;
    String body;
    serializeJson(req, body);

    int code = http.POST(body);
    String payload = http.getString();
    http.end();

    if (code != HTTP_CODE_OK) {
        Serial.println("[FB] Login failed, HTTP " + String(code));
        return false;
    }

    JsonDocument res;
    if (deserializeJson(res, payload)) {
        Serial.println("[FB] Login response parse error");
        return false;
    }

    const char* idToken = res["idToken"];
    unsigned long expiresInSec = String(res["expiresIn"] | "3600").toInt();
    if (!idToken || expiresInSec == 0) {
        Serial.println("[FB] Login response missing token");
        return false;
    }

    _idToken = idToken;
    _hasToken = true;
    unsigned long lifetimeMs = expiresInSec * 1000UL;
    if (lifetimeMs > TOKEN_REFRESH_MARGIN_MS) {
        lifetimeMs -= TOKEN_REFRESH_MARGIN_MS;
    }
    _tokenExpiresAt = millis() + lifetimeMs;
    Serial.println("[FB] Login ok");
    return true;
}

bool FirebaseRest::ensureToken() {
    if (_hasToken && (long)(_tokenExpiresAt - millis()) > 0) {
        return true;
    }

    // Basarisiz login sonrasi her istekte tekrar denemeyip loop'u bloklamayi onler.
    if ((long)(_nextLoginAllowedAt - millis()) > 0) {
        return false;
    }

    if (login()) {
        return true;
    }

    _hasToken = false;
    _nextLoginAllowedAt = millis() + LOGIN_RETRY_DELAY_MS;
    return false;
}

int FirebaseRest::request(const char* method, const String& path, const String* body, String* response) {
    if (!ensureToken()) {
        return -1;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);

    String url = _baseUrl + path + ".json?auth=" + _idToken;
    if (!http.begin(client, url)) {
        return -1;
    }

    int code;
    if (body) {
        http.addHeader("Content-Type", "application/json");
        code = http.sendRequest(method, *body);
    } else {
        code = http.sendRequest(method);
    }

    if (response && code == HTTP_CODE_OK) {
        *response = http.getString();
    }
    http.end();

    // Token reddedildi; sonraki istekte yeniden giris yapilir.
    if (code == HTTP_CODE_UNAUTHORIZED) {
        _hasToken = false;
    }

    return code;
}

int FirebaseRest::getRaw(const String& path, String& payload) {
    return request("GET", path, nullptr, &payload);
}

int FirebaseRest::getInt(const String& path) {
    String payload;
    if (getRaw(path, payload) != HTTP_CODE_OK) {
        return -1;
    }
    payload.trim();
    if (payload.length() == 0 || payload == "null") {
        return -1;
    }
    return payload.toInt();
}

int FirebaseRest::setInt(const String& path, int value) {
    String body = String(value);
    return request("PUT", path, &body, nullptr);
}

int FirebaseRest::setString(const String& path, const String& value) {
    JsonDocument doc;
    doc.set(value);
    String body;
    serializeJson(doc, body);
    return request("PUT", path, &body, nullptr);
}

int FirebaseRest::pushString(const String& path, const String& value) {
    JsonDocument doc;
    doc.set(value);
    String body;
    serializeJson(doc, body);
    return request("POST", path, &body, nullptr);
}

int FirebaseRest::setJson(const String& path, const String& json) {
    return request("PUT", path, &json, nullptr);
}
