/*
  M5StickC_SmartMeter.ino
  スマートメーター電力表示 for M5StickC series

  Copyright (c) 2025 Kaz  (https://akibabara.com/blog/)
  Released under the MIT license.
  see https://opensource.org/licenses/MIT

  BP35A1ライブラリ katsushun89さん（BP35A1.hのみApache-2.0ライセンス）
  https://github.com/Katsushun89/M5Stack_SmartMeter/tree/simple-IPMV
  https://qiita.com/KatsuShun89/items/7a09917bb813b42bb9d1

  Wi-SUN Hat（BP35A1変換アダプタ） rin_ofumiさん
  https://kitto-yakudatsu.com/archives/7206
  https://github.com/rin-ofumi/m5stickc_wisun_hat
*/

#include <M5Unified.h>
#include "config.h"
#include <WiFi.h>

// Webサーバー関連
#include <ESP32WebServer.h>   // https://github.com/Pedroalbuquerque/ESP32WebServer
#include <ESPmDNS.h>
#include "html.h"
ESP32WebServer server;

// Ambient関連
#include <Ambient.h>
Ambient ambient;

// Webクライアント関連
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
WiFiClient client;

// BP35A1関連
#include "BP35A1.h"
BP35A1 *bp35a1;

// ウォッチドックタイマー
#define WDT_TIMER_SETUP 5*60  // setup()でのタイムアウト [sec] 5分
#define WDT_TIMER_LOOP  5*60  // loop()でのタイムアウト  [sec] 5分
hw_timer_t *wdtimer0 = NULL;

// 設定
#define MEASURE_TIME 5000   // スマートメーター測定間隔(ms)
#define AMBIENT_SEND_TIME 5*60*1000   // Ambient送信間隔(ms)

// グローバル変数
int stickc = -1;   // -1=不明 0=無印 1=Plus 2=Plus2
uint32_t watt = 0;
uint32_t lastUpdate = 0;
String measureDate = "";

//----------------------------------------------------------------------

// Wi-Fi接続する
bool wifiConnect() {
  bool stat = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi connecting.");
    for (int j=0; j<10; j++) {
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASS);  //  Wi-Fi APに接続
      for (int i=0; i<10; i++) {
        if (WiFi.status() == WL_CONNECTED) break;
        Serial.print(".");
        delay(500);
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("connected!");
        Serial.println(WiFi.localIP());
        stat = true;
        break;
      } else {
        Serial.println("failed");
        WiFi.disconnect();
      }
    }
  }
  return stat;
}

// mDNSにホスト名を登録する
bool mdnsRegister(char* hostname) {
  for (int i=0; i<100; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(50);
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin(hostname)) {
      Serial.println("mDNS registered. http://"+String(hostname)+".local/");
      return true;
    }
  }
  Serial.println("mDNS failed.");
  return false;
}

// BEEP音を鳴らす
void beep(unsigned long duration=100) {
  M5.Speaker.tone(2000, duration);
  delay(duration);
}

// LCDに実行結果を表示する
void lcdViewResult(bool res, String passstr, String failstr, uint16_t passwait=0, uint16_t failwait=2000) {
  String str = res ? passstr : failstr;
  if (str != "") M5.Lcd.println(str);
  delay( res ? passwait : failwait);
}

// LCDにインジケーターを表示する
void lcdIndicator(int lv) {
  uint16_t cols[4] = { TFT_BLACK, TFT_YELLOW, TFT_GREEN, TFT_RED };
  uint16_t col = cols[lv];
  uint16_t x = M5.Lcd.width() - 5;
  M5.Lcd.fillCircle(x,5, 4, col);
}

// 経過時間を文字列で返す
String ago2Str(uint32_t lastms) {
  uint32_t ss = (millis() - lastms) / 1000;
  char buff[11];
  String str = "";
  uint32_t hh = ss / 3600;
  ss -= hh * 3600;
  uint32_t mm = ss / 60;
  ss -= mm * 60;
  sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
  return String(buff);
}

// システムを再起動する
void IRAM_ATTR wdt_reboot() {
  Serial.println("\n*** REBOOOOOOT!!");
  //M5.Power.reset(); // ←なんでこれ無いん？
  esp_restart();
}

// WDTのカウンターをクリアする
void wdt_clear() {
  // timerWrite(wdtimer0, 0);
  timerRestart(wdtimer0);
}

