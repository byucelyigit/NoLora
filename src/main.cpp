#include <U8g2lib.h>
#include <SPI.h>
#include <ESP32RotaryEncoder.h>
#include <RtcDS1302.h>
#include <relay.h>
#include <Alarm.h>

#include <EEPROM.h>
#include "Display.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <pushover.h>
#include <Firebase.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */
#define RELAY_COUNT 4
#define PARAMETER_COUNT 13  
#define ALARM_COUNT 8




RTC_DATA_ATTR int bootCount = 0;

//pushover diye bir uygulmayı kullanarak iphone bildirim gönderme işlemi yapılabilir.
//https://pushover.net/apps/9auj2r-gardener
const char* ssid = "TP-Link_D1BE";
const char* password = "87973341";
const String REFERENCE_URL = "https://send-kurudere-messages-default-rtdb.europe-west1.firebasedatabase.app/";


const uint8_t DI_ENCODER_A   = 13;
const uint8_t DI_ENCODER_B   = 14;
const uint8_t RELAY1   = 25;
const uint8_t RELAY2   = 27;
const uint8_t RELAY3   = 26;
const uint8_t RELAY4   = 2;

const uint8_t BUTTON1_ENTER = 34;
const uint8_t BUTTON2_EXIT = 36; 
const uint8_t PRESSURE_ANALOG = 35;
const uint8_t CLOCK_IO = 18;
const uint8_t CLOCK_SCL = 5;
const uint8_t CLOCK_RST = 19;

bool button1_enter_Pressed = false;
bool button2_exit_pressed = false;

int alarmHr = 21;
int alarmMin = 00;
int alarm_no = 1;
Display::Page page_no = Display::Page::PAGE_ALARM_LIST;
int param_no = 1;
int functionNo = 1;
int param_value = 0;
int pressureLimit = 10;
int pressureMeasureTime = 1000; // 1 second
int pressureMeasureCount = 3;
int averagePressure;


char time_format_buffer[10] = "00:00:00"; 
char date_format_buffer[10] = "00/00/00"; 

