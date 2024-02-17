/* Ближайшие задачи:
 * +Добавить схоронение конфигов:  поиск: handleSubmit()
 * +разбить количество сообщений/текущее Сообщение и общий конфиг на разные файлы
 * +починить отображение сообщений после перезапуска
 * !!!!!! можно и забить хер, например
 * продумать подключение к фабричной сети (если есть хардкод - к нему, если нет, к тому, что на флешке если прописано что не точка доступа, в паршивке прописывать точку доступа)
 * не стесняться ребутать себя при выборе режима вайвай, т.к. откючаться и переподключаться контролер умеет плохо
 * типа ищем хардкод, если его нет прописываем одноразовый файл -- грузить не хардкод, ребутаемся, стираем одноразку и там уже дальше что в конфигах
 * при этом можно попикивать
 * !!!!!!
 * Добавить шифрованню, можно на компе в обычных сях испробовать или на тинкеркаде
 * контроль версий добавить в паршивку
 * 0.025 мгц шаг канала -- 434.775 максимум
 * Описание того, что требуется хранить на флешке: 
 * 1. Тип подключения к вайвай (точка доступа или к роутеру)
 * SSID/пароли точки доступа и роутера
 * Число неудачных включений (неудачное, если не было подключения к вайвай)
 * Мастер-ключ для шифрования (может быть дополнительно набор ключей для выбора алгоритма по времени)
 * Имя устройства для передачи
 * +Параметры LoRA (ширина канала, длина префикса, частота)
 * +Двухмерый массив последних 100 сообщений + датавремя, дополнительно число заюзаных слотов, номер текущего (чтобы в правильном порядке показывать)
 * мелодии, хз
 * +первым делом пишем интерфейс, для этого комментим всю лору и все часики, сначала всё читаем и вырпезаем лишнее/непонятное
 * есть мысль подгружать футер и хедер по отдельности с другими гетами, чтобы тело грузить с помощью жаваскрипта
починить iframe/обновления куска+ 
звук, 
настройки
ntp
+добавить шифрование
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

#define devicename "radio.chat.a0"
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
<a href="/rm">Заводская конфигурация (сброс всех настроек)</a><br>
<a href="/fw-upgrade">Обновить прошивку</a> | <a href="/debug">Отладочная информация</a> | <a href="/ntp">Обновить время по NTP (дату руками))</a><br>
<a href="/reboot">Перезагрузить</a><br>
<form action="/" method="post">
<fieldset>
    <legend>Общие параметры:</legend>
    <div>
    <input type="hidden" value="24" name="gen" id="gen">
      <input type="checkbox" id="ledon" name="ledon")=====";

const char settings1[] PROGMEM = R"=====(>
      <label for="ledon">Светодиод (надо придумать зачем)</label>
    </div>
    <div>
      <input type="checkbox" id="speaker" name="speaker")=====";

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
      <input type="checkbox" id="crypt" name="crypt")=====";

const char settings5[] PROGMEM = R"=====(>
      <label for="crypt">Включить шифрование (нет бэкенда)</label>
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
      <input type="radio" id="client" value="client" name="mode")=====";

const char settings8[] PROGMEM = R"=====(>
      <label for="client">Клиент</label>
    </div>
    <div>
      <input type="radio" id="server" value="server" name="mode")=====";
      
const char settings88[] PROGMEM = R"=====(>
      <label for="server">Точка доступа</label>
    </div>
    <input type="submit" value="Сохранить" />
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
  <input type="number" id="txpower" name="txpower" step="3"
         min="2" max="20" value=")=====";

const char settings13[] PROGMEM = R"=====(">
  <label for="txpower">Мощность передатчика (17 или 20)</label>
</div>
<div>
  <input type="number" id="spreadingfactor" name="spreadingfactor"
         min="6" max="12" value=")=====";

const char settings14[] PROGMEM = R"=====(">
  <label for="spreadingfactor">Коэффициент распространения Spreading Factor</label>
</div>
<div>
  <input type="number" id="frequency" name="frequency"
         min="410000000" max="525000000" step="25000" value=")=====";

const char settings15[] PROGMEM = R"=====(">
  <label for="frequency">Частота сигнала (Гц) Frequency шаг 25000 начиная с 433 мгц</label>
</div>
<div>
  <input type="number" id="bandwith" name="bandwith"
         min="7800" max="500000" value=")=====";
         
const char settings155[] PROGMEM = R"=====(">
  <label for="bandwith">Ширина канала (Гц) Bandwith (умеет 7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000)</label>
</div>
<div>
  <input type="number" id="denominator" name="denominator"
         min="5" max="8" value=")=====";         

const char settings16[] PROGMEM = R"=====(">
  <label for="denominator">Coding rate denominator</label>
</div>
<div>
  <input type="number" id="preamblelength" name="preamblelength"
         min="6" max="65535" value=")=====";

const char settings165[] PROGMEM = R"=====(">
  <label for="preamblelength">Длина преамбулы (6..65535)</label>
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
unsigned long signalfrequency = 433E6; //
unsigned long signalbandwith = 125E3;
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
String cssid = "SSID";
String cWPA = "WPA123456";
String assid = devicename;
String aWPA = devicename;
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
    int freqtone = 0;
    if(SoundON){
      tone(D8,100+freqtone*30);      
    }
      msg_text += (char)LoRa.read();      
      //fixme добавить расшифровку      
    }

    if(SoundON){
    noTone(D8);
    }
    msg_rssi = String(LoRa.packetRssi());
   DateTime now = rtc.now();
   char buff[] = "DD.MM.YYYY hh:mm:ss";
   String msg_time = now.toString(buff);
   if(SoundON){
   tone(D8,2000,200);
   }
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
    f = SPIFFS.open("/msgs", "w");    
    f.write(lastmessage); 
    f.write(bool2byte(fullmessagearray));  //fullmessagearray
    f.close();
/*    f = SPIFFS.open("/init.bin", "w"); 
    f.write(bool2byte(isAP)); //isAP
    f.write(bool2byte(SoundON)); //SoundON
    f.write(bool2byte(LEDON)); //LEDOn

    f.write(txpower); //txPower - TX power in dB, defaults to 17 (17,20)
    f.write(spreadingfactor); //spreadingFactor - spreading factor, defaults to 7 (6..12)
    f.write(signalfrequency); //signalBandwidth - signal bandwidth in Hz, defaults to 125E3 (7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3)
    f.write(codingratedenominator); //codingRateDenominator - denominator of the coding rate, defaults to 5 (5..8)
    f.write(preamblelength); //preambleLength - preamble length in symbols, defaults to 8 (6..65535)
    f.write(syncword); //syncWord - byte value fto use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.write(bool2byte(cryptON)); //Enable or disable cryptography
    f.write(shiftkey);
    f.close(); // закрыли файл
    */
    newmessage += "<span class=\"u2 chat\">"+msg_text+"</span><span class=\"recinfo\">"+msg_attr+"</span>";
    //ыщо туд можно попикать и поморгать, хз
    if(SoundON){
    tone(D8,4000,50);
    }
        //fixme управление по лора
    if(msg_text == "/ip"){
      IPAddress myIP;
       if(isAP){
         myIP =  WiFi.softAPIP();
       } else {
         myIP = WiFi.localIP();
       }
       String ipaddr = String(myIP[0])+"."+String(myIP[1])+"."+String(myIP[2])+"."+String(myIP[3]);
       sendMsg(ipaddr);
    } else {
      if(msg_text == "/getap"){
        getap();
      }
    }
  }
        
}

