/*
  BP35A1.cpp
  作成 katsushun89さん
  https://github.com/Katsushun89/M5Stack_SmartMeter/tree/simple-IPMV
  LICENSE Apache-2.0

  修正者 Kaz (https://akibabara.com/blog/)
*/
#include "BP35A1.h"

DriveContext::DriveContext(BP35A1 *bp35a1_) : bp35a1_{bp35a1_} {}
BP35A1 *DriveContext::getBP35A1() { return bp35a1_; }

BP35A1::BP35A1(void){
    pass_ = B_ROOT_PASS;
    auth_ID_ = B_ROOT_AUTH_ID;
    ipv6_addr_ = "";
    channel_ = "";
    pan_ID_ = "";
}

// シリアルポートにログを出力する
void BP35A1::serialPrint(String str, uint16_t lv) {
    if (! debugPrintFlag) return;
    if (!(debugLevel & lv)) return;
    Serial.print(str);
}


// コマンドを実行する ASCII版
void BP35A1::command(String cmd) {
    serialPrintln(">> " + cmd);
    Serial2.println(cmd);
    delay(100);
}

// コマンドを実行する BINARY版
void BP35A1::commandBin(byte *cmd, size_t len) {
    serialPrintln(">> "+String(reinterpret_cast<char*>(cmd)));
    Serial2.write(cmd, len);
    delay(100);
}

// 指定した文字が来るまで応答を待つ
bool BP35A1::waitExpectedRes(uint32_t wait_time, String expected_str, String *res_str){
    uint32_t tm = millis() + wait_time;
    if (tm < millis()) tm = wait_time - 0xFFFFFFFF - millis();
    
    while (millis() < tm) {
        delay(50);
        *res_str = Serial2.readStringUntil('\0');
        if(res_str->length() == 0) continue;

        serialPrintln("<< " + *res_str);
        if(res_str->indexOf(expected_str) != -1) {
            return true;
        }
        // 以下、例外的に終了させたい文字列
        if(res_str->indexOf("EVENT 22") != -1) break;
        if(res_str->indexOf("EVENT 24") != -1) break;
        //FAILでbreakしたくないケースがあれば関数を分離する
        if(res_str->indexOf("FAIL") != -1) break; 
    }
    return false;
}

// 指定した文字が来るまで応答を待つ　簡易版
bool BP35A1::waitExpectedRes(uint32_t wait_time, String expected_str){
    String res_str;
    return waitExpectedRes(wait_time, expected_str, &res_str);
}

// Converting from Hex to Decimal:
//
// NOTE: This function can handle a positive hex value from 0 - 65,535 (a four digit hex string).
//       For larger/longer values, change "unsigned int" to "long" in both places.
unsigned int BP35A1::hexToDec(String hex_string) {
    unsigned int dec_value = 0;
    int next_int;
    
    for (uint32_t i = 0; i < hex_string.length(); i++) {
        
        next_int = int(hex_string.charAt(i));
        if (next_int >= 48 && next_int <= 57) next_int = map(next_int, 48, 57, 0, 9);		//0-9
        if (next_int >= 65 && next_int <= 70) next_int = map(next_int, 65, 70, 10, 15);		//A-F
        if (next_int >= 97 && next_int <= 102) next_int = map(next_int, 97, 102, 10, 15);	//a-f
        next_int = constrain(next_int, 0, 15);
        dec_value = (dec_value * 16) + next_int;
        //serialPrintln(dec_value);
    }
    return dec_value;
}

// コマンドエコーバックをオフにする
bool BP35A1::noEcho(void) {
    command("SKSREG SFE 0");
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
    if (! is_received_res){
        serialPrintln("noEcho() nores err");
        return false;
    }
    serialPrintln("noEcho() ok");
    return true;
}

