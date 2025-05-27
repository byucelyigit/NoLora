#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include <RtcDS1302.h>
#include <Alarm.h>

class Display {
public:

    enum Page {
        PAGE_ALARM_LIST = 0,
        PAGE_PARAM_LIST = 1,
        PAGE_PARAM_VALUE = 2,
        PAGE_MANUAL = 3
    };

    Display(U8G2 &u8g2);
    void show(Alarm a, RtcDateTime now, int pressure);
    void showManuelRelayStatus(int relay_no, int status);
    void show(Alarm a, int param_no);
    void show(int param_value, int mode);
    void showIPAddress(const char* ipAddress);
    void showPressure(int pressure);

private:
    U8G2 &u8g2;
    char time_format_buffer[10];
    char date_format_buffer[10];

    char* formatTime(const RtcDateTime& dt);
    char* formatDate(const RtcDateTime& dt);
};

#endif // DISPLAY_H