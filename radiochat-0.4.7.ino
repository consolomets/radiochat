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
починить iframe/обновления куска+ 
звук, 
настройки
ntp
добавить шифрование
id отправителя / получателя / сообщения + контроль целостности отправки
заводской хардкодинг для подключения к вайвай?
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
//onclick="var x = document.getElementById('sendform').value;document.getElementById('sendform').value='';msgsent(x);"
//
const char header0[] PROGMEM = R"=====(
<!DOCTYPE html>
<head>
<title>Радиочат</title>
<meta name="viewport" content="width=device-width, user-scalable=no, initial-scale=1">
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<script type="text/javascript">  
function msgsent(){
   var formData = new FormData(document.forms.mess); 
   var xhr = new XMLHttpRequest();
  xhr.open("POST", "/");
  xhr.send(formData);
  document.getElementById('sendform').value='';
  //setTimer(document.getElementById('sendform').value='',4000);
}
function auto_height(elem) { 
    elem.style.height = "1px";
    elem.style.height = (elem.scrollHeight)+"px";
}
   function loadCart(){
   fetch('/pageupdate').then(
      response => {
         return response.text();
      }
   ).then(
      text => {
         document.getElementById('im').innerHTML += text;
         if (text !== ""){
         moveWin();
         }
      }
   );
   }   
   function pullup(){
   fetch('/pullup').then(
      response => {
         return response.text();
      }
   ).then(
      text => {
         document.getElementById('im').innerHTML = text + document.getElementById('im').innerHTML;
      }
   );
   }
setInterval(loadCart,5000);
function moveWin()
      {  
        window.scroll(0,10000);        
      }
</script>  
<script>document.addEventListener('DOMContentLoaded',function(e){document.addEventListener('scroll',function(e){let documentHeight=document.body.scrollHeight;let currentScroll=window.scrollY+window.innerHeight;let modifier=200;if(window.scrollY>0&&window.scrollY<10&&document.body.scrollHeight-window.innerHeight>30){pullup();}})})</script>
<style>
body { 
  margin: 0;
  font-family: Roboto, Arial, sans-serif;
  background-color:#1f0040;
}
.header {
  position: fixed;
  top: 0;
  z-index: 999;
  overflow: visible;
  background-color: #6600ff;  
  width:100%;
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
)=====";
const char header1[] PROGMEM = R"=====(
.im {
    margin: auto;    
    padding-top: 2em;
    padding-bottom: 2em;
    padding-left: 5px;
    padding-right: 5px;
    box-sizing: border-box;
    position: relative;
    overflow: hidden;
    background:#1f0040;
}
.chat{

    background: #72b8ff;
    border-radius: 10px;
    display: inline-block;
    padding: 10px;
    color: #fff;
    background:#444;
    font-weight: lighter;    
    margin:5px;
position:related;     
}
.chat.u2{
    float: left;
    clear: both;
    border-top-left-radius: 0px;
    font-weight: bold;
}
.sentinfo{
    float: right;
    color: #aaf;
    font-size: small;
  clear: both;
  margin-top: -3px;
  margin-bottom: 10px;  
}
.recinfo{
    float: left;
    color: #aaf;
    font-size: small;
  clear: both;
  margin-top: -3px;
  margin-bottom: 10px;  
}
.chat.u1{
    float: right;
    clear: both;
    border-top-right-radius: 0px;
    background: #6600ff;
    font-weight: bold;
}
)=====";
const char header2[] PROGMEM = R"=====(
footer {
  text-align: center;  
  background-color: #6600ff;
  color: white;
  position:fixed;
  bottom:0;  
  width: 100%;
}
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
    background-color: #000;
  color: white;
}
button[type=button] {
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
<body onLoad="moveWin();">
<div class="header">
  <a href="/" class="logo">&#10032; Radio.chat</a>
  <div class="header-right">
  <a>&#128267;
)=====";

const char header3[] PROGMEM = R"=====(
% | </a>
    <a href="/fw-upgrade">&nbsp;⬆&nbsp;</a>
    <a href="/settings">&nbsp;&nbsp;☰&nbsp;&nbsp;</a>
  </div>
</div>

<div id="im" class="im">
)=====";

