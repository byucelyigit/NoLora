#include <Arduino.h>
#include "pushover.h"
#include <HTTPClient.h>

const char *PUSHOVER_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
"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\n"
"uVBptWcBI6C/Gx9ROA==\n"
"-----END CERTIFICATE-----\n";

// yukarıdaki sertifika https://randomnerdtutorials.com/esp32-https-requests/ sitesinde anlatıltığı şekilde alınmıştır. Geçerlilik süresi 2026 olarak gözüküyor. 


const char* serverUrl = "https://api.pushover.net/1/messages.json"; // Pushover API URL
const char* userKey = "uzirvs2c54j5aeojecat9rf4zpwrot"; // Pushover kullanıcı anahtarınız
const char* appToken = "av15a3a64zcmjk9yrb1n8bff9auj2r"; // Pushover uygulama token'ınız


void Pushover::sendNotification(String message) {
    Serial.println("Bildirim gönderiliyor...");
    Serial.println("Mesaj: " + message);
    if (pushNotification) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.setTimeout(5000); // Set timeout for HTTP requests

            // Use the IP address directly
            //IPAddress serverIP = performMDNSLookup("api.pushover.net"); // Resolve the hostname to an IP
            //if (serverIP) {
                //String serverUrl = "https://api.pushover.net/1/messages.json";
                //Serial.println("Server URL: " + serverUrl);
                int retryCount = 0;
                bool started = false;
                while (retryCount < 5 && !started) {
                    started = http.begin(serverUrl); // Initialize HTTP client with the server URL
                    Serial.println("HTTP client : " + String(started));
                    if (!started) {
                        Serial.println("HTTP begin failed, retrying...");
                        delay(1000); // Wait 1 second before retrying
                        retryCount++;
                    }
                }
                if (!started) {
                    Serial.println("HTTP begin failed after 5 attempts.");
                    return; // Exit the function if initialization fails
                }

                http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Specify content type
                Serial.println("HTTP header added.");
                String payload = "token=" + String(appToken) + "&user=" + String(userKey) +
                                 "&message=" + message;
                Serial.println("Payload created.");
                int httpResponseCode = http.POST(payload); // Send the POST request. burada https kaynaklı root sertifikasını yüklemek gerekebilir. örnek kod var.
                //https://github.com/brunojoyal/PushoverESP32/blob/master/src/PushoverESP32.cpp
                

                if (httpResponseCode > 0) {
                    Serial.println("Bildirim gönderildi: " + String(httpResponseCode));
                    Serial.println("Yanıt: " + http.getString());
                } else {
                    Serial.println("Bildirim gönderme başarısız: " + String(httpResponseCode));
                }
                // Close the connection
                Serial.println("HTTP connection closing...");
                http.end(); // Properly close the HTTP client
                Serial.println("HTTP connection closed.");
            // } else {
            //     Serial.println("Hostname resolution failed.");
            // }
        } else {
            Serial.println("WiFi bağlantısı yok, bildirim gönderilemedi.");
        }
    }
}