void soundIP(){ //переписать delay на millis fixme
         
       IPAddress myIP;
       if(isAP){
         myIP =  WiFi.softAPIP();
       } else {
         myIP = WiFi.localIP();
       }
       String ipaddr = String(myIP[0])+"."+String(myIP[1])+"."+String(myIP[2])+"."+String(myIP[3]);
       debinfo += "debug ip from sounding: " + ipaddr;
       byte l = ipaddr.length();
       debinfo += " length:"+String(l);
        
        for(byte j=0;j<l;j++){
          char x = ipaddr[j];
          if(x == '.'){
            delay(2000); 
          } else {
          byte y = x - '0';
          if (y == 0) {
            tone(D8,10000);
            delay(500);      
            noTone(D8);
          } else {
            for(byte k = 1; k==x; k++){
            tone(D8,y*100);
            delay(300);
            noTone(D8);
            }
            delay(1000);
          }
          delay(2000);
          }
        }
       
       
}


void sendMsg(String msg_text){
  if (msg_text != prevmessage and msg_text[1] != '/'){ //костыль, т.к. веб-форма шлёт дважды...
    if(SoundON){
    tone(D8,500);
    }
    LoRa.beginPacket(); //fixme добавить шифрование
    LoRa.print("=");
    LoRa.print(msg_text);    
    LoRa.endPacket();
    if(SoundON){
    noTone(D8);
    }
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
    f = SPIFFS.open("/msgs", "w");
    f.write(lastmessage); 
    f.write(bool2byte(fullmessagearray));  //fullmessagearray
    f.close();
    
    prevmessage = msg_text;
  } 
  
}

