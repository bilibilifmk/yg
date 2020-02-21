#include <Time.h>
#include <TimeLib.h> 
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <FS.h>
#include <DNSServer.h>

#include <Servo.h>
#include<OneWire.h>
#include<DallasTemperature.h>
Servo myservo;

#define rs D3

#define Trig 4 //Tring连接D2
#define Echo 5 //Echo 连接 D1 
#define duoji D4//d4舵机
#define gongliao 12//d6 供料
#define shuibeng D7//d7水泵
#define deng D8//d8灯光
int dj=20;
//int gl=0;

bool sxh=true,dg=true;
unsigned sy,wd,gl,wsqk; // 水位 温度 供料 水循环 灯光 喂食情况

unsigned wss=0,sls=1,yggds=18,zgwds=40; //喂食代码   食量  鱼缸高度   最高温度

String txhms; //通信号码

int djs,biaoji; //喂食倒计时  标记

unsigned long previousMillis = 0;

const long interval = 10000;

OneWire onewire(14);// 温度传感器 D5
DallasTemperature sensors(&onewire);//初始化传感器
double shuiwei,shuiwen;




IPAddress timeServer(120, 25, 115, 20); // 阿里云ntp服务器 如果失效可以使用 120.25.115.19   120.25.115.20
#define STD_TIMEZONE_OFFSET +8 //设置中国
const int timeZone = 8;     // 修改北京时区
WiFiUDP Udp;
unsigned int localPort = 8888;  // 修改udp 有些路由器端口冲突时修改
int servoLift = 1500;
int pd=1;



//=====================================================================================================================================================
String Hostname = "yg";     //主机名，连上WiFi后通过该值访问8266，必须字母开头，必须每块板子都不同，否则会冲突
String url = Hostname + ".com";    //配网时的访问地址，主机名+.dog，或者直接访问6.6.6.6（尽量用com、cn、org等存在的域名，否则浏览器可能会以搜索执行）
const char *AP_name = "yg";    //配网时8266开启的热点名
int Signal_filtering = -60;   //搜索结果过滤掉信号强度低于这个值的WiFi（不了解勿改动）
//=====================================================================================================================================================

const byte DNS_PORT = 53;
String WiFi_State;    //WiFi配置状态  0：未配置  1：已配置
#define WiFi_State_Addr 0   //记录WiFi配置状态的EEPROM地址

long time_out = millis();

IPAddress apIP(6, 6, 6, 6);
ESP8266WebServer webServer(80);
DNSServer dnsServer;