//----------------------------------------------------------------------

// Web トップページ：PATH=/
void handleRoot() {
  String html = String(HTML_CPANEL);
  html.replace("[API_KEY]", API_KEY);
  server.send(200, "text/html", html);
}

// Web API 消費電力を返す：PATH=/api/status
void apiStatus() {
  if (server.arg("key") == API_KEY) {
    DynamicJsonDocument json(128);
    String responseData;
    json["success"] = 1;
    json["watt"] = watt;
    json["date"] = measureDate;
    serializeJson(json, responseData);
    server.send(200, "application/json", responseData);
  } else {
    server.send(403, "text/html", "Forbidden");
  }
}

// Web API リセットする：PATH=/api/reset
void apiReset() {
  if (server.arg("key") == API_KEY) {
    server.send(200, "text/plain", "1");
    delay(3000);
    wdt_reboot();
  } else {
    server.send(403, "text/html", "Forbidden");
  }
}

// 親機から消費電力を取得する
bool getRemoteApiStatus() {
  HTTPClient http;
  DynamicJsonDocument json(128);
  bool res = false;

  String url = "http://"+String(HOST_NAME)+".local/api/status?key="+String(API_KEY);
  http.setTimeout(15000);
  http.begin(client, url);
  auto code = http.GET();
  DeserializationError error = deserializeJson(json, http.getString());
  Serial.println("API "+String(code)+" "+url);
  if (!error) {
    if (json.containsKey("success") && json["success"].as<int>() == 1) {
      watt = json["watt"].as<int>();
      measureDate = json["date"].as<String>();
      Serial.println("API result "+String(watt)+" [W] at "+ measureDate);
      res = true;
    }
  }
  if (! res) {
    watt = 0;
    measureDate = "error "+String(code);
  }
  return res;
}

// 親機にリセット指令を出す
void requestServerReset() {
  HTTPClient http;
  String url = "http://"+String(HOST_NAME)+".local/api/reset?key="+API_KEY;
  http.setTimeout(5000);
  http.begin(client, url);
  auto code = http.GET();
  Serial.println("API "+String(code)+" "+url);
}

//----------------------------------------------------------------------