const char footer0[] PROGMEM = R"=====(
</div>
<br>
<footer>
<form id="mess" name="mess" action="/" method="post">
<textarea id="sendform" rows="1" class="auto_height" oninput="auto_height(this)" name="msgtextforsend" placeholder="Сообщение..." maxlength="250"></textarea>
<button type="button" onclick="msgsent()">&#9654;</button>
</form>
</footer>
</body></html>
)=====";


////////////////////////////////////////////////////////////////////////////// settings settings settings settings

const char settings0[] PROGMEM = R"=====(
<!DOCTYPE html>
<head>
<title>Настройки</title>
<meta name="viewport" content="width=device-width, user-scalable=no, initial-scale=1">
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<style>
p,
label {
    font: 1rem 'Fira Sans', sans-serif;
}
input {
    margin: .4rem;
}
</style>

</head>
<body>
<h3>Настройки
</h3>
<a href="/"><- На главную</a><br>
<a href="/default">Заводская конфигурация (сброс всех настроек)</a><br>
<a href="/fw-upgrade">Обновить прошивку</a> | <a href="/debug">Отладочная информация</a> | <a href="/ntp">Обновить время по NTP</a><br>
<a href="/reset">Перезагрузить</a><br>
<form action="/" method="post">
<fieldset>
    <legend>Общие параметры:</legend>
    <div>
    <input type="hidden" value="0" name="gen">
      <input type="checkbox" id="ledon" name="ledon" 
)=====";

const char settings1[] PROGMEM = R"=====(>
      <label for="ledon">Светодиод</label>
    </div>
    <div>
      <input type="checkbox" id="speaker" name="speaker" 
)=====";

const char settings2[] PROGMEM = R"=====(>
      <label for="speaker">Звуки</label>
    </div>
    <input type="submit" value="Сохранить" />
</fieldset>
</form>
<form action="/" method="post">
<fieldset>
    <legend>Системное время:</legend>
    <div>
      <input type="date" id="cdate" name="cdate" value=")=====";

const char settings3[] PROGMEM = R"=====(">
      <label for="cdate">Дата</label>
    </div>
    <div>
      <input type="time" id="ctime" name="ctime" value=")=====";

const char settings4[] PROGMEM = R"=====(">
      <label for="ctime">Время</label>
    </div>
    <input type="submit" value="Сохранить" />
</fieldset>
</form>
<form action="/" method="post">
<fieldset>
    <legend>Шифрование:</legend>
    <div>
    <input type="hidden" value="0" name="cr">
      <input type="checkbox" id="crypt" name="crypt" onchange="this.classList.add('changed'); check(); this.form.submit()" )=====";

const char settings5[] PROGMEM = R"=====(>
      <label for="crypt">Включить шифрование</label>
    </div>
    <div>
      <input type="text" id="masterkey" name="masterkey" size="34" value=")=====";

const char settings6[] PROGMEM = R"=====(">
      <label for="masterkey">Master key</label>
    </div>)=====";

const char settings7[] PROGMEM = R"=====(">
      <label for="tshift">Timeshift</label>
    </div>
    <input type="submit" value="Сохранить" />
</fieldset>
</form>
<br><hr>
<h3>
Wi-Fi
</h3>
<form action="/" method="post">
<fieldset>
    <legend>Режим подключения Wi-Fi:</legend>
    <div>
    <input type="hidden" value="0" name="wf">
      <input type="checkbox" id="client" name="client" onchange="this.classList.add('changed'); check(); this.form.submit()" )=====";

const char settings8[] PROGMEM = R"=====(>
      <label for="client">Клиент</label>
    </div>
    <div>
      <input type="checkbox" id="server" name="server" onchange="this.classList.add('changed'); check(); this.form.submit()" )=====";
      
const char settings88[] PROGMEM = R"=====(">
      <label for="server">Точка доступа</label>
    </div>
</fieldset>
</form>
<fieldset>
<legend>Параметры подключения к Wi-Fi:</legend>
<form action="/" method="post">
<h4>Клиент Wi-Fi</h4>
<label for="clientssid">Имя (SSID)</label>
<input type="text" id="clientssid" name="clientssid" required
       minlength="4" maxlength="32" size="10" value=")=====";

const char settings9[] PROGMEM = R"=====(">
<label for="clientwpa">Пароль</label>
<input type="text" id="clientwpa" name="clientwpa" required
       minlength="8" maxlength="32" size="10" value=")=====";

