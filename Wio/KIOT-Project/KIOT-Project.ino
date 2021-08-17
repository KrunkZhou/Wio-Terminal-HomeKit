#define _TASK_TIMEOUT
#define _TASK_SLEEP_ON_IDLE_RUN
#define THRESHOLD 40
#define BUZZER_PIN WIO_BUZZER
#include <rpcWiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"
#include <TaskScheduler.h>
#include"LIS3DHTR.h"
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//参数
const char* ssid = "KIOT-Terminal"; //WiFi SSID (部分5Ghz AP可能无法连接)
const char* password =  "KIOT-UYDFGW6IdaW"; //WiFi PW
const char* server_ip = "http://10.0.0.86"; //HomeBridge地址 不要后斜杠以及ssl

//自定义菜单配置
//顺序对应页码 从第2页也就是编号1开始
//不要忘记改变array大小
//{ "编号", "HB端口号", "菜单标题", "向上拨提示文字", "按下提示文字", "向下拨提示文字" }
const String menu_config[7][6] = { { "0", "0", "Home" }, 
                                { "1", "44002", "Living Room", "ON", "Game", "OFF" },
                                { "2", "44003", "Sync Box", "Start", "", "End" },
                                { "3", "44004", "Server Fan", "ON", "", "OFF" },
                                { "4", "44005", "Dinning Room", "ON", "", "OFF" },
                                { "5", "44006", "Nanoleaf", "ON", "Pink", "OFF" },
                                { "6", "44007", "Garage Door", "Open", "", "Close" }
                                };

// 时区offset定义
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -4 = -14400
  // GMT 0 = 0
const int time_offset = -14400;

//不重要参数
const int default_page = 0; //首页
const int info_page = -1; //系统信息页
const int min_page = 1; //自定义菜单第一页
const int max_page = (sizeof(menu_config) / sizeof(menu_config[0])) -1; //自定义菜单最后一页
const int sleep_timer = 1800; //自动关屏时间(秒)
const int imu_disable_time = 5; //关屏幕后禁止抬手亮屏时间(秒)
const int enable_buzzer = 1; //开启提示音
const int enable_welcome_notice = 1; //开启欢迎界面
const int enable_serial = 1; //启用串口信息

//状态变量
int lcd_state = 1; //亮屏状态
int current_page = 0; //当前页面
int pri_page = -1; //上一个页面
int mutli_button_lock = 0; //锁定方向键
int top_button_lock = 0; //锁定上方键
int please_turn_on_lcd = 0; //请求开屏幕
int imu_state = 1; //暂时禁用IMU
uint32_t targetTime = 0; //更新时间计时器
String pri_timeStamp = ""; //上次更新的时间记录

//固定值
const int led = 13;

TFT_eSPI tft;
LIS3DHTR<TwoWire> lis;
Scheduler ts;
WebServer server(80);
void displayAutoOff();
void displayAutoOffDisable();
void imuOn();
Task lcd_auto_off_timer(sleep_timer * TASK_SECOND, TASK_ONCE, &displayAutoOff, &ts, 
                    false,NULL,&displayAutoOffDisable);
Task lcd_off_disable_imu_timer(imu_disable_time * TASK_SECOND, TASK_ONCE, &imuOn, &ts, false);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//初始化
void setup(void) {
  Serial.begin(115200);
  prepare_lcd(); //准备屏幕

  //准备led buzzer
  pinMode(led, OUTPUT);
  digitalWrite(led, 0); //关led
  if (enable_buzzer) pinMode(WIO_BUZZER, OUTPUT);
  
  prepare_button();//准备按键

  //准备wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  check_wifi(1);
  
  //连接完成
  Serial.println("\nThanks for using KIOT Project\nPowered By KRUNK.CN\n");
  if (enable_serial) Serial.println("Connected to "+String(ssid));
  if (enable_serial) Serial.println("IP address: "+WiFi.localIP().toString());
  
  prepare_http_server(); //http服务器
  change_page(); //刷新到首页
  if (enable_welcome_notice) welcome_notice(); //欢迎界面
  
  prepare_imu();//准备IMU
  prepare_timeClient(); //准备NTP时间

  // 屏幕Timeout
  ts.enableAll();
  restartDisplayWake();
}