ThreeWire myWire(CLOCK_IO,CLOCK_SCL,CLOCK_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

HTTPClient http;

EEPROMClass eprom;

Pushover pushover;

RotaryEncoder rotaryEncoder( DI_ENCODER_A, DI_ENCODER_B );

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Display display(u8g2);

Firebase fb(REFERENCE_URL);

//Preferences pref;

Alarm alrm[] = {Alarm(0), Alarm(1), Alarm(2), Alarm(3), Alarm(4), Alarm(5), Alarm(6), Alarm(7)};
Relay relay[] = {Relay(0, RELAY1), Relay(1, RELAY2), Relay(2, RELAY3), Relay(3, RELAY4)};

WebServer server(80);

unsigned long lastInteractionTime = 0; // Tracks the last interaction time
bool screenOn = true;

void printMessage(String payload, int x, int y) {
  int lenString = payload.length();
  char statusstring[lenString+1];
  payload.toCharArray(statusstring, lenString+1);
}

int measurePressure() {
  	int voltage = analogRead(PRESSURE_ANALOG);
  	float vin = (voltage * 3.3) / 1024.0;
  	int pressure = vin*100;
    //int pressure = 111;
	return pressure;
	//Serial.print(vin);
	//String myString = String(vin);
	//printMessage(myString, 0, 38);
	//delay(100); 
}

void knobCallback(long value) {
    lastInteractionTime = millis(); // Reset the last interaction time on knob turn    
    switch (page_no) {
        case Display::Page::PAGE_ALARM_LIST:
            alarm_no = value;
            break;
        case Display::Page::PAGE_PARAM_LIST:
            param_no = value;
            break;
        case Display::Page::PAGE_PARAM_VALUE:
            // Handle PARAM_VALUE case if needed
            param_value = value;
            break;
        case Display::Page::PAGE_MANUAL:
            // Handle MANUAL case if needed
            functionNo = value;
            //relay_no = value;
            break;    
        default:
            // Handle default case if needed
            break;
    }
}


#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[26];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

void logAlarmToFirebase(int alarmNo, const String& message) {
    String logPath = "AlarmLogs/Alarm" + String(alarmNo) + "/Log";
    String timestamp = String(Rtc.GetDateTime().Year()) + "-" +
                       String(Rtc.GetDateTime().Month()) + "-" +
                       String(Rtc.GetDateTime().Day()) + " " +
                       String(Rtc.GetDateTime().Hour()) + ":" +
                       String(Rtc.GetDateTime().Minute()) + ":" +
                       String(Rtc.GetDateTime().Second());
    String logMessage = "[" + timestamp + "] " + message;
    fb.pushString(logPath, logMessage);
    Serial.println("Logged to Firebase: " + logMessage);
}

void onAlarmStatusChange(int alarmNo, Alarm::AlarmStatus newStatus) // Corrected type
{
    Serial.print("Alarm ");
    Serial.println("-------------");
    Serial.println(alarmNo);
    //Serial.print(" status changed to ");
    //Serial.println(newStatus);
    // Add additional logic here if needed
    if (page_no != Display::Page::PAGE_MANUAL) //manuel durumdayken alarm komutlarını dikkate almaz.
    {
        Serial.print("Alarm Status Changed: ");
        Serial.print(newStatus);
        Serial.println("----------------");
        int rln = alrm[alarmNo].relay_no;
        String statusString;
        RtcDateTime now = Rtc.GetDateTime();
        switch (newStatus) {
            case Alarm::AlarmStatus::ALARM_STATUS_RUNNING:
                statusString = "RUNNING";
                relay[rln-1].TurnOn();
                snprintf(time_format_buffer, sizeof(time_format_buffer), "%02u:%02u:%02u", now.Hour(), now.Minute(), now.Second());
                pushover.sendNotification("Bahçe Sulama Başladı. " + String(rln) + " numaralı vana açıldı. Sistem saati: " + String(time_format_buffer) + " Beklenen görev süresi: " + String(alrm[alarmNo].repeat_count * (alrm[alarmNo].run_minutes + alrm[alarmNo].idle_minutes)) + " dakika. Görev No: " + String(alarmNo));
                logAlarmToFirebase(alarmNo, "Alarm " + String(alarmNo) + " STARTED" + "Relay No:" + String(rln));
                break;
            case Alarm::AlarmStatus::ALARM_STATUS_STOPPED:
                relay[rln-1].TurnOff();
                alrm[alarmNo].SaveLastDate(now.Day(), now.Month(), now.Year(), eprom); //normalde bunun alarm nesnesi içinde olması lazım. ama eprom nesnesinin her bir alarma gönderilmesi doğru mu bilemedim. şimdilik böyle kalsın.
                pushover.sendNotification("Bahçe Sulandı. " + String(rln) + " numaralı vana kapandı.");
                logAlarmToFirebase(alarmNo, "Alarm " + String(alarmNo) + " STOPPED");
                statusString = "STOPPED";
                break;
            case Alarm::AlarmStatus::ALARM_STATUS_WAITING:
                statusString = "WAITING";
                relay[rln-1].TurnOff();
                logAlarmToFirebase(alarmNo, "Alarm " + String(alarmNo) + " WAITING");
                break;
            default:
                statusString = "UNKNOWN";
                break;
        }
    }
}

void onRelayStateChange(int relayNo, bool isOn) {
    String state = isOn ? "ON" : "OFF";
    Serial.println("Relay " + String(relayNo) + " turned " + state);

    // Example: Log relay state change to Firebase
    String logPath = "RelayLogs/Relay" + String(relayNo) + "/Log";
    String timestamp = String(Rtc.GetDateTime().Year()) + "-" +
                       String(Rtc.GetDateTime().Month()) + "-" +
                       String(Rtc.GetDateTime().Day()) + " " +
                       String(Rtc.GetDateTime().Hour()) + ":" +
                       String(Rtc.GetDateTime().Minute()) + ":" +
                       String(Rtc.GetDateTime().Second());
    String logMessage = "[" + timestamp + "] Relay " + String(relayNo) + " turned " + state;
    fb.pushString(logPath, logMessage);
    Serial.println("Logged to Firebase: " + logMessage);

    // Update Firebase relays/status value
    int relayStatus = 0;
    if (isOn) {
        relayStatus |= (1 << (relayNo - 1)); // Set the bit for the relay
    } else {
        relayStatus &= ~(1 << (relayNo - 1)); // Clear the bit for the relay
    }
    fb.setInt("relays/status", relayStatus);
}

void handleAlarmJson() {
    if (server.hasArg("alarmNo")) {
        int alarmNo = server.arg("alarmNo").toInt();
        if (alarmNo >= 0 && alarmNo < ALARM_COUNT) {
            String json = alrm[alarmNo].toJson();
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "application/json", json);
        } else {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "text/plain", "Invalid alarm number");
        }
    } else {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(400, "text/plain", "Missing alarmNo parameter");
    }
}

