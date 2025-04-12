#include "Alarm.h"



void (*Alarm::statusChangeCallback)(int alarmNo, Alarm::AlarmStatus newStatus) = nullptr;

void Alarm::initFirstAlarm(EEPROMClass &eprom)
{
  eprom.begin(total_memory);
  int default_parameters[] = {21,0,2,1,1,1,1,1,1,22,100,0,0,0}; // parametre sayısı değiştiğinde parametre_count değişkenini değiştirmek gerekiyor.   
  int bank = 0;
  int offset = address(bank);
  for(int param = 0; param < parametre_count; param++)
  {
    Serial.print("Param No:" ); Serial.println(param);
    Serial.print("Param Value:" ); Serial.println(default_parameters[param]);
    eprom.write(offset + param, default_parameters[param]);
  }
  eprom.commit();
}

String Alarm::toJson() {
  StaticJsonDocument<512> doc; // Adjust size as needed
  doc["alarmno"] = alarmno;
  doc["alarm_hour"] = alarm_hour;
  doc["alarm_minute"] = alarm_minute;
  doc["repeat_count"] = repeat_count;
  doc["run_minutes"] = run_minutes;
  doc["idle_minutes"] = idle_minutes;
  doc["idle_time"] = idle_time;
  doc["day_period"] = day_period;
  doc["relay_no"] = relay_no;
  doc["last_run_day"] = last_run_day;
  doc["last_run_month"] = last_run_month;
  doc["last_run_year"] = last_run_year;
  doc["pressure"] = pressure;
  doc["trigger_type"] = trigger_type;
  doc["alarm_status"] = alarm_status;
  doc["repeat_count_remaining"] = repeat_count_remaining;
  doc["run_remaining_minutes"] = run_remaining_minutes;
  doc["idle_remaining_minutes"] = idle_remaining_minutes;

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

void Alarm::initEPROM(EEPROMClass &eprom)
{
  Serial.println("initEPROM");    
  Serial.println("---------");    
  
  eprom.begin(total_memory);
  Serial.print("total_memory:" ); Serial.println(total_memory);    
  eprom.write(0,bank_count); //0 adresinde toplam kaç alarm kurulu olduğu tutulur.  1 ve 2 rezerve edilmiş.
  Serial.print("total bank_count:" ); Serial.println(bank_count);    
  //----------- 1 numaları alarm.
  //
  //int default_parameters[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13}
  int default_parameters[] = {21,0,0,15,30,1,1,1,1,22,100,0,0,0}; // parametre sayısı değiştiğinde parametre_count değişkenini değiştirmek gerekiyor. 
  Serial.println("initAlarms");  
  for (int bank = 0; bank < bank_count; bank++) 
  {
    int offset = address(bank);
    Serial.print("Bank:" ); Serial.println(bank);    
    Serial.print("Offset:" ); Serial.println(offset);    
    for(int param = 0; param < parametre_count; param++)
        {
          Serial.print("Param No:" ); Serial.println(param);
          Serial.print("Param Value:" ); Serial.println(default_parameters[param]);
          eprom.write(offset + param, default_parameters[param]);
        }
  }

  eprom.commit();
  Serial.println(" * write check * ");
  int result = eprom.read(0);
  Serial.println(result);
  result = eprom.read(3);
  Serial.println(result);
  result = eprom.read(56);
  Serial.println(result);
  Serial.println("---------------");


  /*
  int offset = address(0);
  Serial.println("initAlarms");
  Serial.println(offset);
  eprom.write(offset + 0, 21);   //0  21:00  _alarm_hour
  eprom.write(offset + 1,  0);   //1         _alarm_minute
  eprom.write(offset + 2,  4);   //2  4 defa tekrarla _repeat_count
  eprom.write(offset + 3, 15);   //3 15 dakika çalış _run_time
  eprom.write(offset + 4, 30);   //4 30 dakika dinlen _idle_time
  eprom.write(offset + 5, 1);    //5 Her gün çalış _day_period
  eprom.write(offset + 6, 1);    //6 relay no 1 (her bir relay için birden fazla alarm tanımlaması yapılabilir.) _relayNo
  eprom.write(offset + 7, 1);    //7 01 / 01 / 2022 tarihinde en son çalışmış. _last_run_day
  eprom.write(offset + 8, 1);    //8 _last_run_month
  eprom.write(offset + 9, 22);   //9 _last_run_year
  eprom.write(offset + 10, 100); //10 basınç 100 altında olursa çalışmayı duraklat veya durdur. _pressure
  eprom.write(offset + 11, 0);   //11 0 time trigger  1 pressure trigger. Basınç değeri belli bir değerin üzerinde ise çalışmaya başlar.

  //----------- 2 numaları alarm.  
  offset = address(1);
  Serial.println(offset);
  eprom.write(offset + 0, 21);   //0  21:30
  eprom.write(offset + 1, 30);   //1
  eprom.write(offset + 2,  3);   //2  3 defa tekrarla
  eprom.write(offset + 3, 10);   //3 10 dakika çalış
  eprom.write(offset + 4, 20);   //4 20 dakika dinlen
  eprom.write(offset + 5, 2);    //5 Gün aşırı çalış
  eprom.write(offset + 6, 2);    //6 relay no 2 (her bir relay için birden fazla alarm tanımlaması yapılabilir.)
  eprom.write(offset + 7, 1);    //7 01 / 01 / 2022 tarihinde en son çalışmış.
  eprom.write(offset + 8, 1);    //8
  eprom.write(offset + 9, 22);   //9
  eprom.write(offset + 10, 130); //10 basınç 130 altında olursa çalışmayı duraklat veya durdur.}
  eprom.write(offset + 11, 1);   //11 0 time trigger  1 pressure trigger. Basınç değeri belli bir değerin üzerinde ise çalışmaya başlar. ya da belli bir saat geldiğinde çalışmaya başlar.
  eprom.commit();
  Serial.println("---------------");
  int result = eprom.read(0);
  Serial.println(result);
  result = eprom.read(3);
  Serial.println(result);
*/
}

Alarm::Alarm(int _alarmno)
{
  Serial.begin(115200);
  alarmno = _alarmno;
  Serial.println("Alarm::Alarm");
  Serial.println(_alarmno);
  alarm_status = Alarm::AlarmStatus::ALARM_STATUS_STOPPED;
  oldAlarmStatus = alarm_status;
  taskStatus = false;
}


long calculateTotalMinutes(const RtcDateTime& dt) {
  return dt.Year() * 525600 + dt.Month() * 43800 + dt.Day() * 1440 + dt.Hour() * 60 + dt.Minute();
}

void Alarm::Log(String stimeDifferenceMinutes, String rtcTotalMinutes)
{
  //aynı string gelmiş ise logu yazma
  String log = 
  "Alarm:" + String(alarmno) + 
  " Status:" + String(alarm_status) + 
  " Run Remaining:" + String(run_remaining_minutes) + 
  " Idle Remaining:" +  String(idle_remaining_minutes) + 
  " Repeat Remaining:" +  String(repeat_count_remaining) + 
  " Repeat:" +  String(repeat_count) + 
  " Idle:" +  String(idle_minutes) + 
  " Run:" +  String(run_minutes) + 
  " rtcAlarmStartTotalMinutes:" + String(rtcAlarmStartTotalMinutes) + 
  " rtcTotalMinutes:" + rtcTotalMinutes +
  " stimeDifferenceMinutes:" + stimeDifferenceMinutes ;
  if(alarmno==0)
  {
    if (previousLog == log)
    {
      return;
    }
    Serial.println(log);
    previousLog = log;    
  }
}

void Alarm::RunCycle(RtcDateTime rtc)
{
    int clockMin = rtc.Minute();
	int clockHr = rtc.Hour();
    long rtcTotalMinutes;
    long timeDifferenceMinutes;
    //Serial.println("Alarm::RunCycle");
    if((clockMin==alarm_minute) && (clockHr == alarm_hour))
    {
        taskStatus = true;
        //Serial.println("Alarm::Update:isTrigger");
        alarm_status = Alarm::AlarmStatus::ALARM_STATUS_RUNNING;
        alarm_start_time = RtcDateTime(rtc.Year(), rtc.Month(), rtc.Day(), alarm_hour, alarm_minute, 0);  
        rtcAlarmStartTotalMinutes = calculateTotalMinutes(alarm_start_time);
        repeat_count_remaining = repeat_count;
        run_remaining_minutes = run_minutes;
        idle_remaining_minutes = idle_minutes;        
    }        
    rtcTotalMinutes = calculateTotalMinutes(rtc); //bu rutin sürekli olarak çağrıldığı için buradaki rtc değerini gerçek zaman olarak düşünebiliriz.
    //aşağıdaki kod şunu yapar: Her bir iterasyonda alarm yeniden başlamış gibi hesaplaması için iterasyon sayısı ile iterasyon süresi çarpılır.
    timeDifferenceMinutes = rtcTotalMinutes - (rtcAlarmStartTotalMinutes + ((run_minutes + idle_minutes) * (repeat_count - repeat_count_remaining)));            
    //  ****  Log(String(timeDifferenceMinutes), String(rtcTotalMinutes));
    switch (alarm_status)
    {
      case Alarm::AlarmStatus::ALARM_STATUS_RUNNING:
        run_remaining_minutes = run_minutes - timeDifferenceMinutes;
        if (run_remaining_minutes==0)
        {
          alarm_status = Alarm::AlarmStatus::ALARM_STATUS_WAITING;
        }
        break;
      case Alarm::AlarmStatus::ALARM_STATUS_WAITING:
        idle_remaining_minutes = (run_minutes + idle_minutes) - timeDifferenceMinutes;
        if (idle_remaining_minutes==0)
        {
          if(repeat_count_remaining > 1)
          {
            repeat_count_remaining--;
            alarm_status = Alarm::AlarmStatus::ALARM_STATUS_RUNNING;
            run_remaining_minutes = run_minutes;
            idle_remaining_minutes = idle_minutes;
          }
          else
          {
            alarm_status = Alarm::AlarmStatus::ALARM_STATUS_STOPPED;
            taskStatus = false;
          }
        }
        break;
      default:
        break;
    }
}

void Alarm::Update(RtcDateTime rtc)
{
  //Serial.println("Alarm::Update");

  if (oldAlarmStatus != alarm_status) // alarm durumu değiştiğinde
  {
    oldAlarmStatus = alarm_status;
    if (statusChangeCallback) statusChangeCallback(alarmno, alarm_status);
  }

  int lastRunDays;
  int currentDays;
  int diffDays;
  RtcDateTime lastRun(last_run_year, last_run_month, last_run_day, 0, 0, 0);
  lastRunDays = calculateTotalMinutes(lastRun) / 1440;
  currentDays = calculateTotalMinutes(rtc) / 1440;  
  diffDays = currentDays - lastRunDays;  

  switch(day_period)    
  {
    case AlarmFrequency::ALARM_FREQUENCY_OFF:
        break;
    case AlarmFrequency::ALARM_FREQUENCY_EVERYDAY:
        RunCycle(rtc);
        break;
    case AlarmFrequency::ALARM_FREQUENCY_DAYAFTER:// bir gün sula bir gün dur
        //Serial.println("ALARM_FREQUENCY_DAYAFTER");
        //Serial.print("Gün farkı: ");
        //Serial.println(diffDays);
        if(diffDays > 2)
        {
            RunCycle(rtc);
        }
        break;
    case AlarmFrequency::ALARM_FREQUENCY_TWODAYSAFTER:
        //Serial.println("ALARM_FREQUENCY_TWODAYSAFTER");
        //Serial.print("Gün farkı: ");
        //Serial.println(diffDays);
        if(diffDays > 3)
        {
            RunCycle(rtc);
        }
        break;    
  }
}



void Alarm::SaveAlarm()
{

}

int Alarm::address(int alrm_no)
{
  // 0  : total number of alarms
  // 1,2: rezerved
  // 3-
   int base_address = 0;
   int offset = base_address + alrm_no * parametre_count + 3;
   return offset;
}



int Alarm::GetParamValue(int param_no){
  int value;
  switch(param_no) {
    case 1:
        //param_name = "Alarm Hour";  
        value = alarm_hour;
        break;
    case 2:
        //param_name = "Alarm Minute";
        value = alarm_minute;
        break;
    case 3:
        //param_name = "Repeat Count";
        value = repeat_count;
        break;
    case 4:
        //param_name = "Run Minutes";
        value = run_minutes;
        break;
    case 5:
        //param_name = "Idle Minutes";
        value = idle_minutes;
        break;
    case 6:
        //param_name = "Day Period";
        value = day_period;
        break;
    case 7:
        //param_name = "Relay No";
        value = relay_no;
        break;
    case 8:
        //param_name = "Last Run Day";
        value = last_run_day;        
        break;  
    case 9:
        //param_name = "Last Run Month";
        value = last_run_month;
        break;  
    case 10:
        //param_name = "Last Run Year";
        value = last_run_year;
        break;  
    case 11:
        //param_name = "Pressure";
        value = pressure;
        break;  
    case 12:
        //param_name = "Trigger Type";
        value = trigger_type;
        break;  
    case 13:
        //param_name = "Alarm Status";
        value = alarm_status;
        break;
    default:
        return value; // Invalid parameter number, do nothing
}  
  return value;
}

String Alarm::GetParamName(int param_no){
  String param_name = "";
  switch(param_no) {
      case 1:
          param_name = "Alarm Hour";  
          break;
      case 2:
          param_name = "Alarm Minute";
          break;
      case 3:
          param_name = "Repeat Count";
          break;
      case 4:
          param_name = "Run Minutes";
          break;
      case 5:
          param_name = "Idle Minutes";
          break;
      case 6:
          param_name = "Day Period";
          break;
      case 7:
          param_name = "Relay No";
          break;
      case 8:
          param_name = "Last Run Day";
          break;  
      case 9:
          param_name = "Last Run Month";
          break;  
      case 10:
          param_name = "Last Run Year";
          break;  
      case 11:
          param_name = "Pressure";
          break;  
      case 12:
          param_name = "Trigger Type";
          break;  
      case 13:
          param_name = "Alarm Status";
          break;
      default:
          return param_name; // Invalid parameter number, do nothing
  }
  return param_name;
}

int Alarm::GetParamMinValue(int param_no)
{
  int param_min;
  switch(param_no) {
    case 1:
        //param_name = "Alarm Hour";  
        param_min = 0;
        break;
    case 2:
        //param_name = "Alarm Minute";
        param_min = 0;
        break;
    case 3:
        //param_name = "Repeat Count";
        param_min = 1;
        break;
    case 4:
        //param_name = "Run Minutes";
        param_min = 1;
        break;
    case 5:
        //param_name = "Idle Minutes";
        param_min = 0;
        break;
    case 6:
        //param_name = "Day Period";
        param_min = 0;
        break;
    case 7:
        //param_name = "Relay No";
        param_min = 0;
        break;
    case 8:
        //param_name = "Last Run Day";
        param_min = 1;        
        break;  
    case 9:
        //param_name = "Last Run Month";
        param_min = 1;
        break;  
    case 10:
        //param_name = "Last Run Year";
        param_min = 2022;
        break;  
    case 11:
        //param_name = "Pressure";
        param_min = 0;
        break;  
    case 12:
        //param_name = "Trigger Type";
        param_min = 0;
        break;  
    case 13:
        //param_name = "Alarm Status";
        param_min = 0;
        break;
    default:
        return param_min; // Invalid parameter number, do nothing
}
return param_min;
}

int Alarm::GetParamMaxValue(int param_no)
{
    int param_max;
    switch(param_no) {
      case 1:
          //param_name = "Alarm Hour";  
          param_max = 23;
          break;
      case 2:
          //param_name = "Alarm Minute";
          param_max = 59;
          break;
      case 3:
          //param_name = "Repeat Count";
          param_max = 20;
          break;
      case 4:
          //param_name = "Run Minutes";
          param_max = 60;
          break;
      case 5:
          //param_name = "Idle Minutes";
          param_max = 60;
          break;
      case 6:
          //param_name = "Day Period";
          param_max = 2;
          break;
      case 7:
          //param_name = "Relay No";
          param_max = 4;
          break;
      case 8:
          //param_name = "Last Run Day";
          param_max = 31;        
          break;  
      case 9:
          //param_name = "Last Run Month";
          param_max = 12;
          break;  
      case 10:
          //param_name = "Last Run Year";
          param_max = 2099;
          break;  
      case 11:
          //param_name = "Pressure";
          param_max = 400;
          break;  
      case 12:
          //param_name = "Trigger Type";
          param_max = 1;
          break;
      case 13:
          //param_name = "Alarm Status";
          param_max = 2;
          break;  
      default:
          return param_max; // Invalid parameter number, do nothing
    }
    return param_max;
}

void Alarm::SaveLastDate(int day, int month, int year, EEPROMClass &eprom)
{
  // Set the last run date in the EEPROM
  Serial.println("Alarm::SaveLastDate");
  Serial.println("------------------");
  Serial.print("alarmno:");Serial.println(alarmno);
  Serial.print("day:");Serial.println(day);
  Serial.print("month:");Serial.println(month);
  Serial.print("year:");Serial.println(year);
  eprom.begin(total_memory);
  int offset = address(alarmno);
  eprom.write(offset + 7, day);  
  eprom.write(offset + 8, month);  
  eprom.write(offset + 9, year-2000);  // bir sebeple eprom 2025 gibi bir tarihi kayıt edemedi.
  last_run_day = day;        
  last_run_month = month;
  last_run_year = year;
  eprom.commit();
  Serial.println("Last Run Date Saved");
}

void Alarm::SetParamValue(int param_no, int param_value, EEPROMClass &eprom)
{
  switch(param_no) {
    case 1:
        //param_name = "Alarm Hour";  
        alarm_hour = param_value;
        break;
    case 2:
        //param_name = "Alarm Minute";
        alarm_minute = param_value;
        break;
    case 3:
        //param_name = "Repeat Count";
        repeat_count = param_value;
        break;
    case 4:
        //param_name = "Run Minutes";
        run_minutes = param_value;
        break;
    case 5:
        //param_name = "Idle Minutes";
        idle_minutes = param_value;
        break;
    case 6:
        //param_name = "Day Period";
        day_period = static_cast<Alarm::AlarmFrequency>(param_value);
        break;
    case 7:
        //param_name = "Relay No";
        relay_no = param_value;
        break;
    case 8:
        //param_name = "Last Run Day";
        last_run_day = param_value;        
        break;  
    case 9:
        //param_name = "Last Run Month";
        last_run_month = param_value;
        break;  
    case 10:
        //param_name = "Last Run Year";
        last_run_year = param_value;
        param_value = param_value - 2000; // bir sebeple eprom 2025 gibi bir tarihi kayıt edemedi.
        break;  
    case 11:
        //param_name = "Pressure";
        pressure = param_value;
        break;  
    case 12:
        //param_name = "Trigger Type";
        trigger_type = param_value;
    case 13:
        //param_name = "Alarm Status";
        alarm_status = static_cast<Alarm::AlarmStatus>(param_value);
        break;
      break;  
    default:
      return; // Invalid parameter number, do nothing
  }
  // Save the updated parameter value to EEPROM
  // alarm statüsünü kaydetmeye gerek yok
  if(param_no == 13)
  {
    return;
  }
  eprom.begin(total_memory);
  int offset = address(alarmno);
  eprom.write(offset + param_no-1, param_value);
  eprom.commit();
  Serial.print("Alarm No:");Serial.println(alarmno);
  Serial.print("Param No:");Serial.println(param_no);
  Serial.print("Param value:");Serial.println(param_value);
}

void Alarm::initAlarms(EEPROMClass &eprom)
{
  Serial.println("Alarm::initAlarms");
  eprom.begin(total_memory);
  int offset = address(alarmno);
  alarm_hour = eprom.read(offset + 0);
  alarm_minute = eprom.read(offset + 1);
  repeat_count = eprom.read(offset + 2);
  run_minutes = eprom.read(offset + 3);
  idle_minutes = eprom.read(offset + 4);
  day_period = static_cast<Alarm::AlarmFrequency>(eprom.read(offset + 5));
  relay_no = eprom.read(offset + 6);
  last_run_day = eprom.read(offset + 7);  
  last_run_month = eprom.read(offset + 8);
  last_run_year = eprom.read(offset + 9) + 2000;//bir sebeple eprom 2025 gibi bir tarihi kayıt edemedi.
  pressure = eprom.read(offset + 10);
  trigger_type = eprom.read(offset + 11);
  //alarm_status = static_cast<Alarm::AlarmStatus>(eprom.read(offset + 12));

  Serial.println("Alarm Data Loaded");
  Serial.println("------------------");
  Serial.print("alarmno:");Serial.println(alarmno);
  Serial.print("alarm_hour:");Serial.println(alarm_hour);
  Serial.print("alarm_minute:");Serial.println(alarm_minute);
  Serial.print("repeat_count:");Serial.println(repeat_count);
  Serial.print("run_minutes:");Serial.println(run_minutes);  
  Serial.print("idle_minutes:");Serial.println(idle_minutes);    
  Serial.print("day_period:");Serial.println(day_period);    
  Serial.print("relay_no:");Serial.println(relay_no);  
  Serial.print("last_run_day:");Serial.println(last_run_day);  
  Serial.print("last_run_month:");Serial.println(last_run_month);  
  Serial.print("last_run_year:");Serial.println(last_run_year);  
  Serial.print("pressure:");Serial.println(pressure);  
  Serial.print("trigger_type:");Serial.println(trigger_type);  
  Serial.print("alarm_status:");Serial.println(alarm_status);  
  Serial.println("");
}

void Alarm::SetStatusChangeCallback(void (*callback)(int alarmNo, Alarm::AlarmStatus newStatus))
{
  statusChangeCallback = callback;
}