void handleSubmit() { //проверка ответов? .toInt  //надо всё прожамкать, посмотреть, перевести в типы и схоронять в флешку
  bool ConfigChanges = false;
 bool  rebootneeded = false;
  
  if (server.hasArg("msgtextforsend")) { /////////NAME of form is hasArg    
    String msg_text = server.arg("msgtextforsend"); //fixme добавить шифрование
          sendMsg(msg_text);               
    }
  if (server.hasArg("gen")) {                   
          LEDON = false;
          SoundON = false;
          if (server.hasArg("speaker")) {     
          SoundON = true;          
          }
          if (server.hasArg("ledon")) {     
          LEDON = true;          
          }
          ConfigChanges = true;
    }
  
  if (server.hasArg("cdate") and server.hasArg("ctime")) {     
    String newdate = server.arg("cdate"); 
    String newtime = server.arg("newtime"); //subst с нуля и до того, что не входит 2023-04-05 12:10 rtc.adjust(DateTime(2022, 10, 20, 3, 55, 10));
    int YYYY = newdate.substring(0,4).toInt();
    int MM = newdate.substring(5,7).toInt();
    int DD = newdate.substring(8).toInt();
    int HH = newtime.substring(0,2).toInt();
    int mm = newtime.substring(0,2).toInt();    
          debinfo += "<br> cdate: " + newdate;               
          debinfo += "<br> cdate: " + newtime;               
    rtc.adjust(DateTime(YYYY, MM, DD, HH, mm, 00));            
    }
  if (server.hasArg("masterkey")) {
    key[0] = server.arg("key0");
    key[1] = server.arg("key1");
    key[2] = server.arg("key2");
    key[3] = server.arg("key3");
    key[4] = server.arg("key4");
    key[5] = server.arg("key5");
    key[6] = server.arg("key6");
    key[7] = server.arg("key7");
    key[8] = server.arg("key8");
    key[9] = server.arg("key9");
    key[10] = server.arg("masterkey");
    shiftkey = server.arg("tshift").toInt();
          debinfo += "<br> masterkey: " + key[10]; 
          cryptON = false;
    ConfigChanges = true;
    }
  if (server.hasArg("crypt")) {     
    String msg_text = server.arg("crypt");     
    cryptON = true;
    ConfigChanges = true;
    }
  
  
  if (server.hasArg("mode")) {     
    String inmode = server.arg("mode");
    isAP = false;
    if (inmode == "server"){
      isAP=true;
    }
          ConfigChanges = true;
    }  
  if (server.hasArg("clientssid")) {     
    cssid = server.arg("clientssid"); 
    cWPA = server.arg("clientwpa"); 
    assid = server.arg("apssid"); 
    aWPA = server.arg("apwpa");
          ConfigChanges = true;
    }  
  if (server.hasArg("txpower")) {     
    txpower = server.arg("txpower").toInt();
    spreadingfactor = server.arg("spreadingfactor").toInt();
    signalfrequency = server.arg("frequency").toInt();
    signalbandwith = server.arg("bandwith").toInt();
    codingratedenominator = server.arg("denominator").toInt();
    syncword = server.arg("syncword").toInt();
    crc = false;    
          ConfigChanges = true;
          rebootneeded = true;
    }
  if (server.hasArg("crc")) {     
    crc = true;
          ConfigChanges = true;
    }
    if (ConfigChanges){
        String reloadafter = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><meta http-equiv=\"refresh\" content=\"3; url='/'\"></head><body><p>Один момент! (3 секунды)</p></body></html>"; 
        server.send(200, "text/html", reloadafter);
        factoryreset(true); 
    }
    //if (rebootneeded){
    //  ESP.restart();
    //}
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
  WiFi.softAP(assid, aWPA);
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
  debuginfo += debinfo + "<br>";
  debuginfo += "<br>";
  debuginfo += "125E3 is:" + String(125E3) + "<br>";
  int x = 433E6;
  debuginfo += "433E6 is:" + String(433E6) + "<br>";
  debuginfo += "433E6 through int is:" + String(x) + "<br>";
  f = SPIFFS.open("/init.bin", "r");
  debuginfo += " init file size is: " + String(f.size());
  f.close();
  debuginfo += "<br>";
  debuginfo += "screenlastmessage:" + String(screenlastmessage);
  debuginfo += "<br>";
  debuginfo += "message_id:" + String(lastmessage-1) + ":  exist:" + String(SPIFFS.exists("/msg_id_"+String(lastmessage-1)));
  if(SPIFFS.exists("/msg_id_"+String(lastmessage-1))) // проверка есть ли файл
       {         
         f = SPIFFS.open("/msg_id_"+String(lastmessage-1), "r");
         if(f) // проверка открылся ли файл
         {                            
             debuginfo += f.readString();                
             f.close();    
         }
       }   
  IPAddress myIP1 = WiFi.softAPIP();
  IPAddress myIP2 = WiFi.localIP();
                         
                    
  debuginfo += "soft ip: " + myIP1.toString(); + "<br>";
  debuginfo += "<br>local ip: " + myIP2.toString(); + "<br>";
  debuginfo += "signalfrequency: " + String(signalfrequency) + "<br>";
  debuginfo += "bandwith: " + String(signalbandwith) + "<br>";
  debuginfo += ": " + String(isAP) + "<br>";
  debuginfo += ": " + String(isAP) + "</body></html>";  
  dir = SPIFFS.openDir("/");
         while(dir.next()) 
       {
         debuginfo += "<br>"+dir.fileName();
         f = dir.openFile("r"); 
         debuginfo += " Size file=" + String(f.size());
         
       }


  server.send(200, "text/html", debuginfo);
  soundIP();
}

