/*Описание того, что требуется хранить на флешке: 
 * 1. Тип подключения к вайвай (точка доступа или к роутеру)
 * SSID/пароли точки доступа и роутера
 * Число неудачных включений (неудачное, если не было подключения к вайвай)
 * Мастер-ключ для шифрования (может быть дополнительно набор ключей для выбора по времени)
 * Имя устройства для передачи
 * Параметры LoRA (ширина канала, длина префикса, частота)
 * Двухмерый массив последних 100 сообщений + датавремя, дополнительно число заюзаных слотов, номер текущего (чтобы в правильном порядке показывать)
 * мелодии, хз
 * первым делом пишем интерфейс, для этого комментим всю лору и все часики, сначала всё читаем и вырпезаем лишнее/непонятное
 * есть мысль подгружать футер и хедер по отдельности с другими гетами, чтобы тело грузить с помощью жаваскрипта
 *             <span class="u1 chat">Hey sam, whats up?</span>
            <span class="u2 chat">nothing here how 'bout u?</span>
            <span class="u1 chat">I'm just heading up to your neck of the woods for some work related stuff.</span>
            <span class="u1 chat">driveing through mcdonalds right now</span>
            <span class="u2 chat">cool! where exactly will you be?</span>

            <span class="u1 chat">up in the ridgway, mt. horab area</span>
<span class="sentinfo">time</span>
*/
#include <SPI.h> //LoRa подключается по SPI
#include <LoRa.h> 
#include "FS.h" 
#include <Wire.h> 
#include <RTClib.h> 
#include <ESP8266WiFi.h> 
#include <WiFiClient.h> 
#include <NTPClient.h> 
#include <WiFiUdp.h> 
#include <ESP8266WebServer.h> 
#include <ESP8266HTTPUpdateServer.h> 

#define OTAUSER         "cu"    // Set OTA user
#define OTAPASSWORD     "123"    // Set OTA password
#define OTAPATH         "/fw-upgrade"// Set path for update

#define ss 5 //ноги для  LORA
#define rst 0
#define dio0 4

const char rootheader0[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>Радиочат</title>
<meta name="viewport" content="width=device-width, user-scalable=no, initial-scale=1">
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<script type = "text/javascript">  
function auto_height(elem) { 
    elem.style.height = "1px";
    elem.style.height = (elem.scrollHeight)+"px";
}
</script>  
<style>
* {box-sizing: border-box;}
body { 
  margin: 0;
  font-family: Arial, Helvetica, sans-serif;
}
.header {
  overflow: hidden;
  background-color: #6600ff;  
}
.header a {
  float: left;
  color: white;
  text-align: center;  
  text-decoration: none;
  font-size: 18px; 
  line-height: 30px;
  border-radius: 4px;  
  font-weight: bold;
}
.header a.logo {
  font-size: 25px;  
}
.header a:hover {
  background-color: #000;    
}
.header-right {
  float: right;
}
.chat{
    background: #72b8ff;
    border-radius: 10px;
    display: inline-block;
    padding: 10px;
    color: #fff;
    font-weight: lighter;    
    margin: 5px;
    position: relative;
}
.chat.u1{
    float: left;
    clear: both;
    border-top-left-radius: 0px;
}
.sentinfo{
    float: right;
    color: #555;
    font-size: small;
}
.recinfo{
    float: left;
    color: #555;
    font-size: small;
}
.chat.u2{
    float: right;
    clear: both;
    border-top-right-radius: 0px;
    background: #00D025;
}
footer {
  text-align: center;  
  background-color: #6600ff;
  color: white;
  position: absolute;
  bottom: 0;
  width: 100%;
}
)=====";
const char rootheader1[] PROGMEM = R"=====(
form{
display:flex
}
textarea {
  display: inline-block;
  resize:none;
  width: 83%;
  padding: 8px 20px;
  margin: 8px 5px;  
  border: 1px solid #ccc;
  border-radius: 4px;
  box-sizing: border-box;
}
input[type=submit] {
  display:inline-block;
  width: 13%;
  background-color: #000;
  color: white;
  padding: 8px 0;
  margin: 8px 2px;
  border: 2px solid #fff;
  border-radius: 4px;
  cursor: pointer;
}
</style>
</head>
<body>
<div class="header">
  <a href="/" class="logo">Radiochat</a>
  <div class="header-right">
    <a href="/fw-upgrade">&#160;^&#160;</a>
    <a href="/settings">&#160;&#160;&#9776;&#160;&#160;</a>
  </div>
</div>
)=====";
const char rootfooter[] PROGMEM = R"=====(
<footer>
<form action="/" method="post">
<textarea rows="1" class="auto_height" oninput="auto_height(this)" name="data" placeholder="Сообщение..." maxlength="250"></textarea>
<input type="submit" value=">>">
</form>
</footer>
</body>
</html>
)=====";