// セットアップ
void setup() {
  M5.begin();
  delay(1000);
  bool res;

  // 機種判定
  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickC      : stickc = 0; break;
    case m5::board_t::board_M5StickCPlus  : stickc = 1; break;
    case m5::board_t::board_M5StickCPlus2 : stickc = 2; break;
  }
  
  // LCD設定
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextFont( (stickc == 0) ? 0 : 2);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextScroll(true);
  M5.Lcd.setScrollRect(5, 3, M5.Lcd.width()-10, M5.Lcd.height()-6);
  M5.Lcd.println("System Start!");

  // シリアルポートの初期化
  Serial.begin(115200);
  Serial.println("System Start!");

  // ウォッチドックタイマー setup()用
  // wdtimer0 = timerBegin(0, 80, true);
  // timerAttachInterrupt(wdtimer0, &wdt_reboot, true);
  // timerAlarmWrite(wdtimer0, WDT_TIMER_SETUP*1000000, false);
  // timerAlarmEnable(wdtimer0);
  wdtimer0 = timerBegin(1000000);  // 80MHz / 80 = 1MHz = 1000000 Hz
  timerAttachInterrupt(wdtimer0, &wdt_reboot);
  timerAlarm(wdtimer0, WDT_TIMER_SETUP*1000000, false, 0);

  // Wi-Fi接続
  M5.Lcd.print("Wi-Fi..");
  res = wifiConnect();
  lcdViewResult(res, "OK", "FAIL");

  // 親機モードのセットアップ
  if (SEVERMODE) {
    // mDNSにホスト名を登録する
    M5.Lcd.print("mDNS Regist..");
    res = mdnsRegister(HOST_NAME);
    lcdViewResult(res, "OK", "FAIL");

    // Webサーバーの設定
    M5.Lcd.print("Web Server..");
    server.on("/", handleRoot);
    server.on("/api/status", apiStatus);
    server.on("/api/reset", apiReset);
    server.begin();
    M5.Lcd.println("OK");

    // Ambientの初期化
    M5.Lcd.print("Ambient..");
    ambient.begin(AMBIENT_CHID, AMBIENT_WKEY, &client);
    M5.Lcd.println("OK");

    // 第2シリアルポートの初期化
    if (stickc >= 0) { 
      if (WISUN_HAT_REV == 1) {
        pinMode(25, INPUT);   // Wi-SUN Hat rev0.1 では25をオープンにする
        Serial2.begin(115200, SERIAL_8N1, 36, 0);   // 外部GPIO Wi-SUN Hat rev0.1 RX=36 TX=0
      } if (WISUN_HAT_REV == 2) {
        Serial2.begin(115200, SERIAL_8N1, 26, 0);   // 外部GPIO Wi-SUN Hat rev0.2 RX=26 TX=0
      }
    } else {
      Serial.println("Unknown Board");
    }
    delay(100);

    // BP35A1の初期化
    bp35a1 = new BP35A1();
    for (int k=0; k<9999; k++) {
      if (debug) break;
      if (k > 0) {
        M5.Lcd.println("retry!!");
      }
      M5.Lcd.println("BP35A1 init start");
      // BP35A1 接続テスト
      M5.Lcd.print("testComm..");
      for (int i=0; i<120; i++) {
        res = bp35a1->testComm();
        lcdViewResult(res, "OK", "", 0, 1000);
        if (res) break;
      }
      if (! res) {
        M5.Lcd.println("FAIL");
        continue;
      }
      // コマンドエコーバックをオフにする
      M5.Lcd.print("noEcho..");
      bp35a1->noEcho();
      lcdViewResult(res, "OK", "FAIL");
      // BP35A1 データのASCIIモード設定（自動判定ver）
      M5.Lcd.print("setWOPT..");
      res = bp35a1->setWOPT();
      lcdViewResult(res, "OK", "FAIL");
      if (! res) continue;
      // 以前のPANAセッションを解除
      M5.Lcd.print("clearSession..");
      res = bp35a1->clearSession();
      lcdViewResult(res, "OK", "skip", 0, 0);
      // パスワードの設定
      M5.Lcd.print("setPass..");
      res = bp35a1->setPass();
      lcdViewResult(res, "OK", "FAIL");
      if (! res) continue;
      // IDの設定
      M5.Lcd.print("setAuthID..");
      res = bp35a1->setAuthID();
      lcdViewResult(res, "OK", "FAIL");
      if (! res) continue;
      // BP35A1 アクティブスキャン
      for (int i=0; i<4; i++) {
        M5.Lcd.print("activeScan"+String(i+1)+"..");
        res = bp35a1->activeScan(i);
        lcdViewResult(res, "OK", "FAIL");
        if (! res) break;
      }
      if (! res) continue;
      // 自動測定のタスクを開始する
      bp35a1->debugLevel = 0x0002;    // ログレベル bit2=測定結果のみ出力
      bp35a1->startAutoMeasure();
      M5.Lcd.println("ALL OK!\nPlease wait..");
      delay(1000);
      break;
    }
  }

  // 子機モードのセットアップ
  if (! SEVERMODE) {
    M5.Lcd.println("ALL OK!\nPlease wait..");
    delay(1000);
  }

  // ウォッチドックタイマー設定 loop()用
  wdt_clear();
  // timerAlarmWrite(wdtimer0, WDT_TIMER_LOOP*1000000, false); //set time in us
  timerAlarm(wdtimer0, WDT_TIMER_LOOP*1000000, false, 0); //set time in us

  lastUpdate = millis();
  M5.Lcd.setTextScroll(false);
}

//----------------------------------------------------------------------