//循环
void loop(void) {
  if (targetTime < millis()) { //每秒执行一次
    update_time_display(0); //刷新时间
  }
  please_turn_on_lcd_action(); //解决IMU开屏问题
  check_wifi(0); //检查网络
  server.handleClient(); //web服务器
  ts.execute(); //计划任务
  read_button_action(); //按键动作
}

/*
 * 自定义菜单以及动作
 */

//菜单
void page_custom(int page_num){
  switch(page_num){ //页码
    case -111:
      //自定义页面代码 (如果不需要操作箭头的话)
      break;
    default:
      fill_with_arrow_button_enabled();
      draw_title(String(menu_config[page_num][0]) + " "+ String(menu_config[page_num][2]),TFT_RED);
      draw_arrow_button(menu_config[page_num][3],
                        menu_config[page_num][4],
                        menu_config[page_num][5],1);
      break;
  }
}

//菜单动作 上中下
void page_action(int page_num, int up_press_down){
  int result = -111;
  switch(page_num){ //页码
    case -111:
      //自定义页面代码 (如果不需要操作箭头的话)
      break;
    default: //默认菜单按键
      result = muti_button_press_for_hb(page_num,up_press_down);
      return;
  }
}

//HB动作
int muti_button_press_for_hb(int page_num, int up_press_down){
  int result = -111;
  if (up_press_down==1){ //上
    tft.fillTriangle(60, 80, 35, 130, 85, 130, TFT_GREEN);
    result = http_action(menu_config[page_num][1].toInt(), "kiot-button/1"); //1单击
  }else if (up_press_down==2){ //按
    result = http_action(menu_config[page_num][1].toInt(), "kiot-button/3"); //3长按
  }else if (up_press_down==3){ //下
    tft.fillTriangle(60, 200, 35, 150, 85, 150, TFT_RED);
    result = http_action(menu_config[page_num][1].toInt(), "kiot-button/2"); //2双击
  }
  
  if (result==200){ //成功执行
    draw_arrow_button("","","",0);
  }else if(result!=-111){ //服务器连接失败
    
  }
  return result;
}

/*
 * 按键与菜单
 */

//读取按键操作
void read_button_action(){
  if (digitalRead(WIO_KEY_C) == LOW) { // C按键
    toggle_lcd_backlight(lcd_state); //开关屏幕
    button_press_finally("C Key pressed");
  } else if (digitalRead(WIO_KEY_B) == LOW && top_button_lock == 0) { // B按键
    if (current_page != info_page){ // 加载状态页面
      pri_page = current_page;
      current_page = info_page;
      mutli_button_lock = 1;
    }else{ //回到上一个页面
      int temp = pri_page;
      pri_page = current_page;
      current_page = temp;
      mutli_button_lock = 0;
    }
    change_page(); //刷新主屏幕
    button_press_finally("B Key pressed");
  } else if (digitalRead(WIO_KEY_A) == LOW && top_button_lock == 0) { // A按键
    if (current_page!=default_page){
      pri_page = current_page;
      current_page = default_page; //返回首页
      mutli_button_lock = 0;
      change_page();
    }
    button_press_finally("A Key pressed");
  } 
  
  else if (digitalRead(WIO_5S_UP) == LOW && mutli_button_lock == 0) { // 上
    if (current_page>=min_page&&current_page<=max_page){
      page_action(current_page,1);
    }
    button_press_finally("5 Way Up");
  } else if (digitalRead(WIO_5S_DOWN) == LOW && mutli_button_lock == 0) { // 下
    if (current_page>=min_page&&current_page<=max_page){
      page_action(current_page,3);
    }
    button_press_finally("5 Way Down");
  } else if (digitalRead(WIO_5S_PRESS) == LOW && (mutli_button_lock == 0 || current_page == info_page)) { // 按
    if (current_page>=min_page&&current_page<=max_page){
      page_action(current_page,2);
    }else{
      if(current_page == info_page){
        playMusic();
      }
    }
    button_press_finally("5 Way Press");
  } 
  
  else if (digitalRead(WIO_5S_LEFT) == LOW && mutli_button_lock == 0) { // 左
    tft.fillTriangle(255, 27, 275, 17, 275, 37, TFT_WHITE); //左
    pri_page=current_page;
    if (current_page==min_page || current_page>max_page){
      current_page=default_page;
    }else if(current_page <= default_page){ //向左切换页面
      current_page=max_page;
    }else{
      current_page=current_page-1;
    }
    change_page();
    button_press_finally("5 Way Left");
    tft.fillTriangle(255, 27, 275, 17, 275, 37, TFT_BLACK); //左
    draw_right_corner_arrow();
  } else if (digitalRead(WIO_5S_RIGHT) == LOW && mutli_button_lock == 0) { // 右
    tft.fillTriangle(300, 27, 280, 17, 280, 37, TFT_WHITE); //右
    pri_page=current_page;
    if (current_page>=max_page){
      current_page=default_page;
    }else if(current_page==default_page){ //向右切换页面
      current_page=min_page;
    }else{
      current_page=current_page+1;
    }
    change_page();
    button_press_finally("5 Way Right");
    tft.fillTriangle(300, 27, 280, 17, 280, 37, TFT_BLACK); //右
    draw_right_corner_arrow();
  }
}

