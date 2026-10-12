#pragma once

#include <Arduino.h>

// Azure Static Web App uzerindeki /firmware/version.json dosyasini okur,
// sürüm farkliysa firmware'i indirir, SHA-256 dogrular ve yazar.
// Basarili ise cihaz yeniden baslar ve fonksiyon geri donmez.
// Donus degeri: false -> guncelleme yapilmadi (mesaj outMessage icindedir).
bool otaCheckAndUpdate(const char* currentVersion, String& outMessage);

// Yeni firmware onaylanmadan 3 kez acilirsa onceki surume doner (setup basinda cagirin).
bool otaBootCheck();
// Firmware Firebase'e basariyla yazabildiginde cagirin; geri donusu iptal eder.
void otaConfirmBoot();
// Son acilis geri donusle mi oldu? (bir kez true doner)
bool otaWasRolledBack();