//データ部の表示形式（00：バイナリ表示、01：ASCII表示）
bool BP35A1::setWOPT(void) {
    bool binmode = false;
    command("ROPT");
    String res_str;
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK", &res_str);
    if (is_received_res) {
        if (res_str.startsWith("OK 00")) {
            serialPrintln("setWOPT()) ROPT Binary mode now");
            binmode = true;
        } if (res_str.startsWith("OK 01")) {
            serialPrintln("setWOPT()) ROPT Ascii mode now");
        } else {
            serialPrintln("unknown result: "+res_str);
        }
    } else {
        serialPrintln("setWOPT()) ROPT nores err");
        return false;
    }
    // バイナリモードだったら変更する
    if (binmode) { 
        command("WOPT 01");
        bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
        if(!is_received_res){
            serialPrintln("setWOPT()) WOPT nores err");
            return false;
        }
        serialPrintln("setWOPT() WOPT OK");
    }
    return true;
}

// 以前のPANAセッションを解除
bool BP35A1::clearSession(void) {
    command("SKTERM");
    String res_str;
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK", &res_str);
    if (res_str.startsWith("OK")) {
        serialPrintln("clearSession()) old session cleared!");
    } if (res_str.startsWith("FAIL ER10")) {
        serialPrintln("clearSession()) no old session");
    } else {
        serialPrintln("clearSession() other result: "+res_str);
        return false;
    }
    return true;
}

// バージョンを取得する（単に応答があるか調べるだけ）
bool BP35A1::testComm(void){
    command("SKVER");
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
    if(!is_received_res){
        serialPrintln("testcomm() nores err");
        return false;
    }
    serialPrintln("testcomm() ok");
    return true;
}

// BP35A1::パスワードを設定する
bool BP35A1::setPass(void){
    command("SKSETPWD C " + pass_);
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
    if(!is_received_res){
        serialPrintln("setPass() nores err");
        return false;
    }
    serialPrintln("setPass() OK");
    return true;
}

// BルートIDを設定する
bool BP35A1::setAuthID(void){
    command("SKSETRBID " + auth_ID_);
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
    if(!is_received_res){
        serialPrintln("setAuthID() nores err");
        return false;
    }
    serialPrintln("setAuthID() OK");
    return true;
}

// スキャンを開始する
bool BP35A1::scan(uint32_t *loop_cnt, bool *should_retry, String *addr){
    //PairIDは、B ROOT IDの末尾8バイトが自動的に設定される
    command("SKSCAN 2 FFFFFFFF 3");
    String res_str;
    bool is_received_res = waitExpectedRes(WAIT_SCAN_TIME, "EPANDESC", &res_str);
    if(!is_received_res){
        //もし拡張ビーコン応答より先に"EVENT 22"（スキャン終了）が返ってきたらリトライする
        if(res_str.startsWith("EVENT 22")){
            (*loop_cnt)++;
            *should_retry = true;
            serialPrintln("retry scan");
        }else{
            serialPrintln("scan res is not EVENT 22");
            serialPrintln(res_str.c_str());
        }
        return false;
    }
    uint32_t offset = 0;

    // 応答があったチャンネル、Pan ID、アドレスを格納する
    res_str = res_str.substring(res_str.indexOf("Channel"));
    offset = res_str.indexOf(":") + 1;
    channel_ = res_str.substring(offset, offset + 2);

    res_str = res_str.substring(res_str.indexOf("Pan ID"));
    offset = res_str.indexOf(":") + 1;
    pan_ID_ = res_str.substring(offset, offset + 4);

    res_str = res_str.substring(res_str.indexOf("Addr"));
    offset = res_str.indexOf(":") + 1;
    *addr = res_str.substring(offset, offset + 16);

    serialPrintln("scan() OK, addr:" + *addr);
    return true;
}

// チャンネルを設定する
bool BP35A1::setChannel(void){
    command("SKSREG S2 " + channel_);
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
    if(!is_received_res){
        serialPrintln("setChannel() nores err");
        return false;
    }
    serialPrintln("setChannel() OK");
    return true;
}

