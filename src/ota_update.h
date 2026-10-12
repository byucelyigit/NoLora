#pragma once

#include <Arduino.h>

// Azure Static Web App uzerindeki /firmware/version.json dosyasini okur,
// sürüm farkliysa firmware'i indirir, SHA-256 dogrular ve yazar.
// Basarili ise cihaz yeniden baslar ve fonksiyon geri donmez.
// Donus degeri: false -> guncelleme yapilmadi (mesaj outMessage icindedir).
bool otaCheckAndUpdate(const char* currentVersion, String& outMessage);
