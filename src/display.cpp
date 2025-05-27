#include "Display.h"



Display::Display(U8G2 &u8g2) : u8g2(u8g2) {}

char* Display::formatTime(const RtcDateTime& dt) {
    snprintf_P(time_format_buffer, 
            sizeof(time_format_buffer),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return time_format_buffer;
}

char* Display::formatDate(const RtcDateTime& dt) {
    snprintf_P(date_format_buffer, 
            sizeof(date_format_buffer),
            PSTR("%02u/%02u/%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year()-2000 );
    return date_format_buffer;
}


void Display::showManuelRelayStatus(int relay_no, int status) {
    //alarmlar ile relay doğrudan eşleşmez. Aynı relay için birden fazla alarm tanımlanmış olabilir.
    //Bu ekranda bir relay on/off yapılacak.
    char maneulControlString[23];  //Vana 1
    snprintf_P(maneulControlString, sizeof(maneulControlString), PSTR("Vana %02u Status: %02u"), relay_no, status);    
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_7x13B_mf);
        u8g2.drawStr(0, 10, maneulControlString);
    } while ( u8g2.nextPage() );
}

void Display::show(Alarm a, int param_no) {
    char paramString[20];
    char valueString[10];
    String param_name = a.GetParamName(param_no);
    int value = a.GetParamValue(param_no);
    snprintf_P(paramString, sizeof(paramString), PSTR("%s"), param_name.c_str());
    snprintf_P(valueString, sizeof(paramString), PSTR("%02u"), value);

    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_7x13B_mf);        
        u8g2.drawStr(0, 10, paramString);        
        u8g2.setFont(u8g2_font_courB18_tr);                
        u8g2.drawStr(0, 30, valueString);        
    } while ( u8g2.nextPage() );
}

void Display::show(int param_value, int mode) {
    char valueString[10];
    snprintf_P(valueString, sizeof(valueString), PSTR("%02u"), param_value);
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_7x13B_mf);        
        u8g2.drawStr(0, 10, "Parametre Degeri");        
        u8g2.setFont(u8g2_font_courB18_tr);                
        u8g2.drawStr(0, 30, valueString);        
    } while ( u8g2.nextPage() );
}


void Display::show(Alarm a, RtcDateTime now, int pressure) {
    int x = 0, y = 10;
    char currenttimestring[10];
    char currentdatestring[10];
    char statusstring[2];
    char alarmtime[16];
    char task[17];  //4x(12+23) P
    char current_status[17];  //3x(9+23) P
    char paramNo[4];
    char maneulControlString[10];  //Vana 1
    String s;

       //-------------- 1. satır -----------------------
        char * buffer = formatTime(now);
        strcpy(currenttimestring, buffer);
        buffer = formatDate(now);
        strcpy(currentdatestring, buffer);
        switch(a.alarm_status) {
            case Alarm::ALARM_STATUS_STOPPED: //Stopped. (nothing to do)
                s = "S";
                break;
            case Alarm::ALARM_STATUS_RUNNING:  //Running
                s = "R";
                break;
            case Alarm::ALARM_STATUS_WAITING: //Waiting  (task is idle mode.)
                s = "W";
                break;
            case Alarm::ALARM_STATUS_FAILURE: //Failure
                s = "F";
                break;
        }
        s.toCharArray(statusstring, 2);    

        //--------- 2. satır alarm no ve alarm saati-----------------------------
        snprintf_P(alarmtime, sizeof(alarmtime), PSTR("%02u  %02u:%02u  R%02u"), a.alarmno, a.alarm_hour, a.alarm_minute,a.relay_no);

        //---------  3. satır task ----------------------------
        snprintf_P(task, sizeof(task), PSTR("%02ux(%02u + %02u) %02u"), a.repeat_count, a.run_minutes, a.idle_minutes, a.pressure);

        //---------  4. satır current statüs ----------------------------
        snprintf_P(current_status, sizeof(current_status), PSTR("%02ux(%02u + %02u) %03u"), a.repeat_count_remaining, a.run_remaining_minutes, a.idle_remaining_minutes, pressure);

        u8g2.firstPage();
        do {
            // 1. satır
            u8g2.setFont(u8g2_font_7x13B_mf);        
            u8g2.drawStr(x+12,y,currenttimestring);        
            u8g2.drawStr(x+71,y,currentdatestring);        
            u8g2.drawStr(x, y, statusstring);        
            u8g2.drawLine(0,12, 127,12);        

            //2 satır
            u8g2.setFont(u8g2_font_7x13_mf);
            u8g2.drawStr(x,y+18,alarmtime);

            //3. satır
            u8g2.drawStr(x,y+34,task);

            //4. satır
            u8g2.drawStr(x,y+51,current_status);

        } while ( u8g2.nextPage() );  
    }

void Display::showIPAddress(const char* ipAddress) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_7x13B_mf);
        u8g2.drawStr(0, 10, "Device IP Address:");
        u8g2.setFont(u8g2_font_7x13B_mf);
        u8g2.drawStr(0, 30, ipAddress);
    } while (u8g2.nextPage());
}

void Display::showPressure(int pressure) {
    char pressureString[14];
    snprintf_P(pressureString, sizeof(pressureString), PSTR("P: %03u"), pressure);
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_fur17_tr);
        u8g2.drawStr(0, 22, pressureString);
    } while (u8g2.nextPage());
}