void battery() {  
  String debuginfo = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"></head><body>";
  
   DateTime now = rtc.now();
   char buff[] = "DD.MM.YYYY hh:mm:ss";
   String debugtime = now.toString(buff);
  debuginfo += debugtime + " " + String(battlevel) + "</body></html>";
  server.send(200, "text/html", debuginfo);
}

void resetdefault() {  
  String reloadafter = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><meta http-equiv=\"refresh\" content=\"30; url='/'\"></head><body><p>Один момент! (30 секунд)</p></body></html>"; 
  server.send(200, "text/html", reloadafter);
  factoryreset(false);  
}

void handleSettings() {  
  if(SPIFFS.exists("/apssid")){
    f = SPIFFS.open("/apssid", "r");
    if(f){
      assid = f.readString();
      f.close();
    }
  }
    if(SPIFFS.exists("/apwpa")){
    f = SPIFFS.open("/apwpa", "r");
    if(f){
      aWPA = f.readString();
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
      cWPA = f.readString();
      f.close();
    }
  }

  DateTime now = rtc.now();
  char buffd[] = "YYYY-MM-DD";
  String valuedate = now.toString(buffd);
  char bufft[] = "hh:mm";
  String valuetime = now.toString(bufft);
    
  String contentpart = settings0;

  if (LEDON) {
    contentpart += " checked";
    }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", contentpart); 
  contentpart = " ";
  server.sendContent(settings1);  
  //f = SPIFFS.open("/di", "w");
  //f.write("<br><br>its still ooook3<br><br>");
  //f.close();
     
  if (SoundON){
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
    contentpart += " checked";
  }   
  server.sendContent(contentpart);
  contentpart = " ";
  server.sendContent(settings8);
  if (isAP){
    contentpart += " checked";
  } 
  server.sendContent(contentpart);
  contentpart = " ";
  server.sendContent(settings88);
  server.sendContent(cssid);
  server.sendContent(settings9);
  server.sendContent(cWPA);
  server.sendContent(settings10);
  server.sendContent(assid);
  server.sendContent(settings11);
  server.sendContent(aWPA);
  server.sendContent(settings12);

  contentpart = String(txpower);
  server.sendContent(contentpart);
  server.sendContent(settings13);
  contentpart = String(spreadingfactor);
  server.sendContent(contentpart);
  server.sendContent(settings14);
  contentpart = String(signalfrequency);
  server.sendContent(contentpart);
  server.sendContent(settings15);
  contentpart = String(signalbandwith);
  server.sendContent(contentpart);
  server.sendContent(settings155);
  contentpart = String(codingratedenominator);
  server.sendContent(contentpart);
  server.sendContent(settings16);
  contentpart = String(preamblelength);
  server.sendContent(contentpart);
  server.sendContent(settings165);
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

void ntp(){ //fixme добавить норм обработку года и даты (100% на свежую голову надо делать)
String reloadafter = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><meta http-equiv=\"refresh\" content=\"5; url='/settings'\"></head><body><p>Один момент! (5 секунд)</p></body></html>"; 
        server.send(200, "text/html", reloadafter);  
timeClient.begin();
timeClient.update();

unsigned long epochtime = timeClient.getEpochTime() - utcOffsetInSeconds; //fixme
int HH = timeClient.getHours();
int MM = timeClient.getMinutes();
int SS = timeClient.getSeconds();
// мудрим циклом хотя это тупо
//unsigned long bigdigit = epochtime;
/*timeStamp formattedDate = timeClient.getFormattedDate();
    int YYYY = formattedDate.substring(0,4).toInt();
    int Mnth = formattedDate.substring(5,7).toInt();
    int DD = formattedDate.substring(8).toInt();
/////))))
*/
rtc.adjust(DateTime(2023, 04, 29, HH, MM, SS));


}

void reboot(){
          String reloadafter = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><meta http-equiv=\"refresh\" content=\"10; url='/'\"></head><body><p>Один момент! (10 секунд)</p></body></html>"; 
        server.send(200, "text/html", reloadafter);
        delay(1000);
  ESP.restart();
}

void factoryreset(bool rewrite){ 
  if(!rewrite){
    isAP = true; 
    SoundON = true;
    LEDON = true;
    lastmessage = 0;
    fullmessagearray = false; //если больше 255 сообщений напосылалось
    txpower = 20;
    spreadingfactor = 7;
    signalfrequency = 433E6; //
    signalbandwith = 125E3;
    codingratedenominator = 5;
    preamblelength = 8;
    syncword = 0x34;
    crc = true;
    byte truebyte = 255;
    byte falsebyte = 0;
    battlevel = 0;
    battlevelpercent = 0;
    beeploop = 0;
    
    key[0] = "SkekNdNue9JWTs7vO2MgJtxa";
    key[1] = "j2lpmj3AlJmknfnAgYzoJFAE";
    key[2] = "6GYD5dj2xLk28qpPsZHRqPg0";
    key[3] = "ot926pyRqYj2mkHQMGkjuRk5";
    key[4] = "P8ToIPBVDQFtVGYqA2SzNjjK";
    key[5] = "0ERSyYtblmnZFPkDEq6fdmAJ";
    key[6] = "Mkx5LJRZeEoR03reTB8fLz0f";
    key[7] = "ZDusvCwNgUeMPRoBcfADpJi6";
    key[8] = "5P9gTSOiGAuW8yaAaq9xKZYl";
    key[9] = "ZXAb5qBMO1dnW7QCPfg966wE";
    key[10] = "iQaHg8Fb36vpEjdSrrluZc3U";
    
    shiftkey = 93156784; 
    cryptON = true;
    
    failedstartcount = 0;
    isfailedstart = true;
    assid = devicename;
    aWPA = devicename;
    cssid = "SSID";
    cWPA = "WPA123456";
    looptime = 0;
    battlooptime = 0;
    beeplooptime = 0;
    somethingsend = false;
    
    newmessage = "";
    prevmessage = ""; //костыль
    screenlastmessage = 0;
    debinfo = "";
    SPIFFS.format();
  }
    if(SoundON){
    tone(D8,200,1000);
    }
    
    
    delay(100);

    f = SPIFFS.open("/init.bin", "w"); 
    f.write(bool2byte(isAP)); //isAP
    f.write(bool2byte(SoundON)); //SoundON
    f.write(bool2byte(LEDON)); //LEDOn
    f.write(txpower); //txPower - TX power in dB, defaults to 17 (17,20)
    f.write(spreadingfactor); //spreadingFactor - spreading factor, defaults to 7 (6..12)
    //f.write(signalfrequency); //частота
    //f.write(signalbandwith); //signalBandwidth - signal bandwidth in Hz, defaults to 125E3 (7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3)
    f.write(codingratedenominator); //codingRateDenominator - denominator of the coding rate, defaults to 5 (5..8)
    f.write(preamblelength); //preambleLength - preamble length in symbols, defaults to 8 (6..65535)
    f.write(syncword); //syncWord - byte value to use as the sync word, defaults to 0x34 (0x12)
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();
    f.write(bool2byte(cryptON)); //Enable or disable cryptography
    f.write(shiftkey);
    f.close(); // закрыли файл
    
    f = SPIFFS.open("/msgs", "w");
    f.write(lastmessage); //lastmessage
    f.write(bool2byte(fullmessagearray));  //fullmessagearray
    f.close();
    
    f = SPIFFS.open("/bandwith", "w");
    f.print(String(signalbandwith));    
    f.close();
    
    f = SPIFFS.open("/frequency", "w");
    f.print(String(signalfrequency));    
    f.close();
    
    f = SPIFFS.open("/apssid", "w");         
    f.print(assid);
    f.close();
    
    f = SPIFFS.open("/apwpa", "w"); 
    f.print(aWPA);
    f.close();
    
    f = SPIFFS.open("/clientssid", "w"); 
    f.print(cssid);
    f.close();
    
    f = SPIFFS.open("/clientwpa", "w"); 
    f.print(cWPA);
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
  if(!rewrite)  {
  ESP.restart();
  }
}

void setup() {
  pinMode(D8, OUTPUT);  //uncomment 4 tx rx
  pinMode(TX, FUNCTION_3); //sda
  pinMode(RX, FUNCTION_3); //scl
  pinMode(D4, OUTPUT); //led
  digitalWrite(D4, HIGH);
  
  Wire.begin(TX,RX); 
    //WiFi.mode(WIFI_STA);
    //WiFi.begin("SSID", "WPA123456"); //factory wi-fi hardcode
    //delay(10000);
    if(SoundON){
    tone(D8,3000,500);
    }
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
            factoryreset(false);
        }
        } else {
          factoryreset(false);
        }
        } else {
          factoryreset(false);
        } 

        if(SPIFFS.exists("/di")) {// проверка есть ли файл
        f = SPIFFS.open("/di", "r");
        if(f){
          debinfo += f.readString();
          f.close();
        }
        }
    if(SPIFFS.exists("/bandwith")) // проверка есть ли файл
       {         
         f = SPIFFS.open("/bandwith", "r");
         if(f) // проверка открылся ли файл
         {       
           signalbandwith = f.readString().toInt(); //да, костыль но SPIFFS не умеет большие числа
         }
         else {
               factoryreset(false);  //файл не может открыться
        }
       } else factoryreset(false); //файла нет
    if(SPIFFS.exists("/frequency")) // проверка есть ли файл
       {         
         f = SPIFFS.open("/frequency", "r");
         if(f) // проверка открылся ли файл
         {       
           signalfrequency = f.readString().toInt();
         }
         else {
               factoryreset(false);  //файл не может открыться
        }
       } else factoryreset(false); //файла нет
       
    if(SPIFFS.exists("/init.bin")) // проверка есть ли файл
       {         
         f = SPIFFS.open("/init.bin", "r");
         if(f) // проверка открылся ли файл
         {           
           if(f.size()==11) // если файл не пустой и не битый
           {     
             //читаем всё          
                isAP = byte2bool(f.read());
                SoundON = byte2bool(f.read());
                LEDON = byte2bool(f.read());

                txpower = f.read();
                spreadingfactor = f.read();
                //signalfrequency = f.read(); //?
                //signalbandwith = f.read();                
                codingratedenominator = f.read();
                preamblelength = f.read();
                syncword = f.read();
                crc = byte2bool(f.read());
                cryptON = byte2bool(f.read());
                shiftkey = f.read();
                f.close();                                 
                    if(SPIFFS.exists("/msgs"))    {
      f = SPIFFS.open("/msgs", "r");
                lastmessage = f.read();                
                fullmessagearray = byte2bool(f.read());
      f.close();
    }
                screenlastmessage = lastmessage-15;
           }
           else {
           //factoryreset(); //wrong size
           debinfo += " init file size is: " + String(f.size());
           }
         }
         else {
               factoryreset(false);  //файл не может открыться
        }
       } else factoryreset(false); //файла нет

              if(SPIFFS.exists("/failedstart")){  //fixme нелогично, сначала надо читать, а потом писать
            f = SPIFFS.open("/failedstart", "r");
            if(f){
                isfailedstart = byte2bool(f.read());
                failedstartcount = f.read();
                f.close();
                } else factoryreset(false);
            }
       else factoryreset(false);
       delay(1000);       
       //if (WiFi.status() != WL_CONNECTED) {       
        WiFi.disconnect();
        if(SPIFFS.exists("/clientssid")){
                    f = SPIFFS.open("/clientssid", "r");
                      if(f){
                        cssid = f.readString();
                        f.close();
                      } else factoryreset(false);
                    } else factoryreset(false);
        if(SPIFFS.exists("/clientwpa")){
                    f = SPIFFS.open("/clientwpa", "r");
                    if(f){
                        cWPA = f.readString();
                        f.close();
                    } else factoryreset(false);                    
                    } else factoryreset(false);
      if(SPIFFS.exists("/apssid")){
                    f = SPIFFS.open("/apssid", "r");
                      if(f){
                        assid = f.readString();
                        f.close();
                      } else factoryreset(false);
                    } else factoryreset(false);
      if(SPIFFS.exists("/apwpa")){
                    f = SPIFFS.open("/apwpa", "r");
                    if(f){
                        aWPA = f.readString();
                        f.close();
                    } else factoryreset(false);
                    
                    } else factoryreset(false);
       
       if (isAP){
                    WiFi.softAP(assid, aWPA); //точка доступа

       } else {
                    WiFi.mode(WIFI_STA); //клиент
                    WiFi.begin(cssid, cWPA);                                        
                }
       
       //чтение ключей сюда
       byte keynumber = 0;
        while (keynumber < 11){
        f = SPIFFS.open("/key"+String(keynumber), "r"); 
        key[keynumber] = f.readString();    
        f.close();
        keynumber += 1;
        }
       
       

       


        
        
        