void setup()
{
  //  中断语句，D3高电平变低电平触发
  attachInterrupt(digitalPinToInterrupt(rs), blink, FALLING);

  Serial.begin(115200);
  Serial.println("");
  EEPROM.begin(10);
  SPIFFS.begin();
  //  设置主机名
  WiFi.hostname(Hostname);
  //pinMode(2, OUTPUT);
  //获取WiFi配置状态

  pinMode(shuibeng, OUTPUT);
  pinMode(deng, OUTPUT);
 
  digitalWrite(shuibeng, HIGH);
  digitalWrite(deng, HIGH);
  
  pinMode(gongliao, INPUT_PULLUP);
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);
   myservo.attach(duoji);
   myservo.write(80); 



  
  WiFi_State = EEPROM.read(WiFi_State_Addr);
  delay(300);
    Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  if (WiFi_State == "1")   //已配置
  {

    EEPROM.begin(4096);
    EEPROM.get(300,wss);  //喂食代码
    EEPROM.get(400,sls);  //食量
    EEPROM.get(500,yggds); //鱼缸高度
    EEPROM.get(600,zgwds); //最高温度
     Serial.println("喂食代码：");
         Serial.println(wss);
    Serial.println("食量：");
         Serial.println(sls);
    Serial.println("鱼缸高度：");
         Serial.println(yggds);
    Serial.println("最高温度：");
         Serial.println(zgwds);
   

    
    Serial.println("Configured!");
    Serial.print("Connecting");
    unsigned millis_time = millis();
    while ((WiFi.status() != WL_CONNECTED) && (millis() - millis_time < 5000))
    {
      delay(250);
      Serial.print(".");
    }
    Serial.println("");
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("http://");
      Serial.println(Hostname);
     // digitalWrite(2, HIGH);
    }
    else
    {
      Serial.println("Connection failed!");
      Serial.println("Please reset!");
     // digitalWrite(2, LOW);
    }

  while( pd==1){
  


setSyncProvider(getNtpTime);
  Serial.println(year());
    Serial.println(month());
      Serial.println(day());
       Serial.println(hour());
        Serial.println(minute());
      
}



    
  }
  else if (WiFi_State == "0")   //未配置
  {
    //digitalWrite(2, LOW);
    WiFi.disconnect(true);
    Serial.println("");
    Serial.print("Start WiFi config \nsoftAP srarted --> ");
    Serial.println(AP_name);
    Serial.print("http://");
    Serial.println(url);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_name);
  }
  else
  {
    Serial.println("First using!");
    EEPROM.write(WiFi_State_Addr, 0);
    EEPROM.commit();
    delay(300);
    //    Serial.println("Reset! Please wait...");
    //    ESP.restart();
    Serial.println("Press the 'rst' button!");
    while (1) {
    //
    }
  }

  // Web开启服务
  webServer.begin();
  Serial.println("HTTP Server started");
  webServer.on("/", wwwroot);
   webServer.on("/sz", sz);
  webServer.on("/wificonfig", wifiConfig);
  webServer.on("/wifiscan", wifiScan);
  webServer.on("/opera", opera);
  
 webServer.on("/sjhq", sjhq); //数据获取
 webServer.on("/zdws", zdws);//喂天
  webServer.on("/wsdw", wsdw);//喂食单位
   webServer.on("/yggd", yggd);//鱼缸高度
    webServer.on("/txhm", txhm);//通信
     webServer.on("/zgwd", zgwd);//最高温度
  // 配置DNS服务
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, url, apIP);

   delay(500);
   

  
}



void zdws(){
String a= webServer.arg("zdws");
wss =a.toInt(); 
EEPROM.put(300,wss); 
 EEPROM.commit();
Serial.println(wss); 
}
void wsdw(){
  String a= webServer.arg("wsdw");
  sls =a.toInt(); 
  EEPROM.put(400,sls); 
 EEPROM.commit();
  Serial.println(sls); 
  }
void yggd(){
  String a= webServer.arg("yggd");
  yggds =a.toInt(); 
  EEPROM.put(500,yggds); 
 EEPROM.commit();
  Serial.println(yggds); 
}
void txhm(){
  String a= webServer.arg("txhm"); 
  txhms =a;
  Serial.println("tx："+a);  
  }
void zgwd(){
  String a= webServer.arg("zgwd");
  zgwds =a.toInt(); 
  EEPROM.put(600,zgwds); 
 EEPROM.commit();
  Serial.println(zgwds);  
  }


void sjhq(){
  String parm = webServer.arg("sjhq");
  if(parm=="sy"){
    webServer.send(200, "text/plain", String(sy));
    }else if(parm=="wd"){ 
      webServer.send(200, "text/plain",String( wd));
      }else if(parm=="gl"){ 
      webServer.send(200, "text/plain",String( gl));
      }else if(parm=="sxh"){ 
      webServer.send(200, "text/plain", String(sxh));
      }else if(parm=="dg"){ 
      webServer.send(200, "text/plain",String( dg));
      }else if(parm=="wsqk"){ 
      webServer.send(200, "text/plain", String(wsqk));
      }else if(parm=="sdws"){ 
        ws();
        djs=wss;
        wsqk=1;
          Serial.println("手动喂食"); 
      webServer.send(200, "text/plain", "ok");
      }else if(parm=="light"){ 

          if(dg)
          {  dg=0;
       
          }
             else
             { dg=1;
           
             }
                 Serial.println("灯光");
         
     digitalWrite(deng, dg);

 
        
      webServer.send(200, "text/plain", "ok");
      }else if(parm=="sxhs"){ 

           if(sxh)
      {    sxh=0;
      }
             else
           {   sxh=1;
            }
             
 digitalWrite(shuibeng, sxh);
        
     Serial.println("水循环");
        
      webServer.send(200, "text/plain", "ok");
      }
  }



