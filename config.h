#pragma once

// モード選択（SEVERMODEを変更してからそれぞれコンパイルする）
#define SEVERMODE true      // 親機モード=true 子機モード=false
#define debug false         // スマートメーターにアクセスしないモード

// 親機設定
#define HOST_NAME "power"   // ホスト名。http://ホスト名.local/ でアクセスできる
#define API_KEY "****"  // 子機から親機APIにアクセスするためのAPI KEY

// BルートのIDとパスワード
#define B_ROOT_PASS "****"
#define B_ROOT_AUTH_ID "****"

// Wi-FiのSSIDとパスワード
#define WIFI_SSID "****"
#define WIFI_PASS "****"

// Wi-SUN Hatのリビジョン 1=0.1, 2=0.2
#define WISUN_HAT_REV 1

// Ambientの設定
#define AMBIENT_CHID 000000 // チャネルID
#define AMBIENT_WKEY "****" // ライトキー

// 消費電力設定
#define WATT_BREAKER 3000   // ブレーカー値[W]（赤）
#define WATT_ALERT 2000     // 警告開始値[W]（黄）