//换页
void change_page(){
  tft.setFreeFont(&FreeSansBold12pt7b);
  switch (current_page) {
    case default_page:
      //首页
      //tft.fillScreen(TFT_BLACK);
      fill_with_arrow_button_enabled();
      draw_title(menu_config[0][2],TFT_GREENYELLOW);
      draw_right_corner_arrow();
      break;
    case info_page: //系统信息菜单 info page
      fill_without_line();
      tft.setFreeFont(&FreeSans9pt7b);
      tft.setCursor((320 - tft.textWidth("KRUNK.CN 2021"))/2, 225);
      tft.print("KRUNK.CN 2021");
      draw_title("KIOT Terminal Info",TFT_CYAN);
      
      tft.setFreeFont(&FreeSansBold9pt7b);
      tft.drawString("IP ADDR :", 20, 70);
      tft.drawString(WiFi.localIP().toString(), 135, 70);
      
      tft.drawString("SSID :", 20, 100);
      tft.drawString(String(ssid), 135, 100);

      tft.drawString("WiFi MAC :", 20, 130);
      tft.drawString(String(WiFi.macAddress()), 135, 130);

      tft.drawString("Server IP :", 20, 160);
      tft.drawString(server_ip, 135, 160);
      tft.setFreeFont(&FreeSansBold12pt7b);
      break;
    default:
      page_custom(current_page);
      break;
  }
  update_time_display(1);
}

//欢迎
void welcome_notice(){
  fill_with_main_arrow();
  tft.setFreeFont(&FreeSansBold12pt7b);
  //tft.setCursor((320 - tft.textWidth("Hello There !"))/2, 115);
  //tft.print("Hello There !");
  tft.setFreeFont(&FreeSans9pt7b);
//  tft.setCursor((320 - tft.textWidth("HomeKit"))/2, 150);
//  tft.print("HomeKit");
  tft.setCursor((320 - tft.textWidth("Left Right to change pages"))/2, 195);
  tft.print("Left Right to change pages");
  tft.setCursor((320 - tft.textWidth("Up Down to operate actions"))/2, 215);
  tft.print("Up Down to operate actions");
  
//  tft.setCursor((320 - tft.textWidth(WiFi.localIP().toString()))/2, 225);
//  tft.print(WiFi.localIP().toString());
  tft.setFreeFont(&FreeSansBold12pt7b);
  update_time_display(1);
}

/*
 * 网络
 */