void handleRoot() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/html", R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Relay Control</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f9; }
                h1 { color: #333; }
                button { padding: 10px 20px; font-size: 16px; color: #fff; background-color: #007BFF; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
                button:hover { background-color: #0056b3; }
            </style>
        </head>
        <body>
            <h1>Relay Control</h1>
            <button onclick="toggleRelay(1)">Toggle Relay 1</button>
            <button onclick="toggleRelay(2)">Toggle Relay 2</button>
            <button onclick="toggleRelay(3)">Toggle Relay 3</button>
            <button onclick="toggleRelay(4)">Toggle Relay 4</button>

            <script>
                function toggleRelay(relayNo) {
                    fetch(`/toggleRelay?relayNo=${relayNo}`)
                        .then(response => {
                            if (!response.ok) {
                                alert(`Failed to toggle Relay ${relayNo}`);
                            } else {
                                alert(`Relay ${relayNo} toggled successfully`);
                            }
                        })
                        .catch(error => {
                            alert(`Error: ${error.message}`);
                        });
                }
            </script>
        </body>
        </html>
    )rawliteral");
}

void handleSetTime() {
    if (server.hasArg("hour") && server.hasArg("minute") && server.hasArg("day") &&
        server.hasArg("month") && server.hasArg("year")) {
        
        int hour = server.arg("hour").toInt();
        int minute = server.arg("minute").toInt();
        int day = server.arg("day").toInt();
        int month = server.arg("month").toInt();
        int year = server.arg("year").toInt();

        if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 &&
            day > 0 && day <= 31 && month > 0 && month <= 12 && year >= 2000) {
            
            RtcDateTime newTime(year, month, day, hour, minute, 0);
            Rtc.SetDateTime(newTime);

            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "text/plain", "RTC time updated successfully.");
        } else {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "text/plain", "Invalid time parameters.");
        }
    } else {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(400, "text/plain", "Missing time parameters.");
    }
}

void handleSetAlarm() {
    if (server.hasArg("alarmNo") && server.hasArg("paramNo") && server.hasArg("paramValue")) {
        int alarmNo = server.arg("alarmNo").toInt();
        int paramNo = server.arg("paramNo").toInt();
        int paramValue = server.arg("paramValue").toInt();

        if (alarmNo >= 0 && alarmNo <= ALARM_COUNT) {
            if (paramNo >= 1 && paramNo <= PARAMETER_COUNT) { // Ensure paramNo is valid
                alrm[alarmNo].SetParamValue(paramNo, paramValue, eprom);
                server.sendHeader("Access-Control-Allow-Origin", "*");
                server.send(200, "text/plain", "Alarm parameter updated successfully.");
            } else {
                server.sendHeader("Access-Control-Allow-Origin", "*");
                server.send(400, "text/plain", "Invalid parameter number.");
            }
        } else {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "text/plain", "Invalid alarm number.");
        }
    } else {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(400, "text/plain", "Missing parameters.");
    }
}

void handleRtcTime() {
    RtcDateTime now = Rtc.GetDateTime();
    if (now.IsValid()) {
        char rtcTime[20];
        snprintf(rtcTime, sizeof(rtcTime), "%02u:%02u:%02u", now.Hour(), now.Minute(), now.Second());
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/plain", rtcTime);
    } else {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(500, "text/plain", "RTC time is invalid.");
    }
}

void handlePressure() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", String(averagePressure));
}

