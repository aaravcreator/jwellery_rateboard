#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
 



// includes for DMD DISPLAY
#include <DMDESP.h>
#include <fonts/ElektronMart6x8.h>
#include <fonts/Mono5x7.h>


//SETUP DMD
#define DISPLAYS_WIDE 2 // Number of Panel
#define DISPLAYS_HIGH 1 // Height of Panel
DMDESP merodisplay(DISPLAYS_WIDE, DISPLAYS_HIGH);  // Init DMD
// includes and setup for DMD ENDS

#define WIFI_CONN_TIMEOUT 15 // Define Timeout in seconds eg. 15 secs

#ifndef APSSID 
#define APSSID "RATE DISP - AT"
#define APPSK  "aarav256"
#endif

/* Set these to your desired credentials. */
const char* apssid = APSSID;
const char* appassword = APPSK;
const char* wifiSSID = "xx";
const char* wifiPASS = "x"; 

ESP8266WebServer meroserver(80);

unsigned long connectTimeout;
uint8_t wifiStatus;

////WiFi variables ///

bool blank_device = false;

/// END WiFi variables ///

///Display variables//
//int brightness=20;

///END Display variables///

int ip_button = 5;

///Rate variables ///
String date;
String date_updated;
String gold_buy;
String gold_sell;
String silver_buy;
String silver_sell;

bool updated = false;

/// END Rate variables///




File fsUploadFile;
void handleRoot();
void showConfig();
void changeConfig();

bool loadConfig();
bool saveConfig();
void setdispBrightness( int = 20);
 






bool firstboot = false;





void setup() {
  Serial.begin(9600);

  pinMode(ip_button,INPUT);
  Serial.println("");
   if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.println("Mounted FS...");  
  merodisplay.start(); // Start of DMD
  // call set Brightness subroutine , if 
  setdispBrightness();
  merodisplay.setFont(Mono5x7);
  wifiConnect();
  loadnewRate();
  
  //clear display
  merodisplay.drawRect(0,0,63,15,0,0);
 
  



  meroserver.on("/",handleRoot);
  meroserver.on("/wifiUpdate",handlewifiUpdate);
  meroserver.on("/getWifi",handlegetWifi);
  meroserver.on("/bUpdate",handlebUpdate);// brightness update handle
  meroserver.on("/getB",handlegetB); // brightness get handle
  meroserver.on("/rateUpdate",handlerateUpdate);
  meroserver.on("/getRate",handlegetRate);
  meroserver.on("/uploadFile",HTTP_POST,[](){ meroserver.send(200); },handleFileUpload);
  meroserver.on("/fileList",handleFileList);
  meroserver.on("/reStart",handlereStart);
  meroserver.begin();
}

void loop() {
  meroserver.handleClient();
  merodisplay.loop();
  if(updated){
    Serial.println("NEW RATE:");
    Serial.print("Date: ");
    Serial.println(date);
    Serial.print("Date Updated: ");
    Serial.println(date_updated);
    Serial.print("GOLD SELL: ");
    Serial.println(gold_sell);
    Serial.print("GOLD BUY: ");
    Serial.println(gold_buy);
    Serial.print("SILVER SELL: ");
    Serial.println(silver_sell);
    Serial.print("SILVER BUY: ");
    Serial.println(silver_buy);
     merodisplay.drawRect(0,0,63,15,0,0);
     merodisplay.drawText(0,0,gold_buy);
     merodisplay.drawText(0,8,gold_sell);
     merodisplay.drawText(37,0,silver_buy);
     merodisplay.drawText(37,8,silver_sell);
    
    updated = false;
  }
}









void handleRoot(){
    meroserver.sendHeader("Access-Control-Allow-Origin", "*");
    meroserver.sendHeader("Allow", "HEAD,GET,PUT,POST,DELETE,OPTIONS");
    meroserver.sendHeader("Access-Control-Allow-Methods", "GET, HEAD, POST, PUT");
    meroserver.sendHeader("Access-Control-Allow-Headers", "X-Requested-With, X-HTTP-Method-Override, Content-Type, Cache-Control, Accept");
  
  if(LittleFS.exists("/index.html")){
    File indexHTML = LittleFS.open("/index.html", "r");
    if(indexHTML)
    {
    meroserver.streamFile(indexHTML,"text/html");
    indexHTML.close(); 
    }
    
  }
  else{
    meroserver.send(200,"text/html", "This is root dir index file not found");
    }
}











void handlerateUpdate()
{ 
    meroserver.sendHeader("Access-Control-Allow-Origin", "*");
    Serial.print("HTTP Method: ");
    Serial.println(meroserver.method());
    
//    meroserver.sendHeader("Allow", "HEAD,GET,PUT,POST,DELETE,OPTIONS");
//    meroserver.sendHeader("Access-Control-Allow-Methods", "GET, HEAD, POST, PUT");
//    meroserver.sendHeader("Access-Control-Allow-Headers", "X-Requested-With, X-HTTP-Method-Override, Content-Type, Cache-Control, Accept");
    String rate_data = meroserver.arg("plain");
  
  StaticJsonDocument<300> jsonBuffer;
  deserializeJson(jsonBuffer,rate_data);
  int len = measureJson(jsonBuffer);
  char buff[len];
  serializeJson(jsonBuffer, buff, len + 1);

  File rateFile = LittleFS.open("/rate.json", "w");
  if(rateFile.print(buff))
        {
          Serial.println("Rate file was written");
          meroserver.send(200, "application/json", "{\"status\" : \"ok\"}");
          Serial.println(buff);
          
        } 
        else {
          Serial.println("File write failed");
          meroserver.send(200, "application/json", "{\"status\" : \"failed\"}");
        }
  rateFile.close();
  loadnewRate();
}