// メインループ
void loop() {
  M5.update();
  if (SEVERMODE) server.handleClient();
  wdt_clear(); // WDTタイマークリア

  uint16_t lcdw, lcdh, fx, fy, bx, by, wx, wy, gx, gy, gw, gh, gcol, gbar, gbarb;
  float scale = 1.0;
  bool refresh = false;
  bool success = false;
  bool execok = false;
  static uint32_t wattall = 0;
  static uint32_t wattmax = 0;
  static int wattcnt = 0;
  static int indicator = 0;
  static int oldstat = -1;

  // 5秒ごとに立つフラグ
  static unsigned long tm = 0;
  if (tm < millis()) {
    execok = true;
    tm = millis() + MEASURE_TIME + random(100, 500);
  }

  // Aボタン1秒長押しでリセット
  if (M5.BtnA.pressedFor(1000)) {
    M5.Lcd.clear(TFT_WHITE);
    beep(500);
    if (! SEVERMODE) requestServerReset(); // 子機の場合は親機にもリセット指令を出す
    delay(3000);
    wdt_reboot();
  }

  // 親機：瞬時電力を取得（データがあるときしか処理しない＝実質5秒おき）
  if (SEVERMODE) {
    if (! debug) {
      if (bp35a1->getMeasuredData(&watt) == true) {
        lastUpdate = millis();
        success = true;
        refresh = true;
      }
      if (millis() - lastUpdate > 10*60*1000) {  // 10分以上データが受信されなかったらリセットする
        Serial.println("Something is wrong");
        delay(1000);
        wdt_reboot();
      }
    }
    if (! success && execok) {
      refresh = true;
    } 
  }
  measureDate = ago2Str(lastUpdate)+" ago";

  // 子機：瞬時電力を取得（5秒に1回実行）
  if (!SEVERMODE) {
    if (execok) {
      lcdIndicator(1);
      if (!debug && getRemoteApiStatus()) {
        //lastUpdate = millis();
        success = true;
        refresh = true;
      }
      lcdIndicator(0);
    }
  }

  // 親機：ステータスの変化をインジケーターに表示
  if (SEVERMODE) {
    if (bp35a1->stat != oldstat) {
      indicator = bp35a1->stat;
      oldstat = indicator;
      refresh = true;
    }
  }

  // 座標
  lcdw = M5.Lcd.width();  // c 160 / plus 240
  lcdh = M5.Lcd.height(); // c  80 / plus 135
  fx = 55;
  fy = 75;
  if (stickc == 0) {  // M5StickC
    scale = 0.45;
    gh = 14;
  } else {  // M5StickC Plus, Plus2
    scale = 0.7;
    gh = 20;
  }
  bx = 5;
  by = (lcdh - (fy * scale)) / 2;

  // 画面表示
  if (refresh) {
    M5.Lcd.clear(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE);
    if (SEVERMODE) lcdIndicator(indicator);
    // 測定時刻
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString(measureDate, 5, 2, &fonts::Font0);
    // 消費電力
    wx = (fx * scale) * 5 + 2;
    wy = (fy * scale) - 26;
    M5.Lcd.setTextSize(scale);  // for Font8 (55x75)
    M5.Lcd.drawRightString(String(watt), bx+wx-4, by, &fonts::Font8);
    M5.Lcd.setTextSize(1);  // for Font4 (14x26)
    M5.Lcd.drawString("W", bx+wx, by+wy, &fonts::Font4);
    // グラフ
    gx = 5;
    gy = lcdh - gh - 3;
    gw = lcdw - gx * 2;
    gbar = (watt / (WATT_BREAKER * 1.1)) * (gw - 2);
    gbarb = (WATT_BREAKER / (WATT_BREAKER * 1.1)) * (gw - 2);
    gcol = (watt < WATT_ALERT) ? TFT_GREEN : ((watt < WATT_BREAKER) ? TFT_YELLOW : TFT_RED);
    M5.Lcd.drawRect(gx, gy, gw, gh, TFT_SILVER);
    M5.Lcd.fillRect(gx+1, gy+1, gbar, gh-2, gcol);
    M5.Lcd.drawLine(gbarb, gy, gbarb, gy+gh-1, TFT_YELLOW);
  }

  // 受信成功ならAmbientに送信する（5分に1回実行、初回は10回測定後）
  static unsigned long tm2 = 0;
  if (SEVERMODE && success && !debug) {
    wattall += watt;
    if (watt > wattmax) wattmax = watt;
    wattcnt++;
    if (tm2 < millis() && wattcnt > 10) {
      uint32_t wattav = wattall / wattcnt;  // 5分間の平均値
      if (wattav > 0 && wattmax > 0) {
        Serial.println("Send to Ambient Ave:"+String(wattav)+" W, Max:"+String(wattmax)+" W");
        ambient.set(1, (int)wattav);    
        ambient.set(2, (int)wattmax);
        ambient.send();   // Ambientに送信
      }
      wattall = 0;
      wattmax = 0;
      wattcnt = 0;
      tm2 = millis() + AMBIENT_SEND_TIME;
    }
  }

  delay(10);
}