const char settings10[] PROGMEM = R"=====(">
<h4>Точка доступа Wi-Fi</h4>
<label for="apssid">Имя (SSID)</label>
<input type="text" id="apssid" name="apssid" required
       minlength="4" maxlength="32" size="10" value=")=====";

const char settings11[] PROGMEM = R"=====(">
<label for="apwpa">Пароль</label>
<input type="text" id="apwpa" name="apwpa" required
       minlength="8" maxlength="32" size="10" value=")=====";

const char settings12[] PROGMEM = R"=====(">
       <br>
<input type="submit" value="Сохранить" />
</form>
</fieldset>
<br><hr>
<h3>
LoRa Ra-02 Radio
</h3>
<fieldset>
    <legend>Параметры LoRa (экспериментально):</legend>
   <form action="/" method="post">
<div>
<input type="hidden" value="0" name="lr">
  <input type="number" id="txpower" name="txpower"
         min="2" max="20" value=")=====";

const char settings13[] PROGMEM = R"=====(">
  <label for="txpower">Мощность передатчика</label>
</div>
<div>
  <input type="number" id="spreadingfactor" name="spreadingfactor"
         min="6" max="12" value=")=====";

const char settings14[] PROGMEM = R"=====(">
  <label for="spreadingfactor">Коэффициент распространения Spreading Factor</label>
</div>
<div>
  <input type="number" id="bandwith" name="bandwith"
         min="410" max="525" value=")=====";

const char settings15[] PROGMEM = R"=====(">
  <label for="bandwith">Частота сигнала (МГц) Bandwith</label>
</div>
<div>
  <input type="number" id="denominator" name="denominator"
         min="5" max="8" value=")=====";

const char settings16[] PROGMEM = R"=====(">
  <label for="denominator">Coding rate denominator</label>
</div>
<div>
  <input type="number" id="syncword" name="syncword"
         min="0" max="255" value=")=====";

const char settings17[] PROGMEM = R"=====(">
  <label for="syncword">Кодовый байт SyncWord</label>
</div>
<div>
  <input type="checkbox" id="crc" name="crc")=====";

const char settings18[] PROGMEM = R"=====(>
  <label for="crc">Контрольная сумма</label>
</div>
<input type="submit" value="Сохранить" />
</form>
</fieldset>
</body>
</html>)=====";


boolean isAP = true; 
boolean SoundON = true;
boolean LEDON = true;
byte lastmessage = 0;
boolean fullmessagearray = false; //если больше 255 сообщений напосылалось
byte txpower = 20;
byte spreadingfactor = 7;
int signalbandwith = 7; //
byte codingratedenominator = 5;
byte preamblelength = 8;
byte syncword = 0x34;
boolean crc = true;
const byte truebyte = 255;
const byte falsebyte = 0;
int battlevel = 0;
byte battlevelpercent = 0;
byte beeploop = 0;
String key[11] = {
"SkekNdNue9JWTs7vO2MgJtxa",
"j2lpmj3AlJmknfnAgYzoJFAE",
"6GYD5dj2xLk28qpPsZHRqPg0",
"ot926pyRqYj2mkHQMGkjuRk5",
"P8ToIPBVDQFtVGYqA2SzNjjK",
"0ERSyYtblmnZFPkDEq6fdmAJ",
"Mkx5LJRZeEoR03reTB8fLz0f",
"ZDusvCwNgUeMPRoBcfADpJi6",
"5P9gTSOiGAuW8yaAaq9xKZYl",
"ZXAb5qBMO1dnW7QCPfg966wE",
"iQaHg8Fb36vpEjdSrrluZc3U"
};
unsigned long shiftkey = 93156784;
bool cryptON = true;

byte failedstartcount = 0;
boolean isfailedstart = true;
String ssid = "SSID";
String WPA = "WPA123456";
unsigned long looptime = 0;
unsigned long battlooptime = 0;
unsigned long beeplooptime = 0;
boolean somethingsend = false;

String newmessage = "";
String prevmessage = ""; //костыль
byte screenlastmessage = 0;
String debinfo = "";

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

void pageupdate(){
  server.send(200, "text/html", newmessage);
  newmessage="";
}