void loadnewRate()
{

  if(LittleFS.exists("/rate.json")){
    File rateFile = LittleFS.open("/rate.json", "r");
    if(rateFile)
    {
      size_t size = rateFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      rateFile.readBytes(buf.get(), size);
      rateFile.close();

      StaticJsonDocument<300> doc;
      auto error = deserializeJson(doc, buf.get());
      if (error) 
      {
          Serial.println("Failed to parse config file");
          updated = false;
          return;
       }

       date = doc["date"].as<String>();
       date_updated = doc["date_updated"].as<String>();
       gold_buy = doc["gold_buy"].as<String>();
       gold_sell = doc["gold_sell"].as<String>();
       silver_buy = doc["silver_buy"].as<String>();
       silver_sell = doc["silver_sell"].as<String>();

       updated = true;
    }

}
else
{
  gold_buy = "0000";
  gold_sell = "0000";
  silver_buy = "0000";
  silver_sell = "0000";
}
}


void handlegetRate()
{
  meroserver.sendHeader("Access-Control-Allow-Origin", "*");


 if(LittleFS.exists("/rate.json"))
 {
  File file = LittleFS.open("/rate.json", "r");
  
  meroserver.streamFile(file, "text/html");
  file.close();
  
 }

 else{
  meroserver.send(200,"application/json","{\"status\":\"failed\"}");
 }
  

 
  
}


void handlereStart(){
  meroserver.sendHeader("Access-Control-Allow-Origin", "*");
  meroserver.send(200, "application/json", "{\"status\" : \"ok\"}");
  Serial.println("Restarting");
  delay(500); 
  ESP.restart();
}


void handlewifiUpdate()
{ 
    Serial.print("HTTP Method: ");
    Serial.println(meroserver.method());
    meroserver.sendHeader("Access-Control-Allow-Origin", "*");
//    meroserver.sendHeader("Allow", "HEAD,GET,PUT,POST,DELETE,OPTIONS");
//    meroserver.sendHeader("Access-Control-Allow-Methods", "GET, HEAD, POST, PUT");
//    meroserver.sendHeader("Access-Control-Allow-Headers", "X-Requested-With, X-HTTP-Method-Override, Content-Type, Cache-Control, Accept");
    String wifi_data = meroserver.arg("plain");
  
  StaticJsonDocument<100> jsonBuffer;
  deserializeJson(jsonBuffer,wifi_data);
  int len = measureJson(jsonBuffer);
  char buff[len];
  serializeJson(jsonBuffer, buff, len + 1);

  File wifiFile = LittleFS.open("/wifi.json", "w");
  
  if(wifiFile.print(buff))
  
      {
          wifiFile.close();
          
          meroserver.send(200, "application/json", "{\"status\" : \"ok\"}");
  
          Serial.println("Wifi file was written");
          Serial.println(buff);
          //calling wifi connect to connect to new wifi
          wifiConnect();
        } 
        
        else {
          wifiFile.close();
          meroserver.send(200,"application/json","{\"status\" : \"failed\"}");
          Serial.println("Wifi file write failed, ReStarting");
          delay(500);
          //restarting Device
          ESP.restart();
          
        }  
}

void handlegetWifi()
{
  meroserver.sendHeader("Access-Control-Allow-Origin", "*");

  if(LittleFS.exists("/wifi.json"))
      { 

        // file exists now open and serve the file
        
        File wififile = LittleFS.open("/wifi.json", "r");
        meroserver.streamFile(wififile, "text/html");
        wififile.close();
      }
  else
      {
        // file doesn't exist respond failed to ui
        meroserver.send(200,"application/json","{\"status\" :\"failed\"}");
        
      }
}