//Web服务根目录

void sz(){
  
   File file = SPIFFS.open("/sz.html", "r");
    webServer.streamFile(file, "text/html");
    file.close();
  
  }

void wwwroot()
{
  //  如果已经配置WiFi则访问data目录里的index.html页面，否则访问config.html页面进行配置
  if (WiFi_State == "1")
  {
    File file = SPIFFS.open("/index.html", "r");
    webServer.streamFile(file, "text/html");
    file.close();
  }
  else if (WiFi_State == "0")
  {
    File file = SPIFFS.open("/config.html", "r");
    webServer.streamFile(file, "text/html");
    file.close();
  }
}

//用于WiFi配置，接收ssid、password两个参数
void wifiConfig()
{
  // 判断请求是否存在这两个参数以及是否未配置
  if (webServer.hasArg("ssid") && webServer.hasArg("password") && WiFi_State == "0")
  {
    int ssid_len = webServer.arg("ssid").length();
    int password_len = webServer.arg("password").length();
    // 判断ssid和密码长度是否合理
    if ((ssid_len > 0) && (ssid_len < 33) && (password_len > 7) && (password_len < 65))
    {
      // 把参数获得的String赋值到ssid
      String ssid_str = webServer.arg("ssid");
      String password_str = webServer.arg("password");
      const char *ssid = ssid_str.c_str();
      const char *password = password_str.c_str();
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(password);
      // 开始连接WiFi
      WiFi.begin(ssid, password);
      Serial.print("Connenting");
      //判断是否连接成功，8秒超时
      unsigned long millis_time = millis();
      while ((WiFi.status() != WL_CONNECTED) && (millis() - millis_time < 8000))
      {
        delay(500);
        Serial.print(".");
      }
      // 判断是否连接成功
      if (WiFi.status() == WL_CONNECTED)
      {
        // 连接成功，返回1
        digitalWrite(2, HIGH);
        Serial.println("");
        Serial.println("Connected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("http://");
        Serial.println(Hostname);
        webServer.send(200, "text/plain", "1");
        delay(300);
        WiFi_State = "1";
        // 记录状态到EEPROM：已配置
        EEPROM.write(WiFi_State_Addr, 1);
        EEPROM.commit();
        delay(50);
        //关闭热点
        WiFi.softAPdisconnect();
        delay(5000); ////////////////
        ESP.restart();
      }
      else
      {
        // 连接失败, 返回0
        Serial.println("Connenting failed!");
        webServer.send(200, "text/plain", "0");
      }
    }
    else
    {
      // WiFi密码格式错误
      Serial.println("Password format error");
      webServer.send(200, "text/plain", "Password format error");
    }
  }
  else
  {
    // 请求参数错误
    Serial.println("Request parameter error");
    webServer.send(200, "text/plain", "Request parameter error");
  }
}

//获取WiFi加密类型并返回
String wifi_type(int typecode)
{
  if (typecode == ENC_TYPE_NONE) return "Open";
  if (typecode == ENC_TYPE_WEP) return "WEP ";
  if (typecode == ENC_TYPE_TKIP) return "WPA ";
  if (typecode == ENC_TYPE_CCMP) return "WPA2";
  if (typecode == ENC_TYPE_AUTO) return "WPA*";
}

//扫描WiFi，返回json
void wifiScan()
{
  String req_json = "";
  Serial.println("Scan WiFi");
  int n = WiFi.scanNetworks();
  if (n > 0)
  {
    int m = 0;
    req_json = "{\"req\":[";
    for (int i = 0; i < n; i++)
    {
      if ((int)WiFi.RSSI(i) >= Signal_filtering)
      {
        m++;
        req_json += "{\"ssid\":\"" + (String)WiFi.SSID(i) + "\"," + "\"encryptionType\":\"" + wifi_type(WiFi.encryptionType(i)) + "\"," + "\"rssi\":" + (int)WiFi.RSSI(i) + "},";
      }
    }
    req_json.remove(req_json.length() - 1);
    req_json += "]}";
    webServer.send(200, "text/json;charset=UTF-8", req_json);
    Serial.print("Found ");
    Serial.print(m);
    Serial.print(" WiFi!  >");
    Serial.print(Signal_filtering);
    Serial.println("dB");
  }
}

void opera() {
  if(webServer.arg("opera") == "on"){
    
  }else if(webServer.arg("opera") == "off"){
    
  }
  webServer.send(200, "text/plain", "ok");
}

void loop()
{
 // dnsServer.processNextRequest();
  webServer.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    swsw();//更新数据
    
  }
if(pd==0){
//自动喂食部分
  if(hour()!=0&&minute()!=0){
    biaoji=1;
    }else{
      wsqk=0;//喂食情况
      }
    if(hour()==0&&minute()==0&&biaoji==1){
      --djs;
      if(djs<=0){
        ws;
        djs=wss;
        wsqk=1;
        setSyncProvider(getNtpTime);
        }
    
    biaoji=0;
    }

}

  
}