void pullup(){
  int msg_id = screenlastmessage;
  if (screenlastmessage < 15 and !fullmessagearray){
    screenlastmessage = 0;    
  } else {
          screenlastmessage -= 15;
         }  
  String contentpart = "";
  String msg_text = "";
  String msg_attr = "";
  bool msg_was_sent = true;
  while (msg_id > screenlastmessage){      
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
        contentpart = "<span class=\"u1 chat\">"+msg_text+"</span><span class=\"sentinfo\">"+msg_attr+"</span>" + contentpart;
       } else
       {
        contentpart = "<span class=\"u2 chat\">"+msg_text+"</span><span class=\"recinfo\">"+msg_attr+"</span>" + contentpart;
       }
       msg_id -= 1;
    }
    server.send(200, "text/html", contentpart);    
  }


void mainpage() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", header0);
  server.sendContent(header1);
  server.sendContent(header2);
  String part = String(battlevelpercent);
  server.sendContent(part);
  server.sendContent(header3);

  ////////////////////////////////////
  screenlastmessage = lastmessage - 15;
  int msg_id = screenlastmessage;
  if (msg_id >= 240 and !fullmessagearray){
    msg_id = 0;
  }

  String contentpart = "";
  if (msg_id>=0 or fullmessagearray){        
    String msg_text = "";
    String msg_attr = "";
    bool msg_was_sent = true;
    bool fullmessagearrayloop = fullmessagearray;
    while (msg_id < lastmessage or fullmessagearrayloop) {   
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
        contentpart = "";
       }
       if (msg_id == 255){
        msg_id = 0;
        fullmessagearrayloop = false;
       }
    }
  }
    if (msg_id % 6 != 0){
        server.sendContent(contentpart);
        contentpart = "";
       }
  ////////////////////////////////////
  server.sendContent(footer0);
}

void recMsg(int packetSize){
if (packetSize == 0) return;

  String msg_text = "";
  String msg_rssi = "";
  String x = "";
  x += (char)LoRa.read();
  if (x == "=") {
  while (LoRa.available()) {      
      msg_text += (char)LoRa.read();      
      //fixme добавить расшифровку      
    }
    msg_rssi = String(LoRa.packetRssi());
   DateTime now = rtc.now();
   char buff[] = "DD.MM.YYYY hh:mm:ss";
   String msg_time = now.toString(buff);
   tone(D8,2000,200);
    f = SPIFFS.open("/msg_id_"+String(lastmessage), "w");
    f.print(msg_text); //fixme устанавливать владельца
    f.close();
    String msg_attr = msg_time + " | " + msg_rssi;
    f = SPIFFS.open("/msg_attr_"+String(lastmessage), "w");
    f.write(bool2byte(false)); //получено
    f.print(msg_attr); //fixme    
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
    f.write(syncword); //syncWord - byte value fto use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.write(bool2byte(cryptON)); //Enable or disable cryptography
    f.write(shiftkey);
    f.close(); // закрыли файл
    newmessage += "<span class=\"u2 chat\">"+msg_text+"</span><span class=\"recinfo\">"+msg_attr+"</span>";
    //ыщо туд можно попикать и поморгать, хз
    tone(D8,4000,50);
  }
        
}



void sendMsg(String msg_text){
  if (msg_text != prevmessage){ //костыль, т.к. веб-форма шлёт дважды...
    
  
    LoRa.beginPacket(); //fixme добавить шифрование
    LoRa.print("=");
    LoRa.print(msg_text);    
    LoRa.endPacket();
    DateTime now = rtc.now();
    char buff[] = "DD.MM.YYYY hh:mm:ss";
    String msg_time = now.toString(buff);
    f = SPIFFS.open("/msg_id_"+String(lastmessage), "w");
    f.print(msg_text);
    f.close(); 

    f = SPIFFS.open("/msg_attr_"+String(lastmessage), "w");
    f.write(bool2byte(true)); //отправлено
    f.print(msg_time); //fixme
    f.close();
    String msg_attr = msg_time; //fixme дошло ли, кто смотрел, рсси
    newmessage += "<span class=\"u1 chat\">"+msg_text+"</span><span class=\"sentinfo\">"+msg_attr+"</span>";
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
    f.write(bool2byte(cryptON)); //Enable or disable cryptography
    f.write(shiftkey);
    f.close(); // закрыли файл
    prevmessage = msg_text;
  } 
  
}

