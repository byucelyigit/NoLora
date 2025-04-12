#ifndef ALARM_H
#define ALARM_H

#include <RtcDS1302.h>
#include <EEPROM.h>
#include <ArduinoJson.h> // Include ArduinoJson library

class Alarm
{
  public:
    enum AlarmStatus {
        ALARM_STATUS_STOPPED = 0,  //nothing to do
        ALARM_STATUS_RUNNING = 1,
        ALARM_STATUS_WAITING = 2,  //task is waiting mode.
        ALARM_STATUS_FAILURE = 3
    };

    enum AlarmFrequency {
        ALARM_FREQUENCY_OFF = 0,
        ALARM_FREQUENCY_EVERYDAY = 1,
        ALARM_FREQUENCY_DAYAFTER = 2,
        ALARM_FREQUENCY_TWODAYSAFTER = 3,
    };

    Alarm(int alarmno);
    RtcDateTime AlarmTime;
    void Update(RtcDateTime rtc); //belli bir aralıkta çağrılır ve alarmın durumunu günceller. Tetikleme durumu, kalan süre, vb.
    void SaveAlarm();
    void initAlarms(EEPROMClass &eprom); //Test amacı ile ve ilk alarm verileri set edilmiş olsun diye kullanılır.
    void initEPROM(EEPROMClass &eprom);
    void initFirstAlarm(EEPROMClass &eprom);
    void Log(String stimeDifferenceMinutes, String rtcTotalMinutes);
    int GetParamValue(int param_no);
    String GetParamName(int param_no);
    int GetParamMinValue(int param_no);
    int GetParamMaxValue(int param_no);
    void SetParamValue(int param_no, int param_value, EEPROMClass &eprom);
    void SaveLastDate(int day, int month, int year, EEPROMClass &eprom);
    
    void SetStatusChangeCallback(void (*callback)(int alarmNo, AlarmStatus newStatus));
    String toJson(); // New method to return parameters as JSON
    bool taskStatus = false; // herhangi bir görev icra ediyorsa true değerini alır.
        
    int alarmno; 
    int alarm_hour;
    int alarm_minute;
    int repeat_count;
    int run_minutes;
    int idle_minutes;
    int idle_time;
    AlarmFrequency day_period;  // 0 off, 1 everyday, 2 day after, 3 two days after
    int relay_no;
    int last_run_day;
    int last_run_month;
    int last_run_year;
    int pressure;
    int trigger_type;
    AlarmStatus alarm_status;
    int repeat_count_remaining;
    int run_remaining_minutes;
    int idle_remaining_minutes;



  private:
    int address(int alarmno);
    int parametre_count = 14;
    int bank_count = 8;
    int total_memory = address(parametre_count) + parametre_count;    
    RtcDateTime alarm_start_time;
    long rtcAlarmStartTotalMinutes; //alarmın başlama zamanının belli bir sıfır noktasına göre dakika cinsinden değeri
    String previousLog = "";
    static void (*statusChangeCallback)(int alarmNo, AlarmStatus newStatus); // Marked as static
    AlarmStatus oldAlarmStatus = ALARM_STATUS_STOPPED; // 
    void RunCycle(RtcDateTime rtc); // Her bir iterasyonda çağrılır. alarmın durumunu günceller. Tetikleme durumu, kalan süre, vb.
};



#endif // ALARM_H