void check_wifi(int is_setup){
  //检查网络
  if(WiFi.status() != WL_CONNECTED){
    noti_buzzer();
    //打印正在连接
    tft.fillScreen(TFT_BLACK);
    tft.setCursor((320 - tft.textWidth("Connecting to Wi-Fi.."))/2, 120);
    tft.print("Connecting to Wi-Fi..");
    // 等待连接
    while (WiFi.status() != WL_CONNECTED) {
      toggle_lcd_backlight(0);
      delay(1000);
      if (enable_serial) Serial.print(".");
      if (enable_serial) Serial.println(WiFi.status());
      WiFi.begin(ssid, password);
    }
    if(!is_setup){
      change_page();
    }
  }
}

//http请求
int http_action(int port, String api){
  if((WiFi.status() == WL_CONNECTED)) {
        HTTPClient http;
        if (enable_serial) Serial.print("[HTTP] begin...\n");
        // configure traged server and url
        http.begin(String(server_ip)+":"+port+"/"+api); //HTTP
        if (enable_serial) Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            if (enable_serial) Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                if (enable_serial) Serial.println(payload);
            }
        } else {
            if (enable_serial) Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
        return httpCode;
    }
    return 0;
}

/*
 * 屏幕
 */

void prepare_lcd(){
  tft.begin();
  digitalWrite(LCD_BACKLIGHT, LOW);
  tft.setRotation(3);
  tft.setFreeFont(&FreeSansBoldOblique12pt7b);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("KRUNK.CN KIOT"))/2, 100);
  tft.print("KRUNK.CN KIOT");
  tft.setFreeFont(&FreeSansOblique9pt7b);
  tft.setCursor((320 - tft.textWidth("Experience SmartHome"))/2, 140);
  tft.print("Experience SmartHome");
  tft.setFreeFont(&FreeSansBold12pt7b);
  digitalWrite(LCD_BACKLIGHT, HIGH);
}

//开关屏幕
void toggle_lcd_backlight(int lcd_state_local){
  if(lcd_state_local==1){ //关屏幕
    imuOff(); //关闭抬手亮屏一段时间
    lcd_off_disable_imu_timer.restartDelayed();
    digitalWrite(LCD_BACKLIGHT, LOW);
    lcd_state=0;
    //锁定按键
    mutli_button_lock = 1;
    top_button_lock = 1;
    tft.fillScreen(TFT_WHITE);
  }else{ //开屏幕
    tft.fillScreen(TFT_BLACK);
    change_page();
    digitalWrite(LCD_BACKLIGHT, HIGH);
    //解锁按键
    lcd_state=1;
    mutli_button_lock = 0;
    top_button_lock = 0;
    restartDisplayWake(); //重置屏幕计时器
  }
}

// 屏幕Timeout
void displayAutoOff() {
  toggle_lcd_backlight(1); //关屏幕
  current_page = 0;
  if (enable_serial) Serial.println("Display Auto Off, Going to HomePage");
}

// 屏幕Timeout
void displayAutoOffDisable() {
  if (lcd_auto_off_timer.timedOut()) {
    if (enable_serial) Serial.println("Display Task has timed out");
  } else {
    if (enable_serial) Serial.println("Display timer has been disabled");
  }
}

// 屏幕Timeout
void restartDisplayWake() { 
  if (enable_serial) Serial.println("Display Timeout Task has restarted");
  lcd_auto_off_timer.restartDelayed(); //restart the timer function
}

/*
 * 屏幕 画面以及填充
 */

//画操作箭头
void draw_arrow_button(String a, String b, String c, int if_string){
  tft.setFreeFont(&FreeSansBold9pt7b);
  //上
  tft.fillTriangle(60, 80, 35, 130, 85, 130, TFT_WHITE);
  if (if_string) tft.drawString(a,90,90);
  //中 可选
  if (b!=""&&if_string){
    tft.fillRect(35,135,51,11,TFT_WHITE);
    if (if_string) tft.drawString(b,93,133);
  }else if(if_string){
    tft.fillRect(35,135,51,11,TFT_BLACK);
  }
  //下
  tft.fillTriangle(60, 200, 35, 150, 85, 150, TFT_WHITE);
  if (if_string) tft.drawString(c,90,175);
  tft.setFreeFont(&FreeSansBold12pt7b);
}