void handleSubmit() { //проверка ответов? .toInt  
  if (server.hasArg("msgtextforsend")) { /////////NAME of form is hasArg    
    String msg_text = server.arg("msgtextforsend"); //fixme добавить шифрование
          sendMsg(msg_text);               
    }
  if (server.hasArg("ledon")) {     
    String msg_text = server.arg("ledon"); 
          debinfo += "<br> ledon radio: " + msg_text;               
    }
  if (server.hasArg("speaker")) {     
    String msg_text = server.arg("speaker"); 
          debinfo += "<br> spk radio: " + msg_text;               
    }
  if (server.hasArg("cdate")) {     
    String msg_text = server.arg("cdate"); 
          debinfo += "<br> cdate: " + msg_text;               
    }
  if (server.hasArg("ctime")) {     
    String msg_text = server.arg("ctime"); 
          debinfo += "<br> ctime: " + msg_text;               
    }
  if (server.hasArg("crypt")) {     
    String msg_text = server.arg("crypt"); 
          debinfo += "<br> crypt: " + msg_text;               
    }
  if (server.hasArg("masterkey")) {     
    String msg_text = server.arg("masterkey"); 
          debinfo += "<br> masterkey: " + msg_text;               
    }
  if (server.hasArg("key0")) {     
    String msg_text = server.arg("key0"); 
          debinfo += "<br> key0: " + msg_text;               
    }
  if (server.hasArg("key1")) {     
    String msg_text = server.arg("key1"); 
          debinfo += "<br> key1: " + msg_text;               
    }
  if (server.hasArg("key2")) {     
    String msg_text = server.arg("key2"); 
          debinfo += "<br> key2: " + msg_text;               
    }
  if (server.hasArg("key3")) {     
    String msg_text = server.arg("key3"); 
          debinfo += "<br> key3: " + msg_text;               
    }
  if (server.hasArg("key4")) {     
    String msg_text = server.arg("key4"); 
          debinfo += "<br> key4: " + msg_text;               
    }
  if (server.hasArg("key5")) {     
    String msg_text = server.arg("key5"); 
          debinfo += "<br> key5: " + msg_text;               
    }
  if (server.hasArg("key6")) {     
    String msg_text = server.arg("key6"); 
          debinfo += "<br> key6: " + msg_text;               
    }
  if (server.hasArg("key7")) {     
    String msg_text = server.arg("key7"); 
          debinfo += "<br> key7: " + msg_text;               
    }
  if (server.hasArg("key8")) {     
    String msg_text = server.arg("key8"); 
          debinfo += "<br> key8: " + msg_text;               
    }
  if (server.hasArg("key9")) {     
    String msg_text = server.arg("key9"); 
          debinfo += "<br> key9: " + msg_text;               
    }
  if (server.hasArg("key10")) {     
    String msg_text = server.arg("key10"); 
          debinfo += "<br> key10: " + msg_text;               
    }
  if (server.hasArg("tshift")) {     
    String msg_text = server.arg("tshift"); 
          debinfo += "<br> tshift: " + msg_text;               
    }
  if (server.hasArg("client")) {     
    String msg_text = server.arg("client"); 
          debinfo += "<br> client: " + msg_text;               
    }
  if (server.hasArg("server")) {     
    String msg_text = server.arg("server"); 
          debinfo += "<br> server: " + msg_text;               
    }
  if (server.hasArg("clientssid")) {     
    String msg_text = server.arg("clientssid"); 
          debinfo += "<br> clientssid: " + msg_text;               
    }
  if (server.hasArg("clientwpa")) {     
    String msg_text = server.arg("clientwpa"); 
          debinfo += "<br> clientwpa: " + msg_text;               
    }
  if (server.hasArg("apssid")) {     
    String msg_text = server.arg("apssid"); 
          debinfo += "<br> apssid: " + msg_text;               
    }
  if (server.hasArg("apwpa")) {     
    String msg_text = server.arg("apwpa"); 
          debinfo += "<br> apwpa: " + msg_text;               
    }
  if (server.hasArg("txpower")) {     
    String msg_text = server.arg("txpower"); 
          debinfo += "<br> txpower: " + msg_text;               
    }
  if (server.hasArg("spreadingfactor")) {     
    String msg_text = server.arg("spreadingfactor"); 
          debinfo += "<br> spreadingfactor: " + msg_text;               
    }
  if (server.hasArg("bandwith")) {     
    String msg_text = server.arg("bandwith"); 
          debinfo += "<br> bandwith: " + msg_text;               
    }
  if (server.hasArg("denominator")) {     
    String msg_text = server.arg("denominator"); 
          debinfo += "<br> denominator: " + msg_text;               
    }
  if (server.hasArg("syncword")) {     
    String msg_text = server.arg("syncword"); 
          debinfo += "<br> syncword: " + msg_text;               
    }
  if (server.hasArg("crc")) {     
    String msg_text = server.arg("crc"); 
          debinfo += "<br> crc: " + msg_text;               
    }
}


  
void handleRoot() {
      if (server.args() >= 1 ) {  
    handleSubmit();
  } else {
  mainpage();  
  }
}