void handleToggleRelay() {
    if (server.hasArg("relayNo")) {
        int relayNo = server.arg("relayNo").toInt();
        if (relayNo >= 1 && relayNo <= RELAY_COUNT) {
            int relayIndex = relayNo - 1;
            if (relay[relayIndex].status == Relay::RELAY_OFF) {
                relay[relayIndex].TurnOn();
            } else {
                relay[relayIndex].TurnOff();
            }
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "text/plain", "Relay toggled successfully.");
        } else {
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(400, "text/plain", "Invalid relay number.");
        }
    } else {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(400, "text/plain", "Missing relayNo parameter.");
    }
}

void writeAlarmsToFirebase() {
    for (int i = 0; i < ALARM_COUNT; i++) {
        String alarmPath = "Alarms/Alarm" + String(i);
        String alarmJson = alrm[i].toJson();
        Serial.println("Alarm " + alarmJson);        
        fb.setJson(alarmPath, alarmJson);
        Serial.println("Alarm " + String(i) + " written to Firebase.");
    }
}

void updateRelaysFromFirebase() {
    
    static unsigned long lastFirebaseCheck = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastFirebaseCheck >= 3000) { // Check every 3 seconds
        lastFirebaseCheck = currentMillis;
        int relayStatus = fb.getInt("relays/status");
        if (relayStatus >= 0) { // Ensure a valid value is retrieved
            for (int i = 0; i < RELAY_COUNT; i++) {
                if (relayStatus & (1 << i)) { // Check if the bit is set
                    relay[i].TurnOn();
                } else {
                    relay[i].TurnOff();
                }
            }
        } else {
            Serial.println("Failed to read relay status from Firebase.");
        }
    }
}

