= 【深淵編】BCM2712 SDHC と CYW43455 SDIO（4-bit / 50MHz / CMD53）接続の全貌

Raspberry Pi 5 のオンボード Wi-Fi チップ（Infineon CYW43455）を RTOS から駆動する上で、@<b>{最大の難所のひとつが SDIO（Secure Digital Input Output）インターフェースの立ち上げ} でした。Linux カーネルの肥大化したサブシステムに頼らず、Zephyr のネイティブドライバとしてハードウェアと 1 対 1 で対話し、安定したバルクデータ転送を実現するまでの泥臭い闘いを詳解します。

== Raspberry Pi 5 の SDIO インフラと BCM2712 SDHCI ドライバ

Raspberry Pi 5 では、CYW43455 は SoC（BCM2712）内部の専用 SDHC コントローラにハードウェア配線されています。
筆者は Zephyr のドライバフレームワークに準拠したホストコントローラドライバ @<tt>{drivers/sdhc/sdhc_bcm2712.c} を作成しました。

SDHC コントローラは標準的な @<b>{SDHCI（SD Host Controller Interface）規格} に準拠した MMIO レジスタマップを持ちます：

//table[tbl_11][]{
オフセット	レジスタ名	役割
--------------------------------------------------
@<tt>{0x00}	@<tt>{SDHCI_DMA_ADDRESS}	DMA 転送先バッファ物理アドレス
@<tt>{0x04}	@<tt>{SDHCI_BLOCK_SIZE}	転送ブロックサイズ（例: 64 バイト）
@<tt>{0x06}	@<tt>{SDHCI_BLOCK_COUNT}	転送ブロック数
@<tt>{0x08}	@<tt>{SDHCI_ARGUMENT}	SD コマンドの 32-bit 引数
@<tt>{0x0C}	@<tt>{SDHCI_TRANSFER_MODE}	転送方向（Read/Write）、ブロック/複数ブロック、DMA 有効化
@<tt>{0x0E}	@<tt>{SDHCI_COMMAND}	コマンドインデックス（CMD番号）とレスポンス種別指定
@<tt>{0x10}	@<tt>{SDHCI_RESPONSE}	カードから返却されたレスポンスデータ（R1〜R6）
@<tt>{0x28}	@<tt>{SDHCI_HOST_CONTROL}	1-bit / 4-bit バス幅選択、High-Speed モード指定
@<tt>{0x2C}	@<tt>{SDHCI_CLOCK_CONTROL}	内部クロック発振、SD クロック分周、クロック安定待ち
@<tt>{0x30}	@<tt>{SDHCI_INT_STATUS}	コマンド完了、転送完了、各種エラー割り込みステータス
//}

