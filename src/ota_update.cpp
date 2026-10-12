#include "ota_update.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <time.h>
#include <mbedtls/sha256.h>

#define OTA_BASE_URL "https://thankful-meadow-0dd652c03.6.azurestaticapps.net/firmware/"

// DigiCert Global Root G2 (gecerlilik: 2038-01-15)
static const char OTA_ROOT_CA[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)CERT";

// Sertifika gecerlilik kontrolu icin saatin dogru olmasi gerekir.
static bool otaEnsureClockValid() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    for (int i = 0; i < 40; i++) {
        if (time(nullptr) > 1700000000) return true;
        delay(250);
    }
    return false;
}

static bool isSafeFileName(const String& name) {
    if (name.length() == 0 || name.length() > 64) return false;
    for (size_t i = 0; i < name.length(); i++) {
        char c = name[i];
        bool ok = isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-';
        if (!ok) return false;
    }
    return name.indexOf("..") < 0;
}

static bool sha256Matches(const uint8_t digest[32], const String& expectedHex) {
    if (expectedHex.length() != 64) return false;
    char hex[65];
    for (int i = 0; i < 32; i++) snprintf(hex + i * 2, 3, "%02x", digest[i]);
    return expectedHex.equalsIgnoreCase(String(hex));
}

bool otaCheckAndUpdate(const char* currentVersion, String& outMessage) {
    if (WiFi.status() != WL_CONNECTED) {
        outMessage = "OTA: WiFi yok.";
        return false;
    }
    if (!otaEnsureClockValid()) {
        outMessage = "OTA: saat alinamadi, sertifika dogrulanamaz.";
        return false;
    }

    // 1) Manifest
    String version, file, sha256;
    size_t expectedSize = 0;
    {
        WiFiClientSecure client;
        client.setCACert(OTA_ROOT_CA);
        HTTPClient http;
        http.setTimeout(10000);
        if (!http.begin(client, String(OTA_BASE_URL) + "version.json")) {
            outMessage = "OTA: manifest baglantisi acilamadi.";
            return false;
        }
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            outMessage = "OTA: manifest HTTP " + String(code);
            http.end();
            return false;
        }
        String body = http.getString();
        http.end();

        JsonDocument doc;
        if (deserializeJson(doc, body)) {
            outMessage = "OTA: manifest JSON hatali.";
            return false;
        }
        version = doc["version"] | "";
        file = doc["file"] | "";
        sha256 = doc["sha256"] | "";
        expectedSize = doc["size"] | 0;
    }

    if (version.length() == 0 || !isSafeFileName(file) || sha256.length() != 64 || expectedSize < 100000) {
        outMessage = "OTA: manifest eksik/gecersiz.";
        return false;
    }
    if (version == currentVersion) {
        outMessage = "OTA: cihaz zaten guncel (" + version + ").";
        return false;
    }

    // 2) Firmware indirme ve yazma
    WiFiClientSecure client;
    client.setCACert(OTA_ROOT_CA);
    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(client, String(OTA_BASE_URL) + file)) {
        outMessage = "OTA: firmware baglantisi acilamadi.";
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        outMessage = "OTA: firmware HTTP " + String(code);
        http.end();
        return false;
    }
    int len = http.getSize();
    if (len <= 0 || (size_t)len != expectedSize) {
        outMessage = "OTA: boyut uyusmuyor.";
        http.end();
        return false;
    }
    if (!Update.begin(len)) {
        outMessage = "OTA: yer yok / Update.begin hatasi.";
        http.end();
        return false;
    }

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[1024];
    int remaining = len;
    unsigned long lastData = millis();
    bool failed = false;

    while (remaining > 0) {
        size_t avail = stream->available();
        if (avail == 0) {
            if (!http.connected() || millis() - lastData > 15000) { failed = true; break; }
            delay(5);
            continue;
        }
        size_t n = stream->readBytes(buf, min(avail, min(sizeof(buf), (size_t)remaining)));
        if (n == 0) continue;
        lastData = millis();
        if (Update.write(buf, n) != n) { failed = true; break; }
        mbedtls_sha256_update_ret(&ctx, buf, n);
        remaining -= n;
        yield();
    }
    http.end();

    uint8_t digest[32];
    mbedtls_sha256_finish_ret(&ctx, digest);
    mbedtls_sha256_free(&ctx);

    if (failed || remaining != 0) {
        Update.abort();
        outMessage = "OTA: indirme yarim kaldi.";
        return false;
    }
    if (!sha256Matches(digest, sha256)) {
        Update.abort();
        outMessage = "OTA: SHA-256 uyusmuyor, yazma iptal.";
        return false;
    }
    if (!Update.end(true)) {
        outMessage = "OTA: Update.end hatasi " + String(Update.getError());
        return false;
    }

    outMessage = "OTA: " + String(currentVersion) + " -> " + version + " yuklendi, yeniden basliyor.";
    return true;
}