// Pan IDを設定する
bool BP35A1::setPanID(void){
    command("SKSREG S3 " + pan_ID_);
    bool is_received_res = waitExpectedRes(WAIT_TIME, "OK");
    if(!is_received_res){
        serialPrintln("setPanID() nores err");
        return false;
    }
    serialPrintln("setPanID() OK");
    return true;
}

// アドレスを設定する
bool BP35A1::setAddr(String addr){
    String tmp_addr = "";
    command("SKLL64 " + addr);
    bool is_received_res = waitExpectedRes(WAIT_TIME, ":" + pan_ID_, &tmp_addr);
    if(!is_received_res){
        serialPrintln("setAddr() nores err");
        return false;
    }
    ipv6_addr_ = tmp_addr.substring(0, 39);
    serialPrintln("setAddr() OK ipv6_addr_:" + ipv6_addr_);
    return true;
}

// 接続する
bool BP35A1::join(uint32_t *loop_cnt){
    command("SKJOIN " + ipv6_addr_);
    String res_str;
    bool is_received_res = waitExpectedRes(WAIT_TIME, "EVENT 25");
    if(!is_received_res){
        (*loop_cnt)++;
        serialPrintln("retry join");
        return false;
    }
    serialPrintln("join() OK");
    return true;
}

// アクティブスキャン
bool BP35A1::activeScan(int step) {
    if (step == 0) return activeScan1();
    else if (step == 1) return activeScan2();
    else if (step == 2) return activeScan3();
    else if (step == 3) return activeScan4();
    return false;
}

// アクティブスキャン (1) スキャンの開始
bool BP35A1::activeScan1(void) {
    serialPrintln(__func__);
    loop_cnt = 0;
    addr = "";
    should_retry = false;
    while(loop_cnt <= SCAN_LIMIT){
        /* EVENT 22返ってこなくてもリトライするならshould_retry要らないかも */
        if(scan(&loop_cnt, &should_retry, &addr)){
            break;
        }
    }
    if(loop_cnt >= SCAN_LIMIT){
        serialPrintln("activeScan() scan limit over");
        return false;
    }
    serialPrintln("scan res recv");
    return true;
}

// アクティブスキャン (2) アクティブスキャンが完全に終了するまで待つ
bool BP35A1::activeScan2(void) {
    if (! should_retry) {
        bool is_received_res = waitExpectedRes(WAIT_SCAN_TIME, "EVENT 22");
        if(!is_received_res){
            serialPrintln("scan not complete");
            return false;
        }
    }
    serialPrintln("scan success");
    return true;
}

// アクティブスキャン (3) 仮想レジスタのレジスタ番号S2に拡張ビーコン応答で得られたチャンネル（自端末が使用する周波数の論理チャンネル番号）を設定する
bool BP35A1::activeScan3(void) {
    if(setChannel() == false) return false;
    if(setPanID() == false) return false;
    if(setAddr(addr) == false) return false;
    return true;
}

// アクティブスキャン (4) 接続する
bool BP35A1::activeScan4(void) {
    loop_cnt = 0;
    while(loop_cnt < JOIN_LIMIT){
        if(join(&loop_cnt)){
            break;
        }
    }
    if(loop_cnt >= JOIN_LIMIT){
        serialPrintln("activeScan() join limit over");
        return false;
    }
    serialPrintln("activeScan() OK");
    return true; 
}