//emlist{
/* sdhc_bcm2712.c レジスタアクセスヘルパー */
static inline uint32_t sdhc_bcm2712_read(const struct device *dev, uint32_t reg)
{
    return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void sdhc_bcm2712_write(const struct device *dev, uint32_t val, uint32_t reg)
{
    sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}
//}

ホストコントローラの初期化では、まずカードが認識されるまで @<b>{400 kHz の低速初期化クロック} を供給し、カードの初期化完了後に @<b>{25 MHz（Normal-Speed）}、最終的に @<b>{50 MHz（High-Speed）} へと周波数を引き上げます。

== SDIO プロトコル仕様と初期化シーケンス（CMD0 から TRAN 状態へ）

SD メモリカードと異なり、SDIO カード（I/O デバイス）は独自の列挙シーケンスを持ちます。CYW43455 がホストと正常に対話できるようになるまでの通信シーケンスは以下の通りです：

//image[ch03_sdio_sequence][SDIO 初期化シーケンス（CMD0 から TRAN 状態へ）]{
//}

 1. @<b>{CMD0 (GO_IDLE_STATE)}: バスをリセットし、カードを初期状態にします。
 2. @<b>{CMD5 (IO_SEND_OP_COND)}: SDIO 専用コマンドです。引数 0 で送信すると、カードはサポートする電圧範囲（3.3V 等）と I/O ファンクション数（CYW43455 の場合は 3 ファンクション：F0, F1, F2）を @<b>{R4 レスポンス} で返します。その後、電圧ビットを立てて再度 CMD5 を発行し、カード内部の準備が整う（Ready ビットが 1 になる）までポーリングします。
 3. @<b>{CMD3 (SEND_RELATIVE_ADDR)}: カード固有のアドレス（RCA: Relative Card Address、通常 @<tt>{0x0001}）を取得します（@<b>{R6 レスポンス}）。
 4. @<b>{CMD7 (SELECT_CARD)}: 取得した RCA を指定してカードを選択します。カードはスタンバイ状態から @<b>{TRAN（データ転送可能）状態} へと遷移します。

== CCCR レジスタ設定と複数ファンクション（F0/F1/F2）のセットアップ

TRAN 状態に遷移した直後の CYW43455 は、依然として @<b>{1-bit バス幅 / 400kHz} で動作しています。これらを高速通信に切り替えるため、@<b>{CCCR（Card Common Control Registers）} と呼ばれる Function 0（F0）の専用レジスタ空間を設定します。

CYW43455 は以下の 3 つのファンクションを持ち、ホストとの役割分担が明確に分離されています：

//image[ch03_sdio_functions][CYW43455 内部ファンクション（F0/F1/F2）構成とホスト間の役割分担]{
//}

 * @<b>{Function 0 (F0)}: SDIO 共通制御レジスタ（CCCR/FBR）。バス幅やクロック、割り込みの制御。
 * @<b>{Function 1 (F1)}: Infineon バックプレーンアクセス。SoC 内部レジスタ、ChipCommon、RAM へのアクセス。
 * @<b>{Function 2 (F2)}: WLAN データ転送用。Wi-Fi パケット（SDPCM）の送受信を行う。

//emlist{
CCCR レジスタ操作シーケンス (CMD52 経由):
 1. CCCR 0x07 (Bus Interface Control) に 0x02 を書き込み -> 4-bit バス幅を有効化
 2. BCM2712 SDHC の HOST_CONTROL レジスタの 4-bit ビットを立てる
 3. CCCR 0x13 (High-Speed Enable) に 0x02 (EHS) を書き込み -> 50MHz High-Speed を有効化
 4. BCM2712 SDHC の CLOCK_CONTROL を 50MHz に切り替え
 5. CCCR 0x02 (IO Enable) に 0x02 (F1 有効化) を書き込み -> F1 Ready (0x03) をポーリング待機
 6. CCCR 0x02 (IO Enable) に 0x06 (F1+F2 有効化) を書き込み -> F2 Ready (0x03) をポーリング待機
 7. F1 および F2 のブロックサイズレジスタ (CCCR 0x110-0x111, 0x210-0x211) に 64 (0x0040) を設定
//}

== データ転送の心臓部：CMD52 と CMD53 のメカニズム

SDIO 通信におけるデータ転送は、すべて @<b>{CMD52} と @<b>{CMD53} の2つのコマンドで行われます。

=== CMD52 (Direct I/O)
1 バイト単位のレジスタ読み書きに使用します（CCCR 設定や割り込みフラグ確認など）。
 * @<b>{引数構成}: R/W フラグ (1bit) + Function 番号 (3bit) + RAW フラグ (1bit) + レジスタアドレス (17bit) + 書込データ (8bit)
 * @<b>{レスポンス}: @<b>{R5 レスポンス}（カードステータス + 読み出しデータ）

=== CMD53 (Extended I/O)
大容量のデータ（パケットやファームウェアバイナリ）を高速転送するためのコマンドです。
 * @<b>{バイトモード}: 1 〜 512 バイトの任意バイト数を転送（主に F1 の短小レジスタアクセス用）。
 * @<b>{ブロックモード}: 事前に設定したブロックサイズ（64 バイト）の整数倍単位で転送（Wi-Fi パケット送受信の主力）。

CMD53 の 32-bit 引数（Argument）は以下のように極めて緻密に定義されています：

//emlist{
 31   30 28 27   26     25     24 9               8 0
+----+-----+---+------+------+-----+-----------------+
|R/W | Func| B | OP   | Block| Reg | Byte/Block      |
|Flag| No  |   | Code | Mode | Addr| Count           |
+----+-----+---+------+------+-----+-----------------+
//}
 * @<b>{R/W Flag (Bit 31)}: 0 = Read, 1 = Write
 * @<b>{Func No (Bits 30-28)}: 0 = F0, 1 = F1, 2 = F2
 * @<b>{Block Mode (Bit 27)}: 0 = バイトモード, 1 = ブロックモード
 * @<b>{OP Code (Bit 26)}: 0 = 固定アドレスアクセス（FIFO）, 1 = インクリメンタルアドレス
 * @<b>{Reg Addr (Bits 25-9)}: 転送先レジスタアドレス（17-bit）
 * @<b>{Byte/Block Count (Bits 8-0)}: 転送バイト数（最大512）またはブロック数

== 【苦闘録】SDIO 接続で直面した4大難所とその突破法

CYW43455 との接続開発において、最も多くの時間を費やしたトラブルシューティングの実録です。

=== 難所1：CMD53 ブロックモード転送時のブロックアライメントとパディングの罠
Wi-Fi の受信パケットは「138バイト」や「74バイト」など、中途半端なサイズで届きます。
しかし SDIO のブロックモードでは、@<b>{ブロックサイズ（64 バイト）の整数倍でしか転送できません}。
もし 138 バイトの要求に対してそのまま CMD53 を発行すると、ホストコントローラまたはカード側のステートマシンがハングアップします。

@<b>{対応コード}（@<tt>{airoc_whd_hal_sdio.c}）:
ブロック境界に満たない端数を、次のブロック境界へと切り上げるパディングロジックを実装しました：
//emlist{
uint16_t xfer_size = data_size;
if (mode == SDIO_BLOCK_MODE && (data_size % func->block_size) != 0) {
    /* 64バイトの倍数に切り上げ */
    xfer_size = (data_size + func->block_size - 1) & ~(func->block_size - 1);
}
//}

=== 難所2：バウンスバッファの枯渇（2048B → 4096B）と 64-bit キャッシュアライメント
SDIO コントローラが DMA 転送を行う際、転送元・転送先のバッファがキャッシュライン（64 バイト）にアライメントされていないと、キャッシュインバリデート時に周囲のデータが破壊されます。
また、従来の WHD 実装ではバウンスバッファが 2048 バイトで定義されていました：

//emlist{
/* 修正前：2048バイトでは MTU 1500B + 802.11ヘッダ + SDPCMヘッダのブロック切り上げで溢れる危険性 */
static uint8_t cmd53_bounce_buf[2048];
//}

一見すると、イーサネットの標準 MTU は 1500 バイトであり、ヘッダ類（イーサネット 14B + 802.1Q 4B + SDPCM 12B + ハードウェアタグ 4B = 1534B）を 64 バイトブロック境界へ切り上げても 1536 バイト（24ブロック）に収まるため、2048 バイトで足りるように思えます。

しかし、実際の Wi-Fi 通信では以下の要因により 2048 バイトの境界を容易に突破します：
 1. @<b>{A-MSDU（Aggregate MSDU）の受信}: 802.11n/ac では複数のイーサネットフレームが 1 つの物理フレームに集約される場合があり、パケット長が 2KB を超える。
 2. @<b>{WHD ドライバの内部メタデータ領域}: WHD はパケットデータの直前・直後に独自のディスクリプタやオフセットポインタを付加してバッファ渡しを行う。
 3. @<b>{証明書チェーンのバースト転送}: TLS ハンドシェイク時の巨大な Certificate パケットが TCP セグメント境界ギリギリで押し寄せる。

その結果、@<tt>{xfer_size} が 2048 バイトを超過してバウンスバッファから溢れ、パケットドロップや隣接メモリ破壊を誘発していました。

@<b>{【設計思想：メモリ効率と安全性のトレードオフ】}:
組込み Linux であればメガバイト単位の DMA メモリプールを確保して解決するところですが、RAM 制約がシビアな RTOS では無駄なメモリ確保は許されません。
そこで、@<b>{ちょうど MMU の 1 ページ相当かつ 64 バイトブロック × 64 個に合致する「4096 バイト」} を採用しました。これにより、A-MSDU を含む最大サイズのパケットを確実にカバーしつつ、メモリ消費の増加をわずか 2KB に抑え、64-bit ARM の MMU ページ境界とも親和性の高い安全な設計を実現しています。

@<b>{対応コード}:
バウンスバッファを @<b>{4096 バイト} に拡張し、AArch64 の 64 バイトキャッシュラインアライメント属性を付与しました：
//emlist{
static uint8_t cmd53_bounce_buf[4096] __attribute__((aligned(64)));
//}

=== 難所3：ファームウェア（43455-fmac.bin）と CLM BLOB の F1 バックプレーン転送
CYW43455 は起動時、オンチップ ROM のブートローダーしか動いていません。
Wi-Fi として動作させるには、約 500 KB のファームウェアバイナリと、国コード情報（CLM BLOB）を、SDIO Function 1 のバックプレーンウィンドウ経由で Wi-Fi チップの内部 RAM（@<tt>{0x00000000}）へ CMD53 で分割転送しなければなりません。

転送中に 1 ブロックでも CRC エラーやタイムアウトが発生すると、ファームウェアの整合性チェックが失敗してチップが無反応になります。
BCM2712 SDHC ドライバ側で、コマンド完了（@<tt>{SDHCI_INT_STATUS: CMD_COMPLETE}）と転送完了（@<tt>{TRANSFER_COMPLETE}）をステータスレジスタで厳密にハンドシェイクし、タイムアウト監視を行うことで、500 KB のバルクダウンロードをノーエラーで完走させました。

=== 難所4：SDPCM フレームのハードウェアタグ検証とログ氾濫の抑制
ファームウェアが起動すると、CYW43455 はホストとの通信プロトコルとして @<b>{SDPCM（SDIO Packet Communication Manager）} を使用します。
受信用パケットの先頭には、以下のようなハードウェアタグヘッダが付与されています：

//emlist{
+--------------------+--------------------+
| Packet Length (2B) | ~Packet Length (2B)|  <- ビット反転による整合性検証
+--------------------+--------------------+
| SDPCM Header (12B) : Sequence, Channel  |
+-----------------------------------------+
| Ethernet / 802.3 Frame Data ...         |
+-----------------------------------------+
//}

開発初期、パケットを受信するたびにコンソールへ以下のログが出続け、シリアルバッファを圧迫していました：
//emlist{
AIROC: SDPCM Frame valid hwtag: len=60 (0x003c)
AIROC: SDPCM Frame valid hwtag: len=64 (0x0040)
//}
この整合性検証ロジックが正常にパスしていることを確認した上で、不要なデバッグ出力を抑制（ミュート）し、高速なスループットを維持できるように最適化しました。

== Infineon AIROC Wi-Fi (WHD) と 5GHz帯（802.11ac / 702 Mbps）接続の開通

これらの SDIO 層のハードルをすべて乗り越えたことで、Infineon AIROC ドライバスタックが完全に立ち上がりました。

Zephyr Shell から接続を実行：
//emlist{
uart:~$ net wifi connect -s "MyHome5G" -p "Password123" -k 1 -b 2
Connected
//}

@<tt>{net wifi status} で確認した結果：
//emlist{
uart:~$ net wifi status
Status: connected
SSID: MyHome5G
Band: 5GHz
Channel: 36
Security: WPA2-PSK
Link speed: 702 Mbps
//}

SDIO 4-bit / 50MHz High-Speed モードの広大なバス帯域により、@<b>{702 Mbps（802.11ac VHT80 MCS 8/9）} という圧巻のリンク速度が確立されました！

