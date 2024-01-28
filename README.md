# M5StickCでスマートメーターから消費電力を取得して表示する

Wi-SUN通信モジュール BP35A1 を使って自宅のスマートメーターから現在の消費電力を取得します。複数のM5StickC（無印またはPlus、Plus2）で表示しつつ、[Ambient](https://ambidata.io/)にデータを送信してグラフで見ることもできます。

# 想定する環境
* rin_ofumiさんの[Wi-SUN HATキット](https://kitto-yakudatsu.com/archives/7206)での使用を想定しています。Wi-SUN HATキットに必要な部品の情報や組み立て方、BルートのIDやパスワードの取得についてはリンク先のページに詳しい説明があるので参考にしてください。
* 開発環境はArduino IDEです。Wi-SUN HATキットで紹介されているUIFlowではありませんので、UIFlowを書き込まないようにご注意ください。

# 必要なもの
あらかじめ以下のものが必要です。

## 必要なハードウェア
1. M5StickC（無印またはPlus、Plus2）
2. 子機を追加したい場合はM5StickCをもう一つ
3. BP35A1 Wi-SUN通信モジュール
4. [Wi-SUN HATキット](https://kitto-yakudatsu.com/archives/7206)
5. Wi-Fi環境

## 集めておく情報
1. 電力会社に申請したBルートのIDとパスワード
2. Wi-FiのSSID、パスワード
3. [Ambient](https://ambidata.io/)のチャンネルIDとライトキー

# config.hを編集する
1. Bルートの情報、Wi-Fiの情報、Ambientの情報を自身の環境に合わせて設定してください。
2. API_KEYは子機が親機にアクセスする際の認証キーです。hogehogeとか適当な文字列でかまいません。
3. WISUN_HAT_REVはお持ちのWi-SUN HATキットの基板Revに合わせて修正してください。
4. ブレーカーのアンペア数に合わせてWATT_BREAKERやWATT_ALERTも変更します。

## 親機のコンパイル方法
SEVERMODE = true にしてコンパイルします。親機はスマートメーターにアクセスしてデータを取得するほか、Ambientにデータを送信したり、内蔵Webサーバーから消費電力情報を返すAPIを提供します。

## 子機のコンパイル方法
SEVERMODE = false にしてコンパイルします。子機は親機のAPIにhttpでアクセスしてデータを取得します。子機を使わない場合は親機のみでもかまいません。

# データの外部提供

## 子機からのアクセス
子機と親機はWi-Fiを使用してLAN内で通信を行います。デフォルトで親機のホスト名は設定済みのため、そのままコンパイルすれば使用できます。

## ブラウザからのアクセス
デフォルトでは親機のURLは http://power.local/ になっています。powerの部分を変更したい場合はconfig.hのHOST_NAMEを変更してください。

## JSONによるアクセス
http://power.local/api/status?key=キー で取得できます。キーはconfig.hのAPI_KEYに設定した内容です。

## Ambientでの閲覧
Ambientで作成したチャンネルでグラフを見ることができます。グラフのデータ送信は5分間に1回で、d1が平均値、d2が最大値です。

# そのほか
* デバッグ情報をシリアルポートに出力しているので、うまく動かない場合は、シリアルモニターを見ながらどこがだめなのか確認するといいと思います。
* Aボタン長押しでM5StickCをリセットします。子機をリセットすると親機もリセットするようになってます。データが更新されなくなってしまったときなどにどうぞ。
* 右上に緑色の丸が表示されているときは、データの取得中です。

## 既知の問題
* Wi-SUN HATキットは基板Rev.0.1しか持ってないので、Rev.0.2で動作するかどうかは未確認です。動かない場合は修正してみてください。
* 値が更新されなくなることがよくありますが原因はわかりません。対策として一定回数以上取得できなかったら再起動するようにしてますが、ほんとに再起動するかまでは確認してません。

## TODO
* スマートメーターのデータ取得とWebサーバーを別のCPUで動かしてマルチタスク化したい。（処理が終わるまで待ち時間が発生してるので）
* APIのデータ取得日時をunixtimeで返すようにしたい。（子機の方で遅延が確認できるようにしたい）
* Ambientを使わない選択肢も加える

# 変更履歴
* 2023-12-30 1stリリース
* 2024-01-28 Wi-SUN通信マルチタスク化、NTP廃止、config.h仕様変更

Wi-SUN通信を別タスクで動かすようにしたので、子機がデータを取得できずにタイムアウトしにくくなりました。NTPは廃止して、何秒前に取得したか相対的な表示にしました。
それから、config.hの書き方が変わってるので、旧バージョンを使用している場合は新しく作り直してください。

# ライセンス
BP35A1.h 以外は MIT LICENSE です。<br>
BP35A1.h は Apache-2.0 LICENSE です。

# 謝辞
BP35A1のライブラリは katsushun89さんが作成したプログラムを修正して使用させていただきました。<br>
Wi-SUN HATの使用方法やプログラム上のテクニックなどはrin_ofumiさんのサイトやプログラムを参考にさせていただきました。<br>
有益な情報に感謝いたします。