void getap(){
  WiFi.disconnect();
  delay(7000);
  WiFi.softAP(ssid, WPA);
  delay(3000);
  
}

void debug() {  
  String debuginfo = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"></head><body><a href=\"/fw-upgrade\">Oбновить ПО</a>";
  debuginfo += "isAP: " + String(isAP) + "<br>";
  debuginfo += "SoundON: " + String(SoundON) + "<br>";
  debuginfo += "LEDON: " + String(LEDON) + "<br>";
  debuginfo += "fullmessagearray: " + String(fullmessagearray) + "<br>";
  debuginfo += "lastmessage: " + String(lastmessage) + "<br>";
  debuginfo += "millis: " + String(millis()) + "<br>";
   DateTime now = rtc.now();
   char buff[] = "DD.MM.YYYY hh:mm:ss";
   String debugtime = now.toString(buff);
  debuginfo += "current time: " + debugtime + "<br>";
  debuginfo += "ssid: " + String(ssid) + "<br>";
  debuginfo += "wpa: " + String(WPA) + "<br>";
  debuginfo += debinfo + "<br>";
  debuginfo += ": " + String(isAP) + "<br>";
  debuginfo += ": " + String(isAP) + "<br>";
  debuginfo += ": " + String(isAP) + "<br>";
  debuginfo += ": " + String(isAP) + "<br>";
  debuginfo += ": " + String(isAP) + "<br>";
  debuginfo += ": " + String(isAP) + "</body></html>";  


  server.send(200, "text/html", debuginfo);
}

void battery() {  
  String debuginfo = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"></head><body>";
  
   DateTime now = rtc.now();
   char buff[] = "DD.MM.YYYY hh:mm:ss";
   String debugtime = now.toString(buff);
  debuginfo += debugtime + " " + String(battlevel) + "</body></html>";
  server.send(200, "text/html", debuginfo);
}