boolean isAP = true; 
boolean SoundON = true;
boolean LEDON = true;
byte lastmessage = 0;
boolean fullmessagearray = false; //если больше 255 сообщений напосылалось
byte txpower = 20;
byte spreadingfactor = 7;
byte signalbandwith = 7;
byte codingratedenominator = 5;
byte preamblelength = 8;
byte syncword = 0x34;
boolean crc = true;
const byte truebyte = 255;
const byte falsebyte = 0;

byte failedstartcount = 0;
boolean isfailedstart = true;
String ssid = "SSID";
String WPA = "WPA123456";
unsigned long looptime = 0;
boolean somethingsend = false;
String debuginfo = "";

FSInfo fs_info;
Dir dir;
File f;
RTC_DS3231 rtc;

const long utcOffsetInSeconds = 3600*7; //поправка на часовой пояс в секундах

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void handleNotFound(){
String message = "File Not Found\n\n";
message += "URI: ";
message += server.uri();
message += "\nMethod: ";
message += (server.method() == HTTP_GET)?"GET":"POST";
message += "\nArguments: ";
message += server.args();
message += "\n";
for (uint8_t i=0; i<server.args(); i++){
message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
}
server.send(404, "text/plain", message);
}

bool byte2bool(byte state){  
  return state == truebyte;
}

byte bool2byte(bool state){  
  if(state) 
  return truebyte;
  else
  return falsebyte;
}

void webcontent() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200,"text/html",rootheader0);
  server.sendContent(rootheader1);  
  
  int msg_id = 0;
  String contentpart = "";
  if (lastmessage!=0 or !fullmessagearray){        
    String msg_text = "";
    String msg_attr = "";
    bool msg_was_sent = true;
    while (msg_id < lastmessage) {   
      if(SPIFFS.exists("/msg_id_"+String(msg_id))) // проверка есть ли файл
       {         
         f = SPIFFS.open("/msg_id_"+String(msg_id), "r");
         if(f) // проверка открылся ли файл
         {                            
             msg_text = f.readString();                
             f.close();    
         }
       }
      if(SPIFFS.exists("/msg_attr_"+String(msg_id))) // проверка есть ли файл
       {         
         f = SPIFFS.open("/msg_attr_"+String(msg_id), "r");
         if(f) // проверка открылся ли файл
         {                       
           msg_was_sent = byte2bool(f.read());
           msg_attr = f.readString();                
           f.close();    
         }
       }
       if (msg_was_sent){
        contentpart += "<span class=\"u1 chat\">"+msg_text+"</span><span class=\"sentinfo\">"+msg_attr+"</span>";
       } else
       {
        contentpart += "<span class=\"u2 chat\">"+msg_text+"</span><span class=\"recinfo\">"+msg_attr+"</span>";
       }
       msg_id += 1;
       if (msg_id % 6 == 0){
        server.sendContent(contentpart);
       }
    }
  }
    if (msg_id % 6 != 0){
        server.sendContent(contentpart);
       }
       server.sendContent("<div>член</div>");
  server.sendContent(debuginfo);
  server.sendContent(rootfooter);
}

void recMsg(int packetSize){
if (packetSize == 0) return;

  String msg_text = "";
  String msg_rssi = "";
  while (LoRa.available()) {      
      msg_text += (char)LoRa.read();      
      //fixme добавить расшифровку      
    }
    msg_rssi = String(LoRa.packetRssi());
   DateTime now = rtc.now();
   char buff[] = "DD.MM.YYYY hh:mm:ss";
   String msg_time = now.toString(buff);
    f = SPIFFS.open("/msg_id_"+String(lastmessage), "w");
    f.print(msg_text); //fixme устанавливать владельца
    f.close();
    f = SPIFFS.open("/msg_attr_"+String(lastmessage), "w");
    f.write(bool2byte(false)); //получено
    f.print(msg_time + " | "); //fixme
    f.print(msg_rssi);
    f.close();
    lastmessage +=1;
    if (lastmessage == 255){
      lastmessage = 0;
      fullmessagearray = true;
    }
    f = SPIFFS.open("/init.bin", "w"); 
    f.write(bool2byte(isAP)); //isAP
    f.write(bool2byte(SoundON)); //SoundON
    f.write(bool2byte(LEDON)); //LEDOn
    f.write(lastmessage); 
    f.write(bool2byte(fullmessagearray));  //fullmessagearray
    f.write(txpower); //txPower - TX power in dB, defaults to 17 (17,20)
    f.write(spreadingfactor); //spreadingFactor - spreading factor, defaults to 7 (6..12)
    f.write(signalbandwith); //signalBandwidth - signal bandwidth in Hz, defaults to 125E3 (7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3)
    f.write(codingratedenominator); //codingRateDenominator - denominator of the coding rate, defaults to 5 (5..8)
    f.write(preamblelength); //preambleLength - preamble length in symbols, defaults to 8 (6..65535)
    f.write(syncword); //syncWord - byte value to use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.close(); // закрыли файл
    debuginfo += "<span class=\"sentinfo\">lastmessage: "+String(lastmessage)+"</span>";
    debuginfo += "<span class=\"sentinfo\">fullmessagearray: "+String(fullmessagearray)+"</span>";
    //ыщо туд можно попикать и поморгать, хз
  
    
        
}

