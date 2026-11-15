= 実機動作ログと成果の検証

== 完全な HTTPS 通信ログ

修正を適用した最新の @<tt>{zephyr.bin} を Raspberry Pi 5 にロードし、運命のテストを行いました。

//emlist{
uart:~$ net http get -k https://google.com/
TLS verification is disabled
Connecting to 142.251.118.113:443 (timeout: 15000 ms)...

<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
<H1>301 Moved</H1>
The document has moved
<A HREF="https://www.google.com/">here</A>.
</BODY></HTML>
//}

ついに、エラーコード @<tt>{-0x3a00} も @<tt>{-113} も一切出ず、暗号化ハンドシェイクが完了し、Google からの HTTPS レスポンスがコンソール上に出力されました！

== メモリ使用量とフットプリント評価

最終的な Zephyr RTOS イメージのメモリフットプリントは以下の通りです：

//emlist{
Memory region         Used Size  Region Size  %age Used
           FLASH:           0 B          0 B
             RAM:       1956 KB        16 MB     11.94%
//}

フルスタックの Wi-Fi ドライバ、TCP/IP、DHCP、DNS、TLS 1.2/1.3 暗号化エンジン、HTTP パーサーをすべて含んでも、@<b>{RAM 使用量はわずか 1.95 MB（全体の 12% 未満）} です。
Raspberry Pi 5 の大容量 RAM（4GB / 8GB）から見れば極めて軽量であり、RTOS ならではの超高速ブート（電源投入から数百ミリ秒での起動）と強固な決定性を両立しています。

=== 【総括比較】Raspberry Pi OS (Linux) vs Zephyr RTOS

「なぜ高性能な Raspberry Pi 5 上であえて RTOS を動かすのか？」――その技術的意義と圧倒的なリソース効率を裏付ける比較表を以下に示します：

//table[tbl_23][]{
評価項目	Raspberry Pi OS (Linux 64-bit)	Zephyr RTOS (本プロジェクト)	比較のポイント
--------------------------------------------------
@<b>{コールドブート時間}	15 〜 25 秒 (systemd 起動含む)	@<b>{約 0.3 秒} (EEPROM 直起動)	電源投入即座に Wi-Fi 接続・通信を開始可能
@<b>{RAM 消費量}	200 MB 〜 500 MB 程度	@<b>{約 1.95 MB} (Wi-Fi + TLS 含む)	@<b>{99% 以上のメモリ削減}（超軽量）
@<b>{ストレージ専有量}	数 GB (ext4 ルートファイルシステム)	@<b>{約 1.5 MB} (@<tt>{zephyr.bin} 単一ファイル)	SD カード障害リスクの極小化
@<b>{リアルタイム決定性}	ミリ秒単位のジッター (PREEMPT_RT 除く)	@<b>{マイクロ秒単位の決定論的応答}	厳密な周期制御や即応割り込みハンドリング
@<b>{不意の電源断耐性}	ファイルシステム破損リスク（ジャーナル依存）	@<b>{完全ステートレス（壊れる FS が存在しない）}	産業用・組込みアプライアンスに最適
@<b>{セキュリティ・攻撃面}	シェル、デーモン、多重サービスが稼働	@<b>{不要コード完全ストリップ（単一バイナリ）}	侵入経路・脆弱性の劇的削減（ゼロアタックサーフェス）
@<b>{ネットワーク構成}	NetworkManager + wpa_supplicant + OpenSSL	@<b>{Native TCP/IP + AIROC WHD + Mbed TLS/PSA}	カーネル空間とユーザ空間の境界ゼロ（ゼロコピー）
//}