void handleSettings() {  
  String cssid = "";
  String cwpa = "";
  String apssid = "";
  String apwpa = "";
  if(SPIFFS.exists("/apssid")){
    f = SPIFFS.open("/apssid", "r");
    if(f){
      apssid = f.readString();
      f.close();
    }
  }
    if(SPIFFS.exists("/apwpa")){
    f = SPIFFS.open("/apwpa", "r");
    if(f){
      apwpa = f.readString();
      f.close();
    }
  }
    if(SPIFFS.exists("/clientssid")){
    f = SPIFFS.open("/clientssid", "r");
    if(f){
      cssid = f.readString();
      f.close();
    }
  }
    if(SPIFFS.exists("/clientwpa")){
    f = SPIFFS.open("/clientwpa", "r");
    if(f){
      cwpa = f.readString();
      f.close();
    }
  }

  DateTime now = rtc.now();
  char buffd[] = "YYYY-MM-DD";
  String valuedate = now.toString(buffd);
  char bufft[] = "hh:mm";
  String valuetime = now.toString(bufft);
    
  String contentpart = settings0;

  if (SoundON) {
    contentpart += " checked";
    }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", contentpart); 
  contentpart = " ";
  server.sendContent(settings1);  
  //f = SPIFFS.open("/di", "w");
  //f.write("<br><br>its still ooook3<br><br>");
  //f.close();
     
  if (LEDON){
    contentpart += " checked";
  }
  server.sendContent(contentpart);
  contentpart = " ";
  server.sendContent(settings2);
  contentpart = valuedate;
  server.sendContent(contentpart);

  server.sendContent(settings3);
  contentpart = valuetime;
  server.sendContent(contentpart);
  server.sendContent(settings4);
  contentpart = " ";
  if (cryptON){
    contentpart += " checked";
  }
  server.sendContent(contentpart);
  contentpart = " ";
    f = SPIFFS.open("/di", "w");
  f.write("<br><br>its still ok4<br><br>");
  f.close();
  server.sendContent(settings5);
  contentpart = key[10];
  server.sendContent(contentpart);
  server.sendContent(settings6);
  contentpart = " ";
  byte keynumber = 0;
  while (keynumber < 5){
    contentpart += "<div><input type=\"text\" id=\"key"+String(keynumber)+"\" name=\"key"+String(keynumber)+"\" size=\"34\" value=\""+key[keynumber]+"\"><label for=\"key"+String(keynumber)+"\">Key "+String(keynumber)+"</label></div>";
    keynumber += 1;
  }
  server.sendContent(contentpart);  
  contentpart = " ";
  while (keynumber < 10){
    contentpart += "<div><input type=\"text\" id=\"key"+String(keynumber)+"\" name=\"key"+String(keynumber)+"\" size=\"34\" value=\""+key[keynumber]+"\"><label for=\"key"+String(keynumber)+"\">Key "+String(keynumber)+"</label></div>";
    keynumber += 1;
  }
  server.sendContent(contentpart); 
  contentpart = "<div><input type=\"number\" id=\"tshift\" name=\"tshift\" min=\"-1000000000\" max=\"1000000000\" value=\""+String(shiftkey);
  server.sendContent(contentpart);
  contentpart = " ";
  server.sendContent(settings7);
  if (!isAP){
    contentpart = " checked";
  }
  server.sendContent(contentpart);
  server.sendContent(settings8);
  if (isAP){
    contentpart = " checked";
  }  
  server.sendContent(contentpart);
  contentpart = " ";
  server.sendContent(settings88);
  server.sendContent(cssid);
  server.sendContent(settings9);
  server.sendContent(cwpa);
  server.sendContent(settings10);
  server.sendContent(apssid);
  server.sendContent(settings11);
  server.sendContent(apwpa);
  server.sendContent(settings12);

  contentpart = String(txpower);
  server.sendContent(contentpart);
  server.sendContent(settings13);
  contentpart = String(spreadingfactor);
  server.sendContent(contentpart);
  server.sendContent(settings14);
  contentpart = String(signalbandwith);
  server.sendContent(contentpart);
  server.sendContent(settings15);
  contentpart = String(codingratedenominator);
  server.sendContent(contentpart);
  server.sendContent(settings16);
  contentpart = String(syncword);
  server.sendContent(contentpart);
  server.sendContent(settings17);
  contentpart = " ";
  if (crc){
    contentpart = " checked";
  }
  server.sendContent(contentpart);
  server.sendContent(settings18);
}

void ntp(){
timeClient.begin();
timeClient.update();
unsigned long epochtime = timeClient.getEpochTime() - utcOffsetInSeconds; //fixme
int HH = timeClient.getHours();
int MM = timeClient.getMinutes();
int SS = timeClient.getSeconds();
rtc.adjust(DateTime(2022, 11, 21, HH, MM, SS));

}

void factoryreset(){
    tone(D8,200,1000);
    SPIFFS.format();
    delay(100);
    f = SPIFFS.open("/init.bin", "w"); 
    f.write(bool2byte(false)); //isAP
    f.write(bool2byte(SoundON)); //SoundON
    f.write(bool2byte(LEDON)); //LEDOn
    f.write(0); //lastmessage
    f.write(bool2byte(false));  //fullmessagearray
    f.write(txpower); //txPower - TX power in dB, defaults to 17 (17,20)
    f.write(spreadingfactor); //spreadingFactor - spreading factor, defaults to 7 (6..12)
    f.write(signalbandwith); //signalBandwidth - signal bandwidth in Hz, defaults to 125E3 (7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3)
    f.write(codingratedenominator); //codingRateDenominator - denominator of the coding rate, defaults to 5 (5..8)
    f.write(preamblelength); //preambleLength - preamble length in symbols, defaults to 8 (6..65535)
    f.write(syncword); //syncWord - byte value to use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.write(bool2byte(cryptON)); //Enable or disable cryptography
    f.write(shiftkey);
    f.close(); // закрыли файл
    
    f = SPIFFS.open("/apssid", "w");         
    f.write("radio.chat.a1");
    f.close();
    
    f = SPIFFS.open("/apwpa", "w"); 
    f.write("radio.chat.a1");
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

    byte keynumber = 0;
    while (keynumber < 11){
    f = SPIFFS.open("/key"+String(keynumber), "w"); 
    f.print(key[keynumber]);    
    f.close();
    keynumber += 1;
    }
    
  ESP.restart();
}