void setup(){
	Serial.begin(115200);

    for (int i = 0; i < RELAY_COUNT; i++) {
        relay[i].TurnOff();
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Bağlanıyor...");
    }
    Serial.println("WiFi bağlantısı başarılı!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize mDNS
    // if (!MDNS.begin("esp32")) { // Replace "esp32" with your desired mDNS hostname
    //     Serial.println("Error starting mDNS");
    // } else {
    //     Serial.println("mDNS started");
    // }

    server.on("/", handleRoot);
    server.on("/alarm", handleAlarmJson);
    server.on("/setTime", handleSetTime);
    server.on("/setAlarm", handleSetAlarm);
    server.on("/rtc", handleRtcTime);
    server.on("/pressure", handlePressure); // Add endpoint for averagePressure
    server.on("/toggleRelay", handleToggleRelay);
    server.begin();
    Serial.println("HTTP server started");

    // fb.setString("Example/myString", "Hello World!");
    // fb.setInt("Example/myInt", 123);
    // fb.setFloat("Example/myFloat", 45.67);
    // fb.setBool("Example/myBool", true);

    //sistem resetlendiğinde (hafızada değerler yokken) aşağıdaki rutinin çalışması gerekir. 
    //alrm[0].initEPROM(eprom);
    //alrm[0].initFirstAlarm(eprom);
    




    //writeAlarmsToFirebase(); // Write all alarms to Firebase during setup

	++bootCount;

    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();


    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    //test clock setting
    //RtcDateTime tt =  RtcDateTime(2021, 1, 1,20, 59, 50);
    //Rtc.SetDateTime(tt);

   	pinMode (BUTTON1_ENTER, INPUT);
   	pinMode (BUTTON2_EXIT,INPUT);
   	pinMode (PRESSURE_ANALOG,INPUT);
	pinMode(RELAY1, OUTPUT);	
    pinMode(RELAY2, OUTPUT);	
    pinMode(RELAY3, OUTPUT);	
    pinMode(RELAY4, OUTPUT);	

    // aşağıda daha önceki değerlerin yüklenmesi işlemi yapılır. ilk defa değerler yazıldıktan sonra yukarıdaki satırlar kapatılır ve
    // sadece aşağıdakiler ile okuma yapması sağlanır.
    for(int i = 0; i < 8; i++) {
        alrm[i].initAlarms(eprom);
        alrm[i].SetStatusChangeCallback(onAlarmStatusChange);
    }

    for (int i = 0; i < RELAY_COUNT; i++) {
        relay[i].SetStateChangeCallback(onRelayStateChange);
    }

  	u8g2.begin();
  	u8g2.setDisplayRotation(U8G2_R2);
  	u8g2.setFont(u8g2_font_8x13_tr);	
	
	// This tells the library that the encoder has its own pull-up resistors
	rotaryEncoder.setEncoderType( EncoderType::FLOATING );

    lastInteractionTime = millis(); // Initialize the last interaction time

	// Range of values to be returned by the encoder: minimum is 1, maximum is 10
	// The third argument specifies whether turning past the minimum/maximum will
	// wrap around to the other side:
	//  - true  = turn past 10, wrap to 1; turn past 1, wrap to 10
	//  - false = turn past 10, stay on 10; turn past 1, stay on 1
	rotaryEncoder.setBoundaries(1, 8, true );

	// The function specified here will be called every time the knob is turned
	// and the current value will be passed to it
	rotaryEncoder.onTurned( &knobCallback );

	// The function specified here will be called every time the button is pushed and
	// the duration (in milliseconds) that the button was down will be passed to it
	// rotaryEncoder.onPressed( &buttonCallback );

	// This is where the inputs are configured and the interrupts get attached
	rotaryEncoder.begin();
	
	printMessage("Ready.", 0, 16);

	//esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

	Serial.println("Going to sleep now");
  	//delay(1000);
  	Serial.flush(); 
}

void loop() {
    static unsigned long lastPressureCheck = 0;
    static int pressureReadings[3] = {0, 0, 0};
    static int pressureIndex = 0;


    unsigned long currentMillis = millis();

    // Turn off the screen after 10 seconds of inactivity
    if (currentMillis - lastInteractionTime >= 10000) {
        //Serial.println("Screen off");
        u8g2.setPowerSave(1); // Turn off the display
        screenOn = false;
    }

    // Update pressure readings every second
    if (currentMillis - lastPressureCheck >= pressureMeasureTime) {
        lastPressureCheck = currentMillis;
        pressureReadings[pressureIndex] = measurePressure();
        pressureIndex = (pressureIndex + 1) % pressureMeasureCount;

        // Calculate the average pressure
        averagePressure = (pressureReadings[0] + pressureReadings[1] + pressureReadings[2]) / 3;

        // If the average pressure is below 130, stop all relays
        if (averagePressure < pressureLimit) {
            for (int i = 0; i < RELAY_COUNT; i++) {
                relay[i].TurnOff();
            }
        }
    }

    // Reset inactivity timer on button press or rotary encoder interaction
    if (digitalRead(BUTTON2_EXIT) == 0) {
        lastInteractionTime = currentMillis;
        if (!screenOn) {
            u8g2.setPowerSave(0); // Turn on the display
            screenOn = true;
        }
    }

    // Example: Print JSON representation of the first alarm
    if (Serial.available()) {
        char command = Serial.read();
        if (command == 'j') { // Press 'j' to print JSON
            Serial.println(alrm[0].toJson());
        }
    }

    // Example usage of mDNS lookup
    //performMDNSLookup("example.local"); // Replace "example.local" with the desired mDNS hostname

    RtcDateTime now = Rtc.GetDateTime();
    switch (page_no) {
        case Display::Page::PAGE_ALARM_LIST:
            if(digitalRead(BUTTON1_ENTER) == 0) { 
                if(!button1_enter_Pressed)
                {
                    button1_enter_Pressed = true;
                    page_no = Display::Page::PAGE_PARAM_LIST;
                    rotaryEncoder.setBoundaries(1, PARAMETER_COUNT, true ); 
                    rotaryEncoder.setEncoderValue(param_no); 
                    //sendNotification();
                }
            } else {
                button1_enter_Pressed = false;
            }
            // Handle button press to turn relay on only once until released
            if (digitalRead(BUTTON2_EXIT) == 0) {
                if (!button2_exit_pressed) {
                    button2_exit_pressed = true;
                    page_no = Display::Page::PAGE_MANUAL; rotaryEncoder.setBoundaries(1, RELAY_COUNT+1, true );
                }
            } else {
                button2_exit_pressed = false;
            }            

            display.show(alrm[alarm_no - 1], now, averagePressure);        
            break;

        case Display::Page::PAGE_PARAM_LIST:
            if(digitalRead(BUTTON1_ENTER) == 0)  {
                if(!button1_enter_Pressed)
                {
                    button1_enter_Pressed = true;
                    page_no = Display::Page::PAGE_PARAM_VALUE; 
                    param_value = alrm[alarm_no - 1].GetParamValue(param_no); 
                    int min = alrm[alarm_no-1].GetParamMinValue(param_no);
                    int max = alrm[alarm_no-1].GetParamMaxValue(param_no);
                    rotaryEncoder.setBoundaries(min, max, false);
                } 
            } else {
                button1_enter_Pressed = false;
            }
            if (digitalRead(BUTTON2_EXIT) == 0) {
                if (!button2_exit_pressed) {
                    button2_exit_pressed = true;
                    page_no = Display::Page::PAGE_ALARM_LIST;   
                    rotaryEncoder.setBoundaries(1, ALARM_COUNT, true );
                } else {
                    button2_exit_pressed = false;
                }
            }
            display.show(alrm[alarm_no-1],  param_no);        
            break;
        case Display::Page::PAGE_PARAM_VALUE:
            if(digitalRead(BUTTON1_ENTER) == 0){  
                if(!button1_enter_Pressed) { 
                    button1_enter_Pressed = true;
                    alrm[alarm_no - 1].SetParamValue(param_no, param_value, eprom);
                    page_no = Display::Page::PAGE_PARAM_LIST; 
                } 
            } else {
                button1_enter_Pressed = false;
            }

            if (digitalRead(BUTTON2_EXIT) == 0) {
                if (!button2_exit_pressed) {
                    button2_exit_pressed = true;
                    page_no = Display::Page::PAGE_PARAM_LIST;
                
                } else {
                    button2_exit_pressed = false;
                }            
            }
            display.show(param_value, 1);        
            break;
        case Display::Page::PAGE_MANUAL:
            // Handle button press to turn relay on only once until released
            int relay_no = functionNo-1;

            if (digitalRead(BUTTON2_EXIT) == 0) {
                if (!button2_exit_pressed) {
                    button2_exit_pressed = true;
                    page_no = Display::Page::PAGE_ALARM_LIST; rotaryEncoder.setBoundaries(1, ALARM_COUNT, true );
                }
            } else {
                button2_exit_pressed = false;
            }
            if(functionNo <= RELAY_COUNT) // ilk parametreler rölelerin açılıp kapanması için.
            {
                // Handle button press to turn relay on only once until released
                relay_no = functionNo;
                if (digitalRead(BUTTON1_ENTER) == 0) {
                    if (!button1_enter_Pressed) {
                        button1_enter_Pressed = true;
                        if (relay[relay_no-1].status == Relay::RELAY_OFF) {
                            relay[relay_no-1].TurnOn();
                        } else {
                            relay[relay_no-1].TurnOff();
                        }
                    }
                } else {
                    button1_enter_Pressed = false;
                }
                display.showManuelRelayStatus(relay_no, relay[relay_no-1].status);
            } 
            else if (functionNo == RELAY_COUNT + 1) // Show IP address
            {
                if (screenOn) {
                    String ipAddress = WiFi.localIP().toString();
                    display.showIPAddress(ipAddress.c_str());
                }
            }
            break;
    }

    for (int i = 0; i < 8; i++) {
        alrm[i].Update(now);
    }

	String status = "OK";
	long reading = 0;
	int mode = 1;



    //buraya RELAY1 pinini her saniye açıp kapayacak kod yaz:
    // relay[0].TurnOn();
    // delay(1000);
    // relay[0].TurnOff();
    // delay(1000);

    // relay[1].TurnOn();
    // delay(1000);
    // relay[1].TurnOff();
    // delay(1000);

    // relay[2].TurnOn();
    // delay(1000);
    // relay[2].TurnOff();
    // delay(1000);

    // relay[3].TurnOn();
    // delay(1000);
    // relay[3].TurnOff();
    // delay(1000);




    if (!now.IsValid())
    {
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
		String status = "F01";		
		//printTimeAndAlarm(now, RtcDateTime(2000,1, 1, alarmHr, alarmMin, 0), status, measurePressure(), mode);
        Serial.println("RTC lost confidence in the DateTime!");
    }

    updateRelaysFromFirebase();
    server.handleClient();
}