void ws(){
      Serial.println("喂食");
  for(int k=0;k<=sls;k++){
   for(int i=0;i<=2;i++){  
     myservo.write(90);  
     delay(500);
     myservo.write(dj);
     delay(500);
  }
 }
   Serial.println("喂食结束");
    myservo.write(dj);
  }








void swsw(){
  //水温计算
   float k[15];
for(int i=0;i<15;i++){

  float temp; // 

  digitalWrite(Trig, LOW); //给Trig发送一个低电平
  delayMicroseconds(2);    //等待 2微妙
  digitalWrite(Trig,HIGH); //给Trig发送一个高电平
  delayMicroseconds(10);    //等待 10微妙
  digitalWrite(Trig, LOW); //给Trig发送一个低电平
  
  temp = float(pulseIn(Echo, HIGH)); //存储回波等待时间,
  shuiwei = (temp * 17 )/1000; //水位距离传感器距离
  k[i]=(shuiwei-1)*(100/yggds);
   delay(50);
}
for (int i=0;i<15;i++){
 for(int j=0;j<15-i;i++){
        if(k[j]>k[j+1]){
          float b=k[j+1];
          k[j+1]=k[j];
          k[j]=b;
           }
              }
                }
shuiwei=0;
for(int i=1;i<14;i++) shuiwei+=k[i];
float val=shuiwei/13;
  


   
if(val<=1){ shuiwei =100;}else if(val>=99){shuiwei =0;}else{shuiwei =100-val;}
  
  sensors.requestTemperatures();
  shuiwen=  sensors.getTempCByIndex(0);
  
  sy=shuiwei;
  wd=shuiwen;
  



  gl=digitalRead(gongliao);
if(gl){
  
    Serial.println("供料报警");
  }
  
  Serial.print("缸内剩余水");
  Serial.print(shuiwei);
  Serial.println("%");
  
  Serial.print("目前水温");
  Serial.println(shuiwen);
  }
















void digitalClockDisplay(){
  //
  Serial.print(hour());
  printDigits(minute());
 Serial.println();

}
void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("连接时间服务器");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("时间服务器应答");
      pd=0;
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("链接服务器失败:-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;    
  packetBuffer[3] = 0xEC; 

  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;                
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}




















//中断重置
void blink() {
  Serial.println("Keep 3 seconds to reset!");
  bool res_state = true;
  unsigned int res_time = millis();
  //  等待三秒，如果D3还是下拉，则重置，否则取消
  while (millis() - res_time < 3000)
  {
    if (digitalRead(rs) != LOW)
    {
      res_state = false;
      Serial.println("Cancel reset!");
      break;
    }
  }
  if (res_state == true)
  {
    WiFi.disconnect(true);
    delay(100);
    EEPROM.write(WiFi_State_Addr, 0);
    unsigned a=0;
    EEPROM.put(300,a);  //喂食代码
    EEPROM.put(400,a);  //食量
    EEPROM.put(500,a); //鱼缸高度
    EEPROM.put(600,a); //最高温度


    
    EEPROM.commit();
    delay(300);
    Serial.println("Press the 'rst' button!");
    while (1) {
     
    }
     Serial.println("Reset!");
     ESP.restart();
  }
}