void setup() {
  pinMode(D8, OUTPUT);  //uncomment 4 tx rx
  pinMode(TX, FUNCTION_3); //sda
  pinMode(RX, FUNCTION_3); //scl
  Wire.begin(TX,RX); 
    //WiFi.mode(WIFI_STA);
    //WiFi.begin("SSID", "WPA123456"); //factory wi-fi hardcode
    //delay(10000);
    tone(D8,3000,500);
    SPIFFS.begin();  
        isfailedstart = true;
        if(SPIFFS.exists("/failedstart")){
        f = SPIFFS.open("/failedstart", "w");
        if(f){ 
        f.write(bool2byte(isfailedstart));
        failedstartcount = failedstartcount + 1;
        f.write(failedstartcount);
        f.close();
        if(failedstartcount >= 3){
            factoryreset();
        }
        } else {
          factoryreset();
        }
        } else {
          factoryreset();
        } 

        if(SPIFFS.exists("/di")) {// проверка есть ли файл
        f = SPIFFS.open("/di", "r");
        if(f){
          debinfo += f.readString();
          f.close();
        }
        }
        
    if(SPIFFS.exists("/init.bin")) // проверка есть ли файл
       {         
         f = SPIFFS.open("/init.bin", "r");
         if(f) // проверка открылся ли файл
         {           
           if(f.size()==20) // если файл не пустой и не битый
           {     
             //читаем всё          
                isAP = byte2bool(f.read());
                SoundON = byte2bool(f.read());
                LEDON = byte2bool(f.read());
                lastmessage = f.read();                
                fullmessagearray = byte2bool(f.read());
                txpower = f.read();
                spreadingfactor = f.read();
                signalbandwith = f.read(); //?
                codingratedenominator = f.read();
                preamblelength = f.read();
                syncword = f.read();
                crc = byte2bool(f.read());
                cryptON = byte2bool(f.read());
                shiftkey = f.read();
                f.close();                                 
                screenlastmessage = lastmessage-15;
           }
           else {
           //factoryreset(); //wrong size
           debinfo += " init file size is: " + String(f.size());
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
       WiFi.disconnect();
       if (WiFi.status() != WL_CONNECTED) {       
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
       
       

        
        

    tone(D8,500,100);


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
  server.on("/pullup",pullup);
  server.on("/ntp", ntp); 
  server.on("/settings", handleSettings);  
  server.on("/debug", debug);
  server.on("/battery", battery);
  server.on("/getap", getap);
  server.on("/pageupdate", pageupdate);  
  server.on("/rm",factoryreset);  
  server.begin();
  httpUpdater.setup(&server, OTAPATH, OTAUSER, OTAPASSWORD);  
  LoRa.setPins(ss, rst, dio0); //uncomment 4 lora
  if (!LoRa.begin(433E6)) {
    delay(4000);
    tone(D8,200,1000);
  }
  tone(D8,5000,50);

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
    if (looptime - battlooptime > 10000) {
      battlevel = analogRead(A0);
      float battlevelpercentfloat = (battlevel - 650) * 0.54;
      if(battlevelpercentfloat > 100){
        battlevelpercentfloat = 100;
      }
      if(battlevelpercentfloat < 1){
        battlevelpercentfloat = 0;
      }
      battlevelpercent = battlevelpercentfloat;
      battlooptime = looptime;
    }
    if (looptime-beeplooptime > 250 and SoundON and looptime > 15000){
      beeploop += 1;
      if (battlevelpercent == 0){
        tone(D8,220*(4 - beeploop % 4),200);
      }
      if (battlevelpercent == 20 and beeploop % 8 == 0){
        tone(D8,220,500);
      }      
      if (beeploop > 31) {
        beeploop = 0;
      }
      beeplooptime = looptime;
    }
  server.handleClient();
  recMsg(LoRa.parsePacket());
}
