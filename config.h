#pragma once

// モード選択（SEVERMODEを変更してからそれぞれコンパイルする）
bool SEVERMODE = true;      // 親機モード=true 子機モード=false
bool debug = false;         // スマートメーターにアクセスしないモード

// 親機設定
char* HOST_NAME = "power";   // ホスト名。http://ホスト名.local/ でアクセスできる
const String API_KEY = "****";  // 子機から親機APIにアクセスするためのAPI KEY

// BルートのIDとパスワード
const String B_ROOT_PASS = "****";
const String B_ROOT_AUTH_ID = "****";

// Wi-FiのSSIDとパスワード
const char* WIFI_SSID = "****";
const char* WIFI_PASS = "****";

// Wi-SUN Hatのリビジョン 1=0.1, 2=0.2
const int WISUN_HAT_REV = 1;

// Ambientの設定
const uint16_t AMBIENT_CHID = 000000; // チャネルID
const char* AMBIENT_WKEY = "****"; // ライトキー

// 消費電力設定
const uint16_t WATT_BREAKER = 3000;   // ブレーカー値[W]（赤）
const uint16_t WATT_ALERT = 2000;     // 警告開始値[W]（黄）