void sendMsg(String msg){
    LoRa.beginPacket(); //fixme добавить шифрование
    LoRa.print(msg);    
    LoRa.endPacket();
    f = SPIFFS.open("/msg_id_"+String(lastmessage), "w");
    f.print(msg);
    f.close();
    f = SPIFFS.open("/msg_attr_"+String(lastmessage), "w");
    f.write(bool2byte(true)); //отправлено
    f.write("когда?"); //fixme
    f.close();
    lastmessage += 1;
    if (lastmessage == 255){
      lastmessage = 0;
      fullmessagearray = true;
    }
    f = SPIFFS.open("/init.bin", "w"); 
    f.write(bool2byte(isAP)); //isAP
    f.write(bool2byte(SoundON)); //SoundON
    f.write(bool2byte(LEDON)); //LEDOn
    f.write(lastmessage); 
    f.write(bool2byte(fullmessagearray));  //fullmessagearray
    f.write(txpower); //txPower - TX power in dB, defaults to 17 (17,20)
    f.write(spreadingfactor); //spreadingFactor - spreading factor, defaults to 7 (6..12)
    f.write(signalbandwith); //signalBandwidth - signal bandwidth in Hz, defaults to 125E3 (7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3)
    f.write(codingratedenominator); //codingRateDenominator - denominator of the coding rate, defaults to 5 (5..8)
    f.write(preamblelength); //preambleLength - preamble length in symbols, defaults to 8 (6..65535)
    f.write(syncword); //syncWord - byte value to use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.close(); // закрыли файл
    debuginfo += "<span class=\"sentinfo\">lastmessage: "+String(lastmessage)+"</span>";
    debuginfo += "<span class=\"sentinfo\">fullmessagearray: "+String(fullmessagearray)+"</span>";    
}

void handleSubmit() { //проверка ответов? .toInt  
  if (server.hasArg("data")) { /////////NAME of form is hasArg    
    String msg_text = server.arg("data"); //fixme добавить шифрование
          sendMsg(msg_text);     
          webcontent();
    }
}

  
void handleRoot() {
      if (server.args() >= 1 ) {  
    handleSubmit();
  } else {
  webcontent();  
  }
}

void handleSettings() {  
  server.send(200, "text/html", "<html><head><title>член</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><meta http-equiv=\"refresh\" content=\"15\"></head><body><a href=\"/fw-upgrade\">Oбновить ПО</a><h1>будешь?</h1></body></html>");
}

void factoryreset(){
    SPIFFS.format();
    delay(100);
    f = SPIFFS.open("/init.bin", "w"); 
    f.write(bool2byte(isAP)); //isAP
    f.write(bool2byte(SoundON)); //SoundON
    f.write(bool2byte(LEDON)); //LEDOn
    f.write(lastmessage); 
    f.write(bool2byte(false));  //fullmessagearray
    f.write(txpower); //txPower - TX power in dB, defaults to 17 (17,20)
    f.write(spreadingfactor); //spreadingFactor - spreading factor, defaults to 7 (6..12)
    f.write(signalbandwith); //signalBandwidth - signal bandwidth in Hz, defaults to 125E3 (7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3)
    f.write(codingratedenominator); //codingRateDenominator - denominator of the coding rate, defaults to 5 (5..8)
    f.write(preamblelength); //preambleLength - preamble length in symbols, defaults to 8 (6..65535)
    f.write(syncword); //syncWord - byte value to use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.close(); // закрыли файл
    
    f = SPIFFS.open("/apssid", "w");         
    f.write("Radio");
    f.close();
    
    f = SPIFFS.open("/apwpa", "w"); 
    f.write("00000000");
    f.close();
    
    f = SPIFFS.open("/clientssid", "w"); 
    f.write("SSID");
    f.close();
    
    f = SPIFFS.open("/clientwpa", "w"); 
    f.write("WPA123456");
    f.close();
    
    f = SPIFFS.open("/failedstart", "w"); 
    f.write(bool2byte(isfailedstart));
    f.write(0);
    f.close();
    
    
    
    for (int i = 0; i <= 255; i++) {      
        String nameofmessagefile = "/msg_id_"+String(i);
        f = SPIFFS.open(nameofmessagefile, "w");         
        f.write(i);
        f.close();        
        Serial.print(i);
        Serial.println(" number of files generated");        
        String nameofmessageattr = "/msg_attr_"+String(i);
        f = SPIFFS.open(nameofmessageattr, "w");         
        f.write(i);
        f.close();               
  }
  ESP.restart();
}