if(SoundON){
    tone(D8,500,100);
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
  server.on("/pullup",pullup);
  server.on("/ntp", ntp); 
  server.on("/settings", handleSettings);  
  server.on("/debug", debug);
  server.on("/battery", battery);
  server.on("/getap", getap);
  server.on("/pageupdate", pageupdate);  
  server.on("/rm",resetdefault);  
  server.on("/reboot",reboot);
  server.begin();
  httpUpdater.setup(&server, OTAPATH, OTAUSER, OTAPASSWORD);  
  LoRa.setPins(ss, rst, dio0); //uncomment 4 lora
  if (!LoRa.begin(signalfrequency)) {
    delay(4000);
    if(SoundON){
    tone(D8,200,1000);
    }
  }
  LoRa.setSyncWord(syncword);
  LoRa.setSpreadingFactor(spreadingfactor);
  LoRa.setTxPower(txpower);
  LoRa.setSignalBandwidth(signalbandwith);
  LoRa.setPreambleLength(preamblelength);
  if(crc){
    LoRa.enableCrc();
    } else {
    LoRa.disableCrc();
      }
    f.write(bool2byte(crc)); //Enable or disable CRC usage, by default a CRC is not used. //LoRa.enableCrc(); LoRa.disableCrc();

           //IPAddress myIP = WiFi.softAPIP();

  /*
   * 
   * txpower = f.read();
                spreadingfactor = f.read();
                signalfrequency = f.read(); //?
                codingratedenominator = f.read();
                preamblelength = f.read();
                syncword = f.read();
                crc = byte2bool(f.read());
   */
  if(SoundON){
  tone(D8,5000,50);
  }
battlevel = analogRead(A0);
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
    if(LEDON){
      if (isAP){
      if (millis() % 1000 <= 500) {
      digitalWrite(D4, HIGH);
      }    else digitalWrite(D4, LOW);
      } else {
        if (millis() % 3000 <= 1500) {
        digitalWrite(D4, HIGH);
      }    else digitalWrite(D4, LOW);
      }
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
