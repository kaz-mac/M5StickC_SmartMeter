/*
  BP35A1.h
  作成 katsushun89さん
  https://github.com/Katsushun89/M5Stack_SmartMeter/tree/simple-IPMV
  LICENSE Apache-2.0

  修正者 Kaz (https://akibabara.com/blog/)
*/
#pragma once
#include <M5Unified.h>
#include "config.h"

class BP35A1 {
private:
    String pass_;
    String auth_ID_;
    String ipv6_addr_;
    String channel_;
    String pan_ID_;

    uint32_t loop_cnt;
    String addr;
    bool should_retry;

public:
    const uint32_t WAIT_TIME = 10 * 1000;
    const uint32_t WAIT_SCAN_TIME = 180 * 1000;
    const uint32_t WAIT_MEASURE_TIME = 30 * 1000;
    const uint32_t SCAN_LIMIT = 15;
    const uint32_t JOIN_LIMIT = 10;
    const uint32_t MEASURE_LIMIT = 5;   //15;
    int MEASURE_LOOP_TIME = 5 * 1000;

    bool debugPrintFlag = true;     // シリアルポートにログを出力する
    uint16_t debugLevel = 0xFFFF;   // ログレベル
    bool noticeFlag = false;
    uint32_t lastWatt = 0;
    uint32_t lastMeasureMillis = 0;
    int stat = 0;   // ステータス 0=なし 1=実行中 2=成功 3=失敗

public:
    BP35A1();
    ~BP35A1() = default;

    void serialPrint(String str, uint16_t lv=0x0001);
    void serialPrintln(String str, uint16_t lv=0x0001) { serialPrint(str+"\n", lv); };
    void command(String cmd);
    void commandBin(byte *cmd, size_t len);
    bool waitExpectedRes(uint32_t wait_time, String expected_str, String *res_str);
    bool waitExpectedRes(uint32_t wait_time, String expected_str);
    unsigned int hexToDec(String hex_string);
    bool noEcho(void);
    bool setWOPT(void);
    bool clearSession(void);
    bool testComm(void);
    bool setPass(void);
    bool setAuthID(void);
    bool scan(uint32_t *loop_cnt, bool *should_retry, String *addr);
    bool setChannel(void);
    bool setPanID(void);
    bool setAddr(String addr);
    bool join(uint32_t *loop_cnt);
    bool activeScan(int step);
    bool activeScan1(void);
    bool activeScan2(void);
    bool activeScan3(void);
    bool activeScan4(void);
    bool getInstantaneousPower(uint32_t *power);
    bool getMeasuredData(uint32_t *power);
    void startAutoMeasure();
};

class DriveContext {
private:
    BP35A1 *bp35a1_;
public:
    DriveContext() = delete;
    explicit DriveContext(BP35A1 *bp35a1_);
    ~DriveContext() = default;
    DriveContext(const DriveContext &other) = delete;
    DriveContext &operator=(const DriveContext &other) = delete;
    BP35A1 *getBP35A1();
};