void setup() {
  pinMode(D8, OUTPUT);  //uncomment 4 tx rx
  pinMode(TX, FUNCTION_3); //sda
  pinMode(RX, FUNCTION_3); //scl
  Wire.begin(TX,RX);
  delay(100); 
    WiFi.mode(WIFI_STA);
    WiFi.begin("SSID", "WPA123456"); //factory wi-fi hardcode
    SPIFFS.begin();  
    if(SPIFFS.exists("/init.bin")) // проверка есть ли файл
       {         
         f = SPIFFS.open("/init.bin", "r");
         if(f) // проверка открылся ли файл
         {           
           if(f.size()==12) // если файл не пустой и не битый
           {     
             //читаем всё          
                isAP = byte2bool(f.read());
                SoundON = byte2bool(f.read());
                LEDON = byte2bool(f.read());
                lastmessage = f.read();
                fullmessagearray = byte2bool(f.read());
                txpower = f.read();
                spreadingfactor = f.read();
                signalbandwith = f.read();
                codingratedenominator = f.read();
                preamblelength = f.read();
                syncword = f.read();
                crc = byte2bool(f.read());
                f.close();                                 
           }
           else {
           factoryreset(); //wrong size
           }
         }
         else {
               factoryreset();  //файл не может открыться
        }
       } else factoryreset(); //файла нет
       if(SPIFFS.exists("/failedstart")){
            f = SPIFFS.open("/failedstart", "r");
            if(f){
                isfailedstart = byte2bool(f.read());
                failedstartcount = f.read();
                f.close();
                } else factoryreset();
            }
       else factoryreset();
       delay(1000);
       if (WiFi.status() != WL_CONNECTED) {
        delay(3000);
        if (WiFi.status() != WL_CONNECTED){
        WiFi.disconnect();
        
       if (isAP){
                    if(SPIFFS.exists("/apssid")){
                    f = SPIFFS.open("/apssid", "r");
                      if(f){
                        ssid = f.readString();
                        f.close();
                      } else factoryreset();
                    } else factoryreset();
                    if(SPIFFS.exists("/apwpa")){
                    f = SPIFFS.open("/apwpa", "r");
                    if(f){
                        WPA = f.readString();
                        f.close();
                    } else factoryreset();
                    WiFi.softAP(ssid, WPA);
                    } else factoryreset();
       } else {
                    if(SPIFFS.exists("/clientssid")){
                    f = SPIFFS.open("/clientssid", "r");
                      if(f){
                        ssid = f.readString();
                        f.close();
                      } else factoryreset();
                    } else factoryreset();
                    if(SPIFFS.exists("/clientwpa")){
                    f = SPIFFS.open("/clientwpa", "r");
                    if(f){
                        WPA = f.readString();
                        f.close();
                    } else factoryreset();                    
                    } else factoryreset();
                    WiFi.mode(WIFI_STA);
                    WiFi.begin(ssid, WPA);
                }
       }
       }
       
       
        isfailedstart = true;
        f = SPIFFS.open("/failedstart", "w"); 
        f.write(bool2byte(isfailedstart));
        failedstartcount = failedstartcount + 1;
        f.write(failedstartcount);
        f.close();
        if(failedstartcount >= 3){
            factoryreset();
        }
        
        
  while (WiFi.status() != WL_CONNECTED) {
  delay(5000); //если висит, то тут.
  }
if (! rtc.begin()) { //uncomment 4 rtc
    while (1) delay(10);
      if (rtc.lostPower()) {    
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(2022, 10, 20, 3, 55, 10));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  }
  //IPAddress myIP = WiFi.softAPIP();
  server.on("/", handleRoot);  
  server.on("/settings", handleSettings);  
  server.begin();
  httpUpdater.setup(&server, OTAPATH, OTAUSER, OTAPASSWORD);  
  LoRa.setPins(ss, rst, dio0); //uncomment 4 lora
  if (!LoRa.begin(433E6)) {
    delay(4000);
  }

}



void loop() {  
  looptime = millis();
  if (looptime > 5000 and isfailedstart){
        isfailedstart = false;
        f = SPIFFS.open("/failedstart", "w"); 
        f.write(bool2byte(isfailedstart));
        failedstartcount = 0;
        f.write(failedstartcount);
        f.close();
    }
  server.handleClient();
  recMsg(LoRa.parsePacket());
}