// 消費電力を取得する
bool BP35A1::getInstantaneousPower(uint32_t *power){
    serialPrintln(__func__);
    const uint32_t DATA_STR_LEN = 14;
    char data_str[DATA_STR_LEN] = {"0"};
    stat = 0;

    //EHD
    data_str[0] = char(0x10);
    data_str[1] = char(0x81);
    //TID
    data_str[2] = char(0x00);
    data_str[3] = char(0x01);
    //SEOJ
    data_str[4] = char(0x05);
    data_str[5] = char(0xff);
    data_str[6] = char(0x01);
    //DEOJ
    data_str[7] = char(0x02);
    data_str[8] = char(0x88);
    data_str[9] = char(0x01);
    //ESV(0x62:プロパティ値読み出し)
    data_str[10] = char(0x62);

    //瞬時電力計測値
    //OPC(1個)
    data_str[11] = char(0x01);
    //EPC
    data_str[12] = char(0xe7);
    //PDC
    data_str[13] = char(0x00);

    String data_str_len = String(DATA_STR_LEN, HEX);
    data_str_len.toUpperCase();
    uint32_t str_len = data_str_len.length(); 
    for(uint32_t i = 0; i < 4 - str_len; i++){
        data_str_len = "0" + data_str_len;
    }
    //serialPrintln("data_str_len:" + data_str_len);
    
    String com_str = "SKSENDTO 1 " + ipv6_addr_ + " 0E1A 1 " + data_str_len + " ";
    byte com_bytes[com_str.length()+DATA_STR_LEN+1];
    com_str.getBytes(com_bytes, com_str.length() + 1);
    for(uint32_t i = 0; i < DATA_STR_LEN; i++){
        com_bytes[com_str.length() + i] = data_str[i];
    }
    
    uint32_t loop_cnt = 0;
    do{
        stat = 1;
        String measure_value;
        commandBin(com_bytes, com_str.length() + DATA_STR_LEN);
        String expected_res = "1081000102880105FF01";
        bool is_received_res = waitExpectedRes(WAIT_MEASURE_TIME, expected_res, &measure_value);
        if(!is_received_res){
            stat = 3;
            serialPrintln("measure nores err", 0x0002);
            loop_cnt++;
            continue;
        }

        uint32_t offset = measure_value.indexOf(expected_res);
        measure_value = measure_value.substring(offset + expected_res.length());
        serialPrintln("EDATA = " + measure_value);
        if(!(measure_value.indexOf("72") != -1 || measure_value.indexOf("52") != -1)){
            stat = 3;
            serialPrintln("measure res data err", 0x0002);
            loop_cnt++;
            continue;
        }

        serialPrintln("measure res OK");
        stat = 2;

        offset = measure_value.indexOf("E7");
        String hex_power = measure_value.substring(offset + 4, offset + 4 + 8);
        serialPrintln(hex_power);
        unsigned int hex_power_char = hexToDec(hex_power);
        serialPrintln("IPMV: " + String(hex_power_char) + " [W]", 0x0002);
        
        *power = static_cast<uint32_t>(hex_power_char);
        return true; 
    }while(loop_cnt < MEASURE_LIMIT);

    return false;
}

// 自動測定で取得したデータがあれば取得する（未受信ならpowerは代入しない）
bool BP35A1::getMeasuredData(uint32_t *power) {
    if (noticeFlag) {
        *power = lastWatt;
        noticeFlag = false;
        return true;
    }
    return false;
}

// 自動測定のタスク（バックグラウンドで実行している）
void autoMeasure(void *args) {
    DriveContext *ctx = reinterpret_cast<DriveContext *>(args);
    BP35A1 *bp35a1_ = ctx->getBP35A1();
    uint32_t awatt = 0;

    for (;;) {
        if (bp35a1_->getInstantaneousPower(&awatt) == true) {
            bp35a1_->noticeFlag = true;
            bp35a1_->lastWatt = awatt;
            bp35a1_->lastMeasureMillis = millis();
        }
        delay(1000);
        bp35a1_->stat = 0;
        delay(bp35a1_->MEASURE_LOOP_TIME - 1000);
    }
}

// 自動測定のタスクを開始する
void BP35A1::startAutoMeasure() {
    DriveContext *ctx = new DriveContext(this);
    xTaskCreateUniversal(
        autoMeasure,  // Function to implement the task
        "autoMeasure",// Name of the task
        2048,             // Stack size in words
        ctx,              // Task input parameter
        2,                // Priority of the task
        NULL,             // Task handle.
        CONFIG_ARDUINO_RUNNING_CORE
    );
}