void wifiConnect()
{
 WiFi.mode(WIFI_AP_STA);
 //disconnecting from previous wifi 
 WiFi.disconnect();

 if(LittleFS.exists("/wifi.json"))
        {
                char * _ssid = "", *_pass = "";
                File wifiFile = LittleFS.open("/wifi.json", "r");
                
                
                if(wifiFile)
                {
                  size_t size = wifiFile.size();
                  std::unique_ptr<char[]> buf(new char[size]);
                  wifiFile.readBytes(buf.get(), size);
                  wifiFile.close();
        
                  StaticJsonDocument<300> doc;
                  auto error = deserializeJson(doc, buf.get());
                  if (error) {
                      Serial.println("Failed to parse wifi file");
                      }
        
//                     _ssid = doc["wifi"];
//                     _pass = doc["pass"];
//          
                     wifiSSID = doc["wifi"];
                     wifiPASS = doc["pass"];
          
                    Serial.println(wifiSSID);
                    Serial.println(wifiPASS);
                    //Begin WiFi connection with the provided ssid and pass
                    WiFi.begin(wifiSSID,wifiPASS);
          
                    connectTimeout = millis() + (WIFI_CONN_TIMEOUT * 1000);
                    while(true)
                    {
                      wifiStatus = WiFi.status();
                  
                      if ((wifiStatus == WL_CONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL) ||
                      (wifiStatus == WL_CONNECT_FAILED) || (millis() >= connectTimeout)) 
                        break;
                  
                      delay(100); 
                    }
    
    
          
          if(wifiStatus != WL_CONNECTED)
                {
                  Serial.println("Couldn't connect to Wifi , Starting Access Point Only");
                  // calling routine to start Acess Point
                  startAP();
                }
          else
                {
                  Serial.print("Connected to WIFI , IP :");
                  Serial.println(WiFi.localIP());
          
                  if(digitalRead(ip_button)== HIGH)
                  {
                    Serial.println("IP BUTTON PRESSED");
                  //merodisplay.drawText(WiFi.localIP().toString());
                  delay(15000);
                  }
          
                  // got connected to WIFI Successfully now returning
                  return;
                } 
                  }
        }
        
        else
        {
          //file doesn't exist starting AP
          startAP(); 
        }
      }

void handlebUpdate(){
  Serial.print("HTTP Method: ");
    Serial.println(meroserver.method());
    meroserver.sendHeader("Access-Control-Allow-Origin", "*");
    String brightness_data = meroserver.arg("plain");
  
  StaticJsonDocument<50> jsonBuffer;
  deserializeJson(jsonBuffer,brightness_data);
  int len = measureJson(jsonBuffer);
  char buff[len];
  serializeJson(jsonBuffer, buff, len + 1);

  File brightnessFile = LittleFS.open("/brightness.json", "w");
  if(brightnessFile.print(buff)){
          
          
          int brightness = jsonBuffer["brightness"].as<signed int>();

          merodisplay.setBrightness(brightness);
          Serial.println("Brightness file was written");
          Serial.print("Brightness Level : ");
          Serial.println(brightness);
          meroserver.send(200, "application/json", "{\"status\" : \"ok\"}");
        } 
        else 
        {
          Serial.println("Brightness file write failed");
          meroserver.send(200, "application/json", "{\"status\" : \"failed\"}");
        }
   brightnessFile.close();
  delay(500);  // restart after delay of 0.5 secs
  ESP.restart(); 
}


void handlegetB(){
  meroserver.sendHeader("Access-Control-Allow-Origin", "*");

   if(LittleFS.exists("/brightness.json"))
      { 
        File brightnessfile = LittleFS.open("/brightness.json", "r");
       // meroserver.sendHeader("Access-Control-Allow-Origin", "*");
        meroserver.streamFile(brightnessfile, "text/html");
        brightnessfile.close();
        
      }

     else
     {
      meroserver.send(200,"application/json","{\"status\":\"failed\"}");
     }
  
  
}

// if config availabe sets according to config if not then as provided in parameter, default 20
void setdispBrightness( int default_brt)
{
  if(LittleFS.exists("/brightness.json")){
    
    File brightnessFile = LittleFS.open("/brightness.json", "r");
    if(brightnessFile){
      size_t size = brightnessFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      brightnessFile.readBytes(buf.get(), size);
      brightnessFile.close();

      StaticJsonDocument<50> doc;
      auto error = deserializeJson(doc, buf.get());
      if (error) {
          Serial.println("Failed to parse brightness file");
        
          }
      else{
        int brightness = doc["brightness"].as<signed int>();

        merodisplay.setBrightness(brightness);
        Serial.print("Brightness: ");
        Serial.println(brightness);
        }       
}
  }

  else{
    Serial.print("can't find Setting default : ");
    Serial.println(default_brt);
    merodisplay.setBrightness(default_brt);
  }
}



void handleFileList()
{
  String path = "/";
  Dir dir = LittleFS.openDir(path);
  String output = "[";

  while(dir.next())
  {
    if (output != "[")
    {
      output += ",";
      }
    output += String(dir.fileName());
    
  }
  output += "]";
  meroserver.sendHeader("Access-Control-Allow-Origin", "*");
  meroserver.send(200,"application/json",output);
}



void startAP()
{
    WiFi.softAP(apssid,appassword);
    IPAddress ip = WiFi.softAPIP();
    Serial.print("IP Address : ");
    Serial.println(ip);
}



void handleFileUpload()
{
//  meroserver.sendHeader("Access-Control-Allow-Origin", "*");
  
  HTTPUpload& upload = meroserver.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if(!filename.startsWith("/"))
      filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = LittleFS.open(filename, "w");
    filename = String();
  } 
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } 
  else if(upload.status == UPLOAD_FILE_END)
  {
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
    meroserver.send(200, "application/json", "{\"status\" : \"ok\"}");
  }

  else{
    meroserver.send(500,"text/plain","couldn't create file");
  }
  
}