//画右上角导航箭头
void draw_right_corner_arrow(){
  tft.drawTriangle(300, 27, 280, 17, 280, 37, TFT_WHITE); //右
  tft.drawTriangle(255, 27, 275, 17, 275, 37, TFT_WHITE); //左
}

//填充下半屏幕 除去操作箭头
void fill_without_main_arrow(){
  tft.fillRoundRect(85, 56, 320, 184, 0, TFT_BLACK);
}
//填充上半屏幕 除去导航箭头
void fill_title_without_nav_arrow(){
  tft.fillRoundRect(0, 0, 254, 55, 0, TFT_BLACK);
}
void fill_title_full_width(){
  tft.fillRoundRect(0, 0, 320, 55, 0, TFT_BLACK);
}
//填充下半部分 包括操作箭头
void fill_with_main_arrow(){
  tft.fillRoundRect(0, 56, 320, 184, 0, TFT_BLACK);
}
//填充线左右
void fill_line_left_right(){
  tft.drawLine(0,55,14,55,TFT_BLACK);
  tft.drawLine(306,55,320,55,TFT_BLACK);
}

//绘画标题与线
void draw_title(String title, uint32_t color){
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.drawString(title,20,20);
  tft.drawLine(15,55,305,55,color);
}

//填充屏幕 除了标题直线 防止闪烁
void fill_without_line(){
  fill_title_full_width();
  fill_with_main_arrow();
  fill_line_left_right();
}

//判断如果上一页在自定义页面范围内就不刷新箭头部分
void fill_with_arrow_button_enabled(){
  if (pri_page==default_page){ //刚开机欢迎页面
    fill_title_without_nav_arrow();
    fill_with_main_arrow();
  }else if(current_page==default_page){ //如果当前为初始页面
    fill_title_without_nav_arrow();
    fill_with_main_arrow();
    fill_line_left_right();
  }else if ((pri_page>=min_page&&pri_page<=max_page)){ //上一页为操作页面
    fill_title_without_nav_arrow();
    fill_without_main_arrow();
  }else{ //上一页为其他页面
    fill_without_line();
  }
  draw_right_corner_arrow(); //画导航箭头
}

/*
 * HTTP 服务器
 */

