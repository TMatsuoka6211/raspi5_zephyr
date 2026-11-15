= ハードウェア記述と構成管理 ― デバイスツリー（DeviceTree）と prj.conf（Kconfig）の神髄

Zephyr RTOS を理解し、自在に制御する上で最も基本的でありながら強力な2大要素が @<b>{「デバイスツリー（DeviceTree）」} と @<b>{「prj.conf（Kconfig）」} です。
多くの開発者が「Linux の DTS と何が違うのか？」「なぜこの Kconfig 設定が必要なのか？」で躓きがちです。本章では、本プロジェクトにおける実際の設定ファイルをベースに、その内部アーキテクチャと設計意図を完全に解き明かします。

== Zephyr におけるデバイスツリーの基礎（Linux DTS との決定的な違い）

Linux におけるデバイスツリー（@<tt>{.dtb}）は、OS 起動時にブートローダーからメモリ上に渡され、Linux カーネルが実行時に動的パース（ランタイム探索）してドライバをプローブします。そのため、パース処理のオーバーヘッドやメモリ消費が存在します。

一方、@<b>{Zephyr RTOS のデバイスツリーは「ビルド時に C 言語のマクロ（プリプロセッサ定数）へと完全展開」されます}：
 1. ビルドシステム（Python スクリプト @<tt>{gen_defines.py} 等）が、DTS/DTSI ファイル群をパースして AST（抽象構文木）を生成します。
 2. ハードウェアのレジスタアドレス、割り込み番号、クロック、GPIO ピンなどが、すべて @<tt>{devicetree_generated.h} の中に @<tt>{#define DT_N_S_...} という静的マクロとして出力されます。
 3. C 言語のドライバコードは、実行時ではなく @<b>{コンパイル時に直値としてこれらを参照} します。

これにより、@<b>{ランタイムメモリ消費量ゼロ、探索オーバーヘッドゼロ、デッドコードの完全ストリッピング（不要なドライバが 1 バイトもバイナリに残らない）} という、リソースの制約が厳しい RTOS に最適なゼロコスト抽象化を実現しています。

//image[ch02_dts_model][Linux と Zephyr における DeviceTree 処理方式の比較]{
//}

== 本プロジェクトにおけるデバイスツリー設計の実例

Raspberry Pi 5 のボード定義 @<href>{file:///home/matsuoka/zephyr-workspace/zephyr/boards/raspberrypi/rpi_5/rpi_5_bcm2712.dts, boards/raspberrypi/rpi_5/rpi_5_bcm2712.dts} および SoC 定義 @<href>{file:///home/matsuoka/zephyr-workspace/zephyr/dts/arm64/broadcom/bcm2712.dtsi, dts/arm64/broadcom/bcm2712.dtsi} では、Wi-Fi 通信に関わるノードが以下のように記述されています：

//emlist{
/* dts/arm64/broadcom/bcm2712.dtsi (SoCレベルの定義) */
sdhci1: mmc@1001100000 {
    compatible = "brcm,bcm2712-sdhci";
    reg = <0x10 0x01100000 0x1000>;
    interrupts = <GIC_SPI 274 IRQ_TYPE_LEVEL IRQ_DEFAULT_PRIORITY>;
    clocks = <&clk_uart>;
    #address-cells = <1>;
    #size-cells = <0>;
    status = "disabled";
};

/* boards/raspberrypi/rpi_5/rpi_5_bcm2712.dts (ボードレベルの定義) */
&sdhci1 {
    status = "okay";
    airoc_wifi: airoc-wifi {
        compatible = "infineon,airoc-wifi";
        status = "okay";
        wifi-reg-on-gpios = <&gio 28 GPIO_ACTIVE_HIGH>;
    };
};
//}

=== この記述の重要ポイント：
 1. @<b>{64-bit 物理アドレス空間 } (@<tt>{reg = <0x10 0x01100000 0x1000>}):
   - BCM2712 は 64-bit SoC であるため、アドレス指定は上位 32-bit (@<tt>{0x10}) と下位 32-bit (@<tt>{0x01100000}) のペアで表現されます。実効物理アドレスは @<tt>{0x1001100000}、サイズは @<tt>{0x1000}（4KB） です。
 2. @<b>{割り込み番号 } (@<tt>{interrupts = <GIC_SPI 274 ...>}):
   - ARM Generic Interrupt Controller（GICv2/v3）の Shared Peripheral Interrupt（SPI）番号 @<b>{274} が SDHC コントローラに割り当てられています。
 3. @<b>{親子関係によるバインディング}:
   - SDHC ホストコントローラ（@<tt>{sdhci1}）の子ノードとして @<tt>{airoc_wifi} が定義されています。これにより、Zephyr は「CYW43455 は SDIO バス（sdhci1）に接続されたスレーブデバイスである」ことを静的に認識します。
 4. @<b>{電源制御ピン } (@<tt>{wifi-reg-on-gpios = <&gio 28 GPIO_ACTIVE_HIGH>}):
   - CYW43455 の電源投入・ハードウェアリセットを司る @<tt>{REG_ON} ピンが、RP1 I/O コントローラ（@<tt>{gio}）の @<b>{GPIO 28} に接続されていることが指定されています。ドライバ起動時、この GPIO を High にドライブすることでチップがウェイクアップします。

== C言語ドライバとの自動バインディング（DT_DRV_COMPAT）の仕組み

Zephyr のデバイスドライバは、ソースコード内で @<tt>{DT_DRV_COMPAT} マクロを定義することで、デバイスツリーノードと自動的に結びつきます。

筆者が作成した @<tt>{drivers/sdhc/sdhc_bcm2712.c} の冒頭を見てみましょう：

//emlist{
#define DT_DRV_COMPAT brcm_bcm2712_sdhci

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sys/device_mmio.h>

/* デバイスインスタンスの自動初期化マクロ */
#define SDHC_BCM2712_INIT(n)                                                   \
    static const struct sdhc_bcm2712_config sdhc_bcm2712_config_##n = {        \
        DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                  \
    };                                                                         \
    static struct sdhc_bcm2712_data sdhc_bcm2712_data_##n;                     \
    DEVICE_DT_INST_DEFINE(n, sdhc_bcm2712_init, NULL,                          \
                          &sdhc_bcm2712_data_##n,                              \
                          &sdhc_bcm2712_config_##n, POST_KERNEL,               \
                          CONFIG_SDHC_INIT_PRIORITY, &sdhc_bcm2712_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_BCM2712_INIT)
//}

 * @<tt>{DT_DRV_COMPAT} に @<tt>{brcm_bcm2712_sdhci} を指定すると、デバイスツリー内で @<tt>{compatible = "brcm,bcm2712-sdhci"} かつ @<tt>{status = "okay"} となっているノードの数だけ、@<tt>{DT_INST_FOREACH_STATUS_OKAY} によって初期化構造体とドライバ登録関数（@<tt>{DEVICE_DT_INST_DEFINE}）が@<b>{コンパイル時に自動生成}されます。
 * @<tt>{DEVICE_MMIO_ROM_INIT} により、DTS に書かれた物理アドレス（@<tt>{0x1001100000}）がカーネル内部の仮想アドレスへ自動マッピング（MMU ページテーブル設定）され、ドライバ側は @<tt>{sdhc_bcm2712_read(dev, SDHCI_COMMAND)} のようにオフセットだけで安全にアクセスできるようになります。

== @<tt>{prj.conf}（Kconfig）の体系的解説と階層構造

@<tt>{prj.conf} はアプリケーション固有のカーネル構成を指定するファイルですが、適当にフラグを並べるだけでは依存関係（Dependency Chain）の不一致でビルドエラーや意図しない機能欠落を引き起こします。

本プロジェクトの @<tt>{prj.conf} は、以下の@<b>{5つの明確なレイヤ}で階層設計されています：

//emlist{
[Layer 5: アプリケーション層]  CONFIG_HTTP_CLIENT=y, CONFIG_SHELL=y
         │
[Layer 4: セキュリティ / 暗号層] CONFIG_MBEDTLS=y, CONFIG_PSA_CRYPTO=y, 楕円曲線 (secp384r1, X25519)
         │
[Layer 3: トランスポート / ネットワーク層] CONFIG_NET_TCP=y, CONFIG_DNS_RESOLVER=y, CONFIG_NET_DHCPV4=y
         │
[Layer 2: L2 リンク / Wi-Fi 管理層] CONFIG_WIFI=y, CONFIG_WIFI_AIROC=y, CONFIG_NET_L2_ETHERNET=y
         │
[Layer 1: ハードウェア / SDIO 層] CONFIG_SDHC=y, CONFIG_SDHC_BCM2712=y, CONFIG_SDIO_STACK=y
//}

=== 各レイヤの設計ポイント：
 1. @<b>{L1（ハードウェア層）}:
   - @<tt>{CONFIG_SDHC=y} および @<tt>{CONFIG_SDIO_STACK=y} により、Zephyr の SDIO サブシステムが有効化され、BCM2712 SDHC ホストコントローラがカード列挙を開始します。
 2. @<b>{L2（Wi-Fi 管理層）}:
   - @<tt>{CONFIG_WIFI_AIROC=y} により Infineon WHD が組み込まれ、SDIO 経由でファームウェアがロードされます。@<tt>{CONFIG_WIFI_STA_AUTO_DHCPV4=y} を有効にすることで、AP 接続完了イベントをトリガーとして自動的に L3 の DHCP クライアントが起動します。
 3. @<b>{L3（ネットワーク層）}:
   - @<tt>{CONFIG_NET_TCP=y} でネイティブ TCP スタックを有効化。@<tt>{CONFIG_DNS_RESOLVER=y} で DNS クライアントを有効化し、DHCP から取得した DNS サーバへの問い合わせを可能にします。
 4. @<b>{L4（セキュリティ層）}:
   - Mbed TLS 3.x は PSA Crypto と密結合しているため、@<tt>{CONFIG_PSA_CRYPTO=y} と @<tt>{CONFIG_ENTROPY_GENERATOR=y} が必須です。また、後述の通り主要な暗号スイートと Named Curves（楕円曲線）を明示的に有効化します。
 5. @<b>{L5（アプリケーション層）}:
   - @<tt>{CONFIG_HTTP_CLIENT=y} により、HTTP/1.1 パーサーおよびソケット通信ヘルパーを有効化します。

== パケットバッファ数・スタックサイズの見積もり計算根拠

@<tt>{prj.conf} の中で、システムの安定動作（クラッシュ防止）に直結するのが @<b>{「バッファプール数」} と @<b>{「スタックサイズ」} の見積もりです。

=== 1. ネットワークバッファプールの見積もり
//emlist{
CONFIG_NET_PKT_RX_COUNT=32
CONFIG_NET_BUF_RX_COUNT=128
CONFIG_NET_PKT_TX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=128
//}
 * @<tt>{net_pkt}: 1つのネットワークパケット（IPパケット）全体のメタデータ構造体（コンテキスト情報、タイムスタンプ等）。
 * @<tt>{net_buf}: 実際のパケットペイロードを断片（フラグメント）として格納する固定長データバッファ（デフォルト 128 バイト）。
 * @<b>{サイジング根拠}:
  - Wi-Fi の MTU は 1500 バイトです。1 つのフルサイズパケット（1500B）を保持するには、@<tt>{1500 ÷ 128 ≒ 12 個} の @<tt>{net_buf} が消費されます。
  - TCP 通信では、ウィンドウサイズ（例: 4〜8パケット）分のセグメントが一時的にキューイングされます。さらに TLS 1.3 ハンドシェイク時には、Google から 3〜4 KB の証明書チェーンが連続して到着します（@<tt>{4000 ÷ 128 ≒ 32 個} の @<tt>{net_buf} を一瞬で消費）。
  - もしデフォルト値（@<tt>{net_buf = 16}）のままだと、@<b>{証明書チェーンを受信した瞬間にバッファが枯渇し、パケットが即座にドロップされて接続タイムアウト（-116 ETIMEDOUT）} に陥ります。
  - そのため、受信バッファには最低でも @<tt>{128 個}（16 KB 分の受信用データ保持領域）を確保しました。

=== 2. 64-bit ARM におけるスタックサイズの見積もり
//emlist{
CONFIG_SHELL_STACK_SIZE=16384
CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=8192
CONFIG_NET_TCP_WORKQ_STACK_SIZE=8192
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
//}
 * 32-bit Cortex-M では 1024〜2048 バイトで十分だったスタックが、なぜ 8192〜16384 バイトも必要なのか？
  - @<b>{レジスタ退避の巨大化}: AArch64 では汎用レジスタ（x0〜x30）が 64-bit（8バイト）幅です。関数呼び出しや例外ハンドラでのレジスタ保存だけで、32-bit 環境の倍のスタックを消費します。
  - @<b>{16バイト・スタックアライメント制約}: SP（スタックポインタ）は常に 16 バイト境界にアライメントされなければならず、パディングによって消費が増大します。
  - @<b>{Mbed TLS の暗号計算フレーム}: AES-GCM や RSA、楕円曲線（ECDHE）の計算関数は、ローカル変数として大きな多倍長整数（Bignum）やコンテキスト構造体をスタック上にアロケートします。
 * この計算に基づき、ソケットディスパッチャ（@<tt>{service_thread}）や TCP ワークキューに @<b>{8192 バイト}、対話シェル（@<tt>{shell_uart}）に @<b>{16384 バイト} を割り当てることで、後述する第5章のスタック破壊を未然に防ぎます。