//准备http服务器
void prepare_http_server(){
  server.on("/", handleRoot);
  server.on("/info", []() {
    server.send(200, "text/plain", "KIOT Device Info\n"+
    String("\nWi-Fi SSID: ")+String(ssid)+
    "\nLocal IP: "+WiFi.localIP().toString()+
    "\nWiFi MAC: "+String(WiFi.macAddress())+
    "\n\nKRUNK.CN KIOT System");
  });
  server.on("/tos", []() {
    server.send(200, "text/plain", "KIOT Device TOS\n"+
    String("\nVisit https://krunk.cn for more infomation")+
    "\n\nKRUNK.CN KIOT System");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  if (enable_serial) Serial.println("HTTP server started");
}

//首页
void handleRoot() {
  digitalWrite(led, 1);
  if (enable_serial) Serial.println("HTTP Request");
  server.send(200, "text/html", "<!doctype html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>KRUNK.CN KIOT</title><link rel='stylesheet' href='https://cdn.bootcss.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://cdn.bootcss.com/jquery/1.11.2/jquery.min.js'></script><script src='https://cdn.bootcss.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body><div class='container' style='margin-top:9%;'><div class='jumbotron'><div class='panel panel-success'><div class='panel-heading'><h2>KIOT 开源智能家庭终端</h2></div></div><p><h5><a href='/info'>设备状态</a></h5><h5><a href='/tos'>使用条款</a></h5><br><h5>Powered By <a href='https://krunk.cn'>KRUNK.CN</a></h5></p></div></div></body></html>");
  digitalWrite(led, 0);
}

//404
void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n\nKRUNK.CN KIOT System";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

/*
 * 按钮
 */

void prepare_button(){
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
}

void button_press_finally(String serial_msg){
  if (enable_serial) Serial.println(serial_msg);
  restartDisplayWake();
  delay(120); //按键延迟 防止连续
}

/*
 * IMU 抬手亮屏
 */

void displayAutoOn() {
  if (imu_state){
    please_turn_on_lcd = 1;
  }
}

void prepare_imu(){
  lis.begin(Wire1);
 
  if (!lis) {
    if (enable_serial) Serial.println("IMU ERROR");
  }
  lis.setOutputDataRate(LIS3DHTR_DATARATE_25HZ); //Data output rate
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G); //Scale range set to 2g
 
  //1 for single click, 2 for double click
  //smaller the threshold value, the more sensitive
  lis.click(1, THRESHOLD);
  //Interrupt signal to trigger when a tap is detected!
  attachInterrupt(digitalPinToInterrupt(GYROSCOPE_INT1), displayAutoOn, RISING);
}

void imuOff(){
  if (enable_serial) Serial.println("IMU Off");
  imu_state=0;
}

void imuOn(){
  if (enable_serial) Serial.println("IMU On");
  imu_state=1;
}

void please_turn_on_lcd_action(){
  if(please_turn_on_lcd == 1 && lcd_state == 0){
    if (imu_state == 1){
      current_page = default_page; //回首页
      toggle_lcd_backlight(0); //开屏幕
      if (enable_serial) Serial.println("Display IMU Auto On");
      noti_buzzer();
    }
    please_turn_on_lcd = 0;
  }
}

/*
 * Buzzer
 */

//提示音
void noti_buzzer(){
  if (enable_buzzer){
    analogWrite(WIO_BUZZER, 128);
    delay(50);
    analogWrite(WIO_BUZZER, 0);
  }
}

void playMusic() {
  pinMode(BUZZER_PIN, OUTPUT);
  int length = 14; //the number of notes
  char notes[] = "ccggaagffeeddc";
  int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2 };
  int tempo = 300;

  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("Enjoy!"))/2, 100);
  tft.print("Enjoy!");
  for(int i = 0; i < length; i++) {
    if(notes[i] == ' ') {
      delay(beats[i] * tempo);
    } else {
      playNote(notes[i], beats[i] * tempo);
    }
    delay(tempo / 6); //delay between notes
  }
  change_page();
}
 
//Play tone
void playTone(int tone, int duration) {
    for (long i = 0; i < duration * 1000L; i += tone * 2) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(tone);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(tone);
    }
}
 
void playNote(char note, int duration) {
    char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
    int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };
 
    // play the tone corresponding to the note name
    for (int i = 0; i < 8; i++) {
        if (names[i] == note) {
            playTone(tones[i], duration);
        }
    }
}

/*
 * 时间
 */

void prepare_timeClient(){
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(time_offset);
  if (!timeClient.update()) {
    timeClient.forceUpdate();
  }
}

//打印以及更新时间
void update_time_display(int force_update){
  if (!timeClient.update()) {
    timeClient.forceUpdate(); //更新时间
  }
  targetTime = millis() + 1000; // 每秒执行一次
  String formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  String timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-4);//时间不带秒

  if(timeStamp!=pri_timeStamp || force_update){
    if (current_page==default_page){ //首页大时间
      String dayStamp = formattedDate.substring(0, splitT);//日期
      tft.setFreeFont(&FreeSansBold24pt7b);
      tft.setTextColor(TFT_BLACK);
      tft.setCursor((320 - tft.textWidth(pri_timeStamp))/2, 120);
      tft.print(pri_timeStamp);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor((320 - tft.textWidth(timeStamp))/2, 120);
      tft.print(timeStamp);

      tft.setFreeFont(&FreeSansBold12pt7b);
      tft.setCursor((320 - tft.textWidth(dayStamp))/2, 160);
      tft.print(dayStamp);
      
      tft.setFreeFont(&FreeSansBold12pt7b);
    }else{ //右下角小时间
      tft.setTextColor(TFT_BLACK);
      tft.drawString(pri_timeStamp, 252, 210);
      tft.setTextColor(TFT_WHITE);
      tft.drawString(timeStamp, 252, 210);
    }
    pri_timeStamp = timeStamp;
  }
}
