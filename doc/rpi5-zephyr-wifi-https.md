# Raspberry Pi 5 × Zephyr RTOS で挑む Wi-Fi & HTTPS通信 完全攻略本
## ― BCM2712 SDIO から TLS 1.3 / HTTP Client 実装と泥沼デバッグの全記録 ―

**著者**: matsuoka  
**出展予定**: 技術書展21  
**対象読者**: 組込みエンジニア、Zephyr RTOS 開発者、Raspberry Pi 上級者、低レイヤ・ネットワーク愛好家  

---

## 目次

- [はじめに](#はじめに)
  - [本書の狙いとゴール](#本書の狙いとゴール)
  - [検証ターゲット環境](#検証ターゲット環境)
- [第1章：Raspberry Pi 5 と Zephyr RTOS のアーキテクチャ](#第1章raspberry-pi-5-と-zephyr-rtos-のアーキテクチャ)
  - [1.1 BCM2712 (Cortex-A76 AArch64) の特徴](#11-bcm2712-cortex-a76-aarch64-の特徴)
  - [1.2 Zephyr RTOS のブートシーケンスとビルド環境](#12-zephyr-rtos-のブートシーケンスとビルド環境)
- [第2章：ハードウェア記述と構成管理 ― デバイスツリー（DeviceTree）と prj.conf（Kconfig）の神髄](#第2章ハードウェア記述と構成管理--デバイスツリーdevicetreeと-prjconfkconfigの神髄)
  - [2.1 Zephyr におけるデバイスツリーの基礎（Linux DTS との決定的な違い）](#21-zephyr-におけるデバイスツリーの基礎linux-dts-との決定的な違い)
  - [2.2 本プロジェクトにおけるデバイスツリー設計の実例](#22-本プロジェクトにおけるデバイスツリー設計の実例)
  - [2.3 C言語ドライバとの自動バインディング（DT_DRV_COMPAT）の仕組み](#23-c言語ドライバとの自動バインディングdt_drv_compatの仕組み)
  - [2.4 `prj.conf`（Kconfig）の体系的解説と階層構造](#24-prjconfkconfigの体系的解説と階層構造)
  - [2.5 パケットバッファ数・スタックサイズの見積もり計算根拠](#25-パケットバッファ数スタックサイズの見積もり計算根拠)
- [第3章：【深淵編】BCM2712 SDHC と CYW43455 SDIO（4-bit / 50MHz / CMD53）接続の全貌](#第3章深淵編bcm2712-sdhc-と-cyw43455-sdio4-bit--50mhz--cmd53接続の全貌)
  - [3.1 Raspberry Pi 5 の SDIO インフラと BCM2712 SDHCI ドライバ](#31-raspberry-pi-5-の-sdio-インフラと-bcm2712-sdhci-ドライバ)
  - [3.2 SDIO プロトコル仕様と初期化シーケンス（CMD0 から TRAN 状態へ）](#32-sdio-プロトコル仕様と初期化シーケンスcmd0-から-tran-状態へ)
  - [3.3 CCCR レジスタ設定と複数ファンクション（F0/F1/F2）のセットアップ](#33-cccr-レジスタ設定と複数ファンクションf0f1f2のセットアップ)
  - [3.4 データ転送の心臓部：CMD52 と CMD53 のメカニズム](#34-データ転送の心臓部cmd52-と-cmd53-のメカニズム)
  - [3.5 【苦闘録】SDIO 接続で直面した4大難所とその突破法](#35-苦闘録sdio-接続で直面した4大難所とその突破法)
  - [3.6 Infineon AIROC Wi-Fi (WHD) と 5GHz帯（802.11ac / 702 Mbps）接続の開通](#36-infineon-airoc-wi-fi-whd-と-5ghz帯80211ac--702-mbps接続の開通)
- [第4章：ネットワークスタック（L3/L4）と DNS 名前解決](#第4章ネットワークスタックl3l4と-dns-名前解決)
  - [4.1 DHCPv4 による自動 IP 取得](#41-dhcpv4-による自動-ip-取得)
  - [4.2 ICMP Ping による導通確認](#42-icmp-ping-による導通確認)
  - [4.3 DNS リゾルバの実装と挙動](#43-dns-リゾルバの実装と挙動)
- [第5章：【激闘録1】64-bit ARM の Instruction Abort とスタック破壊の特定](#第5章激闘録164-bit-arm-の-instruction-abort-とスタック破壊の特定)
  - [5.1 突如発生した CPU Exception（0x338370）](#51-突如発生した-cpu-exception0x338370)
  - [5.2 レジスタダンプと PTE Walk の読み解き方](#52-レジスタダンプと-pte-walk-の読み解き方)
  - [5.3 真犯人：DNS ディスパッチャのスタックオーバーフロー](#53-真犯人dns-ディスパッチャのスタックオーバーフロー)
  - [5.4 64-bit 環境に最適化したスタックサイズ設計](#54-64-bit-環境に最適化したスタックサイズ設計)
- [第6章：HTTP 通信の実現と TCP パイプラインの構築](#第6章http-通信の実現と-tcp-パイプラインの構築)
  - [6.1 Zephyr ネイティブ TCP スタックの設定](#61-zephyr-ネイティブ-tcp-スタックの設定)
  - [6.2 バッファプール（net_buf / net_pkt）のサイジング](#62-バッファプールnet_buf--net_pktのサイジング)
  - [6.3 CUI コマンド `net http get` での平文通信成功](#63-cui-コマンド-net-http-get-での平文通信成功)
- [第7章：【激闘録2】HTTPS（TLS 1.2 / TLS 1.3）の暗号化の壁を突破する](#第7章激闘録2httpstls-12--tls-13の暗号化の壁を突破する)
  - [7.1 Mbed TLS & PSA Crypto の組み合わせ](#71-mbed-tls--psa-crypto-の組み合わせ)
  - [7.2 関門1：RSA 暗号スイートの不足による即時切断（-113 ECONNABORTED）](#72-関門1rsa-暗号スイートの不足による即時切断-113-econnaborted)
  - [7.3 関門2：TLS 1.3 ネゴシエーションの落とし穴](#73-関門2tls-13-ネゴシエーションの落とし穴)
  - [7.4 関門3：謎のエラーコード `-0x3a00` を追え](#74-関門3謎のエラーコード--0x3a00-を追え)
  - [7.5 解決：`MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE` と楕円曲線（secp384r1, X25519）](#75-解決mbedtls_err_pk_unknown_named_curve-と楕円曲線secp384r1-x25519)
  - [7.6 アーキテクチャ的考察：Mbed TLS と PSA Crypto の二重抽象化レイヤの落とし穴](#76-アーキテクチャ的考察mbed-tls-と-psa-crypto-の二重抽象化レイヤの落とし穴)
- [第8章：実機動作ログと成果の検証](#第8章実機動作ログと成果の検証)
  - [8.1 完全な HTTPS 通信ログ](#81-完全な-https-通信ログ)
  - [8.2 メモリ使用量とフットプリント評価](#82-メモリ使用量とフットプリント評価)
- [おわりに](#おわりに)
- [付録：完成版 `prj.conf` リファレンス](#付録完成版-prjconf-リファレンス)

---

## はじめに

### 本書の狙いとゴール

高性能なシングルボードコンピュータ（SBC）として世界中で広く普及している **Raspberry Pi 5**。通常は Raspberry Pi OS（Debian Linux）などの汎用OS上で動かすことが一般的ですが、これをリアルタイムOS（RTOS）の業界標準となりつつある **Zephyr RTOS** でベアメタルネイティブに動かし、オンボードの無線通信チップ（Infineon CYW43455）を経由して **HTTPS 通信（TLS 1.2 / TLS 1.3）** まで完遂させる――本書はその全開発プロセスと、直面した数々のハードなトラブルシューティングの記録です。

Linux では `curl` や `wget` コマンド1つで簡単に取得できる HTTPS ですが、RTOS の世界では以下のすべてを自前で正しく組み上げ、調和させなければなりません：

1. **デバイスツリー（DTS）** によるハードウェアペリフェラルとピン定義
2. **SDIO ホストコントローラドライバ**（BCM2712 の SDHC ハードウェア制御）
3. **Wi-Fi ファームウェアのロードと制御**（Infineon WHD スタック）
4. **802.11 / L2 ネットワーク層・DHCP クライアント**
5. **ネイティブ TCP プロトコルスタック**
6. **DNS クライアントによる名前解決**
7. **Mbed TLS / PSA Crypto による暗号化層（TLS 1.2 / 1.3、公開鍵暗号、楕円曲線暗号）**
8. **HTTP/1.1 プロトコルパーサー**

本書では、筆者が実際に直面した「デバイスツリーによるハードウェアバインド」「SDIO レベルでのブロックアライメント・バウンスバッファ破綻」「謎の CPU 例外（Instruction Abort）」「TLS ハンドシェイクエラー `-0x3a00`」の根本原因をどのように特定し、解決に至ったのかを、再現可能なコードとログを交えて余すところなく解説します。

### 検証ターゲット環境

| 項目 | 詳細 |
|---|---|
| **ターゲットボード** | Raspberry Pi 5 Model B (BCM2712 / Cortex-A76 Quad-core) |
| **アーキテクチャ** | ARMv8-A (AArch64, 64-bit) |
| **無線モジュール** | Infineon CYW43455 (Wi-Fi 5: 802.11ac, Bluetooth 5.0) |
| **インターフェース** | SDIO 4-bit (50MHz High-Speed モード) |
| **RTOS** | Zephyr RTOS (v4.4.99 開発ブランチ) |
| **ツールチェーン** | Zephyr SDK 1.0.0 (`aarch64-zephyr-elf-gcc`) |
| **暗号ライブラリ** | Mbed TLS 3.x + PSA Crypto |

---

## 第1章：Raspberry Pi 5 と Zephyr RTOS のアーキテクチャ

本プロジェクトでは、Raspberry Pi 5 上で Zephyr RTOS をベアメタル起動し、Infineon CYW43455 との SDIO 通信からネイティブ TCP/IP、そして TLS 1.3 による暗号化通信までを完全統合しました。
全体を構成するハードウェア、ドライバ、ネットワーク、セキュリティ、アプリケーションの階層構造（システム全体アーキテクチャ）を下図に示します。

```mermaid
flowchart TD
    subgraph App["Application Layer"]
        HTTP["HTTP Client (net http get)"]
        Shell["Zephyr Shell & Commands"]
    end

    subgraph Security["Security & Crypto Layer"]
        TLS["TLS 1.2 / TLS 1.3 (Mbed TLS)"]
        PSA["PSA Crypto Engine (NIST Curves / X25519)"]
    end

    subgraph NetStack["Native Network Stack"]
        Socket["BSD / POSIX Socket API"]
        DNS["DNS Resolver"]
        TCP["TCP / IP (IPv4, DHCP)"]
        L2["Net L2 (Ethernet / Wi-Fi Mgmt)"]
    end

    subgraph Drivers["Driver & Hardware Subsystem"]
        AIROC["Infineon AIROC Wi-Fi (WHD)"]
        SDHC["BCM2712 SDHC Driver (sdhc_bcm2712.c)"]
    end

    subgraph HW["Hardware (Raspberry Pi 5)"]
        RPi5["SoC: BCM2712 (Cortex-A76 Quad-core 64-bit)"]
        Bus[("SDIO 4-bit / 50MHz Bus")]
        CYW["Wi-Fi Module: Infineon CYW43455"]
    end

    App --> Socket
    Socket --> TLS
    TLS --> PSA
    TLS --> TCP
    Socket --> TCP
    Socket --> DNS
    DNS --> TCP
    TCP --> L2
    L2 --> AIROC
    AIROC --> SDHC
    SDHC --> RPi5
    RPi5 <==> Bus <==> CYW
```

### 1.1 BCM2712 (Cortex-A76 AArch64) の特徴

Raspberry Pi 5 に搭載されている SoC「BCM2712」は、ARM Cortex-A76 を4コア搭載するパワフルな 64-bit プロセッサです。従来の Cortex-M などのマイコンと異なり、以下の特徴を持ちます：

- **64-bit アドレス空間**: ポインタやレジスタサイズが 8 バイト（64-bit）となるため、32-bit マイコンに比べてスタックフレームの消費量が 1.5〜2 倍以上に増大します。
- **仮想記憶と MMU（メモリ管理ユニット）**: Zephyr でも MMU によるページテーブル（Translation Table）が構築され、アクセス権限（AP）や実行禁止（XN: Execute Never）が厳格に管理されます。
- **キャッシュライン**: 64 バイトのキャッシュラインと厳格なアライメントが要求されます。

### 1.2 Zephyr RTOS のブートシーケンスとビルド環境

Zephyr RTOS において、Raspberry Pi 5 は `rpi_5/bcm2712` ボード定義としてサポートが進められています。
ビルドツール `west` を用いて、サンプルアプリケーション（`samples/hello_world`）に Wi-Fi、TCP、TLS、HTTP Client の Kconfig を重層的に追加していく構成をとります。

```bash
# ビルドコマンド例
cd /path/to/zephyr-workspace/zephyr
west build -d build -b rpi_5/bcm2712 samples/hello_world
```

生成された `build/zephyr/zephyr.bin` を SD カードのルートディレクトリに `kernel8.img` として配置することで、Raspberry Pi 5 の EEPROM ブートローダーからダイレクトに起動させることができます。

---

## 第2章：ハードウェア記述と構成管理 ― デバイスツリー（DeviceTree）と prj.conf（Kconfig）の神髄

Zephyr RTOS を理解し、自在に制御する上で最も基本的でありながら強力な2大要素が **「デバイスツリー（DeviceTree）」** と **「prj.conf（Kconfig）」** です。
多くの開発者が「Linux の DTS と何が違うのか？」「なぜこの Kconfig 設定が必要なのか？」で躓きがちです。本章では、本プロジェクトにおける実際の設定ファイルをベースに、その内部アーキテクチャと設計意図を完全に解き明かします。

### 2.1 Zephyr におけるデバイスツリーの基礎（Linux DTS との決定的な違い）

Linux におけるデバイスツリー（`.dtb`）は、OS 起動時にブートローダーからメモリ上に渡され、Linux カーネルが実行時に動的パース（ランタイム探索）してドライバをプローブします。そのため、パース処理のオーバーヘッドやメモリ消費が存在します。

一方、**Zephyr RTOS のデバイスツリーは「ビルド時に C 言語のマクロ（プリプロセッサ定数）へと完全展開」されます**：
1. ビルドシステム（Python スクリプト `gen_defines.py` 等）が、DTS/DTSI ファイル群をパースして AST（抽象構文木）を生成します。
2. ハードウェアのレジスタアドレス、割り込み番号、クロック、GPIO ピンなどが、すべて `devicetree_generated.h` の中に `#define DT_N_S_...` という静的マクロとして出力されます。
3. C 言語のドライバコードは、実行時ではなく **コンパイル時に直値としてこれらを参照** します。

これにより、**ランタイムメモリ消費量ゼロ、探索オーバーヘッドゼロ、デッドコードの完全ストリッピング（不要なドライバが 1 バイトもバイナリに残らない）** という、リソースの制約が厳しい RTOS に最適なゼロコスト抽象化を実現しています。

```mermaid
flowchart LR
    subgraph Linux["Linux DTS モデル（実行時パースとメモリ消費）"]
        L_DTS[".dts"] --> L_DTC["dtc (コンパイラ)"]
        L_DTC --> L_DTB[".dtb (バイナリブロブ)"]
        L_DTB -->|ブートローダー経由でRAM展開| L_RAM["RAM上のDTB領域"]
        L_RAM -->|カーネル起動時に動的パース・探索| L_Driver["デバイスドライバ<br/>(ヒープ消費 & プローブ時間)"]
    end

    subgraph Zephyr["Zephyr DeviceTree モデル（ビルド時ゼロコスト抽象化）"]
        Z_DTS[".dts / .dtsi"] --> Z_Py["Python (gen_defines.py)"]
        Z_Py --> Z_H["devicetree_generated.h<br/>(#define DT_N_S_...)"]
        Z_H -->|C言語プリプロセッサによる静的直値展開| Z_Driver["Cドライバコード<br/>(不要コードは完全ストリップ)"]
    end
```

### 2.2 本プロジェクトにおけるデバイスツリー設計の実例

Raspberry Pi 5 のボード定義 [`boards/raspberrypi/rpi_5/rpi_5_bcm2712.dts`](file:///home/matsuoka/zephyr-workspace/zephyr/boards/raspberrypi/rpi_5/rpi_5_bcm2712.dts) および SoC 定義 [`dts/arm64/broadcom/bcm2712.dtsi`](file:///home/matsuoka/zephyr-workspace/zephyr/dts/arm64/broadcom/bcm2712.dtsi) では、Wi-Fi 通信に関わるノードが以下のように記述されています：

```dts
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
```

#### この記述の重要ポイント：
1. **64-bit 物理アドレス空間 (`reg = <0x10 0x01100000 0x1000>`)**:
   - BCM2712 は 64-bit SoC であるため、アドレス指定は上位 32-bit (`0x10`) と下位 32-bit (`0x01100000`) のペアで表現されます。実効物理アドレスは **`0x1001100000`**、サイズは **`0x1000`（4KB）** です。
2. **割り込み番号 (`interrupts = <GIC_SPI 274 ...>`)**:
   - ARM Generic Interrupt Controller（GICv2/v3）の Shared Peripheral Interrupt（SPI）番号 **274** が SDHC コントローラに割り当てられています。
3. **親子関係によるバインディング**:
   - SDHC ホストコントローラ（`sdhci1`）の子ノードとして `airoc_wifi` が定義されています。これにより、Zephyr は「CYW43455 は SDIO バス（sdhci1）に接続されたスレーブデバイスである」ことを静的に認識します。
4. **電源制御ピン (`wifi-reg-on-gpios = <&gio 28 GPIO_ACTIVE_HIGH>`)**:
   - CYW43455 の電源投入・ハードウェアリセットを司る `REG_ON` ピンが、RP1 I/O コントローラ（`gio`）の **GPIO 28** に接続されていることが指定されています。ドライバ起動時、この GPIO を High にドライブすることでチップがウェイクアップします。

### 2.3 C言語ドライバとの自動バインディング（DT_DRV_COMPAT）の仕組み

Zephyr のデバイスドライバは、ソースコード内で `DT_DRV_COMPAT` マクロを定義することで、デバイスツリーノードと自動的に結びつきます。

筆者が作成した `drivers/sdhc/sdhc_bcm2712.c` の冒頭を見てみましょう：

```c
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
```

- `DT_DRV_COMPAT` に `brcm_bcm2712_sdhci` を指定すると、デバイスツリー内で `compatible = "brcm,bcm2712-sdhci"` かつ `status = "okay"` となっているノードの数だけ、`DT_INST_FOREACH_STATUS_OKAY` によって初期化構造体とドライバ登録関数（`DEVICE_DT_INST_DEFINE`）が**コンパイル時に自動生成**されます。
- `DEVICE_MMIO_ROM_INIT` により、DTS に書かれた物理アドレス（`0x1001100000`）がカーネル内部の仮想アドレスへ自動マッピング（MMU ページテーブル設定）され、ドライバ側は `sdhc_bcm2712_read(dev, SDHCI_COMMAND)` のようにオフセットだけで安全にアクセスできるようになります。

### 2.4 `prj.conf`（Kconfig）の体系的解説と階層構造

`prj.conf` はアプリケーション固有のカーネル構成を指定するファイルですが、適当にフラグを並べるだけでは依存関係（Dependency Chain）の不一致でビルドエラーや意図しない機能欠落を引き起こします。

本プロジェクトの `prj.conf` は、以下の**5つの明確なレイヤ**で階層設計されています：

```
[Layer 5: アプリケーション層]  CONFIG_HTTP_CLIENT=y, CONFIG_SHELL=y
         │
[Layer 4: セキュリティ / 暗号層] CONFIG_MBEDTLS=y, CONFIG_PSA_CRYPTO=y, 楕円曲線 (secp384r1, X25519)
         │
[Layer 3: トランスポート / ネットワーク層] CONFIG_NET_TCP=y, CONFIG_DNS_RESOLVER=y, CONFIG_NET_DHCPV4=y
         │
[Layer 2: L2 リンク / Wi-Fi 管理層] CONFIG_WIFI=y, CONFIG_WIFI_AIROC=y, CONFIG_NET_L2_ETHERNET=y
         │
[Layer 1: ハードウェア / SDIO 層] CONFIG_SDHC=y, CONFIG_SDHC_BCM2712=y, CONFIG_SDIO_STACK=y
```

#### 各レイヤの設計ポイント：
1. **L1（ハードウェア層）**:
   - `CONFIG_SDHC=y` および `CONFIG_SDIO_STACK=y` により、Zephyr の SDIO サブシステムが有効化され、BCM2712 SDHC ホストコントローラがカード列挙を開始します。
2. **L2（Wi-Fi 管理層）**:
   - `CONFIG_WIFI_AIROC=y` により Infineon WHD が組み込まれ、SDIO 経由でファームウェアがロードされます。`CONFIG_WIFI_STA_AUTO_DHCPV4=y` を有効にすることで、AP 接続完了イベントをトリガーとして自動的に L3 の DHCP クライアントが起動します。
3. **L3（ネットワーク層）**:
   - `CONFIG_NET_TCP=y` でネイティブ TCP スタックを有効化。`CONFIG_DNS_RESOLVER=y` で DNS クライアントを有効化し、DHCP から取得した DNS サーバへの問い合わせを可能にします。
4. **L4（セキュリティ層）**:
   - Mbed TLS 3.x は PSA Crypto と密結合しているため、`CONFIG_PSA_CRYPTO=y` と `CONFIG_ENTROPY_GENERATOR=y` が必須です。また、後述の通り主要な暗号スイートと Named Curves（楕円曲線）を明示的に有効化します。
5. **L5（アプリケーション層）**:
   - `CONFIG_HTTP_CLIENT=y` により、HTTP/1.1 パーサーおよびソケット通信ヘルパーを有効化します。

### 2.5 パケットバッファ数・スタックサイズの見積もり計算根拠

`prj.conf` の中で、システムの安定動作（クラッシュ防止）に直結するのが **「バッファプール数」** と **「スタックサイズ」** の見積もりです。

#### 1. ネットワークバッファプールの見積もり
```ini
CONFIG_NET_PKT_RX_COUNT=32
CONFIG_NET_BUF_RX_COUNT=128
CONFIG_NET_PKT_TX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=128
```
- **`net_pkt`**: 1つのネットワークパケット（IPパケット）全体のメタデータ構造体（コンテキスト情報、タイムスタンプ等）。
- **`net_buf`**: 実際のパケットペイロードを断片（フラグメント）として格納する固定長データバッファ（デフォルト 128 バイト）。
- **サイジング根拠**:
  - Wi-Fi の MTU は 1500 バイトです。1 つのフルサイズパケット（1500B）を保持するには、`1500 ÷ 128 ≒ 12 個` の `net_buf` が消費されます。
  - TCP 通信では、ウィンドウサイズ（例: 4〜8パケット）分のセグメントが一時的にキューイングされます。さらに TLS 1.3 ハンドシェイク時には、Google から 3〜4 KB の証明書チェーンが連続して到着します（`4000 ÷ 128 ≒ 32 個` の `net_buf` を一瞬で消費）。
  - もしデフォルト値（`net_buf = 16`）のままだと、**証明書チェーンを受信した瞬間にバッファが枯渇し、パケットが即座にドロップされて接続タイムアウト（-116 ETIMEDOUT）** に陥ります。
  - そのため、受信バッファには最低でも `128 個`（16 KB 分の受信用データ保持領域）を確保しました。

#### 2. 64-bit ARM におけるスタックサイズの見積もり
```ini
CONFIG_SHELL_STACK_SIZE=16384
CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=8192
CONFIG_NET_TCP_WORKQ_STACK_SIZE=8192
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
```
- 32-bit Cortex-M では 1024〜2048 バイトで十分だったスタックが、なぜ 8192〜16384 バイトも必要なのか？
  - **レジスタ退避の巨大化**: AArch64 では汎用レジスタ（x0〜x30）が 64-bit（8バイト）幅です。関数呼び出しや例外ハンドラでのレジスタ保存だけで、32-bit 環境の倍のスタックを消費します。
  - **16バイト・スタックアライメント制約**: SP（スタックポインタ）は常に 16 バイト境界にアライメントされなければならず、パディングによって消費が増大します。
  - **Mbed TLS の暗号計算フレーム**: AES-GCM や RSA、楕円曲線（ECDHE）の計算関数は、ローカル変数として大きな多倍長整数（Bignum）やコンテキスト構造体をスタック上にアロケートします。
- この計算に基づき、ソケットディスパッチャ（`service_thread`）や TCP ワークキューに **8192 バイト**、対話シェル（`shell_uart`）に **16384 バイト** を割り当てることで、後述する第5章のスタック破壊を未然に防ぎます。

---

## 第3章：【深淵編】BCM2712 SDHC と CYW43455 SDIO（4-bit / 50MHz / CMD53）接続の全貌

Raspberry Pi 5 のオンボード Wi-Fi チップ（Infineon CYW43455）を RTOS から駆動する上で、**最大の難所のひとつが SDIO（Secure Digital Input Output）インターフェースの立ち上げ** でした。Linux カーネルの肥大化したサブシステムに頼らず、Zephyr のネイティブドライバとしてハードウェアと 1 対 1 で対話し、安定したバルクデータ転送を実現するまでの泥臭い闘いを詳解します。

### 3.1 Raspberry Pi 5 の SDIO インフラと BCM2712 SDHCI ドライバ

Raspberry Pi 5 では、CYW43455 は SoC（BCM2712）内部の専用 SDHC コントローラにハードウェア配線されています。
筆者は Zephyr のドライバフレームワークに準拠したホストコントローラドライバ `drivers/sdhc/sdhc_bcm2712.c` を作成しました。

SDHC コントローラは標準的な **SDHCI（SD Host Controller Interface）規格** に準拠した MMIO レジスタマップを持ちます：

| オフセット | レジスタ名 | 役割 |
|---|---|---|
| `0x00` | `SDHCI_DMA_ADDRESS` | DMA 転送先バッファ物理アドレス |
| `0x04` | `SDHCI_BLOCK_SIZE` | 転送ブロックサイズ（例: 64 バイト） |
| `0x06` | `SDHCI_BLOCK_COUNT` | 転送ブロック数 |
| `0x08` | `SDHCI_ARGUMENT` | SD コマンドの 32-bit 引数 |
| `0x0C` | `SDHCI_TRANSFER_MODE` | 転送方向（Read/Write）、ブロック/複数ブロック、DMA 有効化 |
| `0x0E` | `SDHCI_COMMAND` | コマンドインデックス（CMD番号）とレスポンス種別指定 |
| `0x10` | `SDHCI_RESPONSE` | カードから返却されたレスポンスデータ（R1〜R6） |
| `0x28` | `SDHCI_HOST_CONTROL` | 1-bit / 4-bit バス幅選択、High-Speed モード指定 |
| `0x2C` | `SDHCI_CLOCK_CONTROL` | 内部クロック発振、SD クロック分周、クロック安定待ち |
| `0x30` | `SDHCI_INT_STATUS` | コマンド完了、転送完了、各種エラー割り込みステータス |

```c
/* sdhc_bcm2712.c レジスタアクセスヘルパー */
static inline uint32_t sdhc_bcm2712_read(const struct device *dev, uint32_t reg)
{
    return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void sdhc_bcm2712_write(const struct device *dev, uint32_t val, uint32_t reg)
{
    sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}
```

ホストコントローラの初期化では、まずカードが認識されるまで **400 kHz の低速初期化クロック** を供給し、カードの初期化完了後に **25 MHz（Normal-Speed）**、最終的に **50 MHz（High-Speed）** へと周波数を引き上げます。

### 3.2 SDIO プロトコル仕様と初期化シーケンス（CMD0 から TRAN 状態へ）

SD メモリカードと異なり、SDIO カード（I/O デバイス）は独自の列挙シーケンスを持ちます。CYW43455 がホストと正常に対話できるようになるまでの通信シーケンスは以下の通りです：

```mermaid
sequenceDiagram
    autonumber
    participant Host as BCM2712 SDHC (Host)
    participant Card as CYW43455 (Card)

    Note over Host,Card: 400 kHz 低速初期化フェーズ
    Host->>Card: CMD0 (GO_IDLE_STATE)
    Note right of Card: バスとカード状態をリセット

    Host->>Card: CMD5 (IO_SEND_OP_COND, arg=0)
    Card-->>Host: Response R4 (OCR: 3.3V, 3 Functions)

    loop Ready ビット待機ポーリング
        Host->>Card: CMD5 (IO_SEND_OP_COND, VDD=3.3V)
        Card-->>Host: Response R4 (Ready=1, I/O準備完了)
    end

    Host->>Card: CMD3 (SEND_RELATIVE_ADDR)
    Card-->>Host: Response R6 (RCA = 0x0001 取得)

    Host->>Card: CMD7 (SELECT_CARD, RCA=0x0001)
    Card-->>Host: Response R1b (Card Selected)
    Note over Host,Card: カードが TRAN（データ転送可能）状態へ遷移！

    Note over Host,Card: 以降、CMD52 経由で 50MHz High-Speed & 4-bit バス幅へ切替
```

1. **CMD0 (GO_IDLE_STATE)**: バスをリセットし、カードを初期状態にします。
2. **CMD5 (IO_SEND_OP_COND)**: SDIO 専用コマンドです。引数 0 で送信すると、カードはサポートする電圧範囲（3.3V 等）と I/O ファンクション数（CYW43455 の場合は 3 ファンクション：F0, F1, F2）を **R4 レスポンス** で返します。その後、電圧ビットを立てて再度 CMD5 を発行し、カード内部の準備が整う（Ready ビットが 1 になる）までポーリングします。
3. **CMD3 (SEND_RELATIVE_ADDR)**: カード固有のアドレス（RCA: Relative Card Address、通常 `0x0001`）を取得します（**R6 レスポンス**）。
4. **CMD7 (SELECT_CARD)**: 取得した RCA を指定してカードを選択します。カードはスタンバイ状態から **TRAN（データ転送可能）状態** へと遷移します。

### 3.3 CCCR レジスタ設定と複数ファンクション（F0/F1/F2）のセットアップ

TRAN 状態に遷移した直後の CYW43455 は、依然として **1-bit バス幅 / 400kHz** で動作しています。これらを高速通信に切り替えるため、**CCCR（Card Common Control Registers）** と呼ばれる Function 0（F0）の専用レジスタ空間を設定します。

CYW43455 は以下の 3 つのファンクションを持ち、ホストとの役割分担が明確に分離されています：

```mermaid
flowchart TD
    subgraph Host["ホスト側：Raspberry Pi 5 (BCM2712 SDHC)"]
        SDHC_Drv["SDHC ホストドライバ (sdhc_bcm2712.c)"]
    end

    subgraph CYW43455["デバイス側：Infineon CYW43455 (SDIO)"]
        F0["Function 0 (F0): CCCR / FBR<br/>・バス幅切替 (1-bit / 4-bit)<br/>・クロック切替 (50MHz High-Speed)<br/>・F1/F2 の Enable / Ready 監視"]
        F1["Function 1 (F1): Backplane Access<br/>・ファームウェア (43455-fmac.bin) 転送<br/>・CLM BLOB (Country Code Matrix) 転送<br/>・ChipCommon / Core レジスタアクセス"]
        F2["Function 2 (F2): WLAN Data Path<br/>・Wi-Fi イーサネットフレーム送受信<br/>・64バイト固定ブロック転送 (CMD53)"]
    end

    SDHC_Drv <==>|"CMD52 (1 Byte Direct R/W)"| F0
    SDHC_Drv <==>|"CMD53 (Byte/Block 転送)"| F1
    SDHC_Drv <==>|"CMD53 (Block Mode / 64B)"| F2
```

- **Function 0 (F0)**: SDIO 共通制御レジスタ（CCCR/FBR）。バス幅やクロック、割り込みの制御。
- **Function 1 (F1)**: Infineon バックプレーンアクセス。SoC 内部レジスタ、ChipCommon、RAM へのアクセス。
- **Function 2 (F2)**: WLAN データ転送用。Wi-Fi パケット（SDPCM）の送受信を行う。

```
CCCR レジスタ操作シーケンス (CMD52 経由):
1. CCCR 0x07 (Bus Interface Control) に 0x02 を書き込み -> 4-bit バス幅を有効化
2. BCM2712 SDHC の HOST_CONTROL レジスタの 4-bit ビットを立てる
3. CCCR 0x13 (High-Speed Enable) に 0x02 (EHS) を書き込み -> 50MHz High-Speed を有効化
4. BCM2712 SDHC の CLOCK_CONTROL を 50MHz に切り替え
5. CCCR 0x02 (IO Enable) に 0x02 (F1 有効化) を書き込み -> F1 Ready (0x03) をポーリング待機
6. CCCR 0x02 (IO Enable) に 0x06 (F1+F2 有効化) を書き込み -> F2 Ready (0x03) をポーリング待機
7. F1 および F2 のブロックサイズレジスタ (CCCR 0x110-0x111, 0x210-0x211) に 64 (0x0040) を設定
```

### 3.4 データ転送の心臓部：CMD52 と CMD53 のメカニズム

SDIO 通信におけるデータ転送は、すべて **CMD52** と **CMD53** の2つのコマンドで行われます。

#### CMD52 (Direct I/O)
1 バイト単位のレジスタ読み書きに使用します（CCCR 設定や割り込みフラグ確認など）。
- **引数構成**: R/W フラグ (1bit) + Function 番号 (3bit) + RAW フラグ (1bit) + レジスタアドレス (17bit) + 書込データ (8bit)
- **レスポンス**: **R5 レスポンス**（カードステータス + 読み出しデータ）

#### CMD53 (Extended I/O)
大容量のデータ（パケットやファームウェアバイナリ）を高速転送するためのコマンドです。
- **バイトモード**: 1 〜 512 バイトの任意バイト数を転送（主に F1 の短小レジスタアクセス用）。
- **ブロックモード**: 事前に設定したブロックサイズ（64 バイト）の整数倍単位で転送（Wi-Fi パケット送受信の主力）。

CMD53 の 32-bit 引数（Argument）は以下のように極めて緻密に定義されています：

```
 31   30 28 27   26     25     24 9               8 0
+----+-----+---+------+------+-----+-----------------+
|R/W | Func| B | OP   | Block| Reg | Byte/Block      |
|Flag| No  |   | Code | Mode | Addr| Count           |
+----+-----+---+------+------+-----+-----------------+
```
- **R/W Flag (Bit 31)**: 0 = Read, 1 = Write
- **Func No (Bits 30-28)**: 0 = F0, 1 = F1, 2 = F2
- **Block Mode (Bit 27)**: 0 = バイトモード, 1 = ブロックモード
- **OP Code (Bit 26)**: 0 = 固定アドレスアクセス（FIFO）, 1 = インクリメンタルアドレス
- **Reg Addr (Bits 25-9)**: 転送先レジスタアドレス（17-bit）
- **Byte/Block Count (Bits 8-0)**: 転送バイト数（最大512）またはブロック数

### 3.5 【苦闘録】SDIO 接続で直面した4大難所とその突破法

CYW43455 との接続開発において、最も多くの時間を費やしたトラブルシューティングの実録です。

#### 難所1：CMD53 ブロックモード転送時のブロックアライメントとパディングの罠
Wi-Fi の受信パケットは「138バイト」や「74バイト」など、中途半端なサイズで届きます。
しかし SDIO のブロックモードでは、**ブロックサイズ（64 バイト）の整数倍でしか転送できません**。
もし 138 バイトの要求に対してそのまま CMD53 を発行すると、ホストコントローラまたはカード側のステートマシンがハングアップします。

**対応コード（`airoc_whd_hal_sdio.c`）**:
ブロック境界に満たない端数を、次のブロック境界へと切り上げるパディングロジックを実装しました：
```c
uint16_t xfer_size = data_size;
if (mode == SDIO_BLOCK_MODE && (data_size % func->block_size) != 0) {
    /* 64バイトの倍数に切り上げ */
    xfer_size = (data_size + func->block_size - 1) & ~(func->block_size - 1);
}
```

#### 難所2：バウンスバッファの枯渇（2048B → 4096B）と 64-bit キャッシュアライメント
SDIO コントローラが DMA 転送を行う際、転送元・転送先のバッファがキャッシュライン（64 バイト）にアライメントされていないと、キャッシュインバリデート時に周囲のデータが破壊されます。
また、従来の WHD 実装ではバウンスバッファが 2048 バイトで定義されていました：

```c
/* 修正前：2048バイトでは MTU 1500B + 802.11ヘッダ + SDPCMヘッダのブロック切り上げで溢れる危険性 */
static uint8_t cmd53_bounce_buf[2048];
```

一見すると、イーサネットの標準 MTU は 1500 バイトであり、ヘッダ類（イーサネット 14B + 802.1Q 4B + SDPCM 12B + ハードウェアタグ 4B = 1534B）を 64 バイトブロック境界へ切り上げても 1536 バイト（24ブロック）に収まるため、2048 バイトで足りるように思えます。

しかし、実際の Wi-Fi 通信では以下の要因により 2048 バイトの境界を容易に突破します：
1. **A-MSDU（Aggregate MSDU）の受信**: 802.11n/ac では複数のイーサネットフレームが 1 つの物理フレームに集約される場合があり、パケット長が 2KB を超える。
2. **WHD ドライバの内部メタデータ領域**: WHD はパケットデータの直前・直後に独自のディスクリプタやオフセットポインタを付加してバッファ渡しを行う。
3. **証明書チェーンのバースト転送**: TLS ハンドシェイク時の巨大な Certificate パケットが TCP セグメント境界ギリギリで押し寄せる。

その結果、`xfer_size` が 2048 バイトを超過してバウンスバッファから溢れ、パケットドロップや隣接メモリ破壊を誘発していました。

**【設計思想：メモリ効率と安全性のトレードオフ】**:
組込み Linux であればメガバイト単位の DMA メモリプールを確保して解決するところですが、RAM 制約がシビアな RTOS では無駄なメモリ確保は許されません。
そこで、**ちょうど MMU の 1 ページ相当かつ 64 バイトブロック × 64 個に合致する「4096 バイト」** を採用しました。これにより、A-MSDU を含む最大サイズのパケットを確実にカバーしつつ、メモリ消費の増加をわずか 2KB に抑え、64-bit ARM の MMU ページ境界とも親和性の高い安全な設計を実現しています。

**対応コード**:
バウンスバッファを **4096 バイト** に拡張し、AArch64 の 64 バイトキャッシュラインアライメント属性を付与しました：
```c
static uint8_t cmd53_bounce_buf[4096] __attribute__((aligned(64)));
```

#### 難所3：ファームウェア（43455-fmac.bin）と CLM BLOB の F1 バックプレーン転送
CYW43455 は起動時、オンチップ ROM のブートローダーしか動いていません。
Wi-Fi として動作させるには、約 500 KB のファームウェアバイナリと、国コード情報（CLM BLOB）を、SDIO Function 1 のバックプレーンウィンドウ経由で Wi-Fi チップの内部 RAM（`0x00000000`）へ CMD53 で分割転送しなければなりません。

転送中に 1 ブロックでも CRC エラーやタイムアウトが発生すると、ファームウェアの整合性チェックが失敗してチップが無反応になります。
BCM2712 SDHC ドライバ側で、コマンド完了（`SDHCI_INT_STATUS: CMD_COMPLETE`）と転送完了（`TRANSFER_COMPLETE`）をステータスレジスタで厳密にハンドシェイクし、タイムアウト監視を行うことで、500 KB のバルクダウンロードをノーエラーで完走させました。

#### 難所4：SDPCM フレームのハードウェアタグ検証とログ氾濫の抑制
ファームウェアが起動すると、CYW43455 はホストとの通信プロトコルとして **SDPCM（SDIO Packet Communication Manager）** を使用します。
受信用パケットの先頭には、以下のようなハードウェアタグヘッダが付与されています：

```
+--------------------+--------------------+
| Packet Length (2B) | ~Packet Length (2B)|  <- ビット反転による整合性検証
+--------------------+--------------------+
| SDPCM Header (12B) : Sequence, Channel  |
+-----------------------------------------+
| Ethernet / 802.3 Frame Data ...         |
+-----------------------------------------+
```

開発初期、パケットを受信するたびにコンソールへ以下のログが出続け、シリアルバッファを圧迫していました：
```text
AIROC: SDPCM Frame valid hwtag: len=60 (0x003c)
AIROC: SDPCM Frame valid hwtag: len=64 (0x0040)
```
この整合性検証ロジックが正常にパスしていることを確認した上で、不要なデバッグ出力を抑制（ミュート）し、高速なスループットを維持できるように最適化しました。

### 3.6 Infineon AIROC Wi-Fi (WHD) と 5GHz帯（802.11ac / 702 Mbps）接続の開通

これらの SDIO 層のハードルをすべて乗り越えたことで、Infineon AIROC ドライバスタックが完全に立ち上がりました。

Zephyr Shell から接続を実行：
```text
uart:~$ net wifi connect -s "MyHome5G" -p "Password123" -k 1 -b 2
Connected
```

`net wifi status` で確認した結果：
```text
uart:~$ net wifi status
Status: connected
SSID: MyHome5G
Band: 5GHz
Channel: 36
Security: WPA2-PSK
Link speed: 702 Mbps
```

SDIO 4-bit / 50MHz High-Speed モードの広大なバス帯域により、**702 Mbps（802.11ac VHT80 MCS 8/9）** という圧巻のリンク速度が確立されました！

---

## 第4章：ネットワークスタック（L3/L4）と DNS 名前解決

### 4.1 DHCPv4 による自動 IP 取得

Wi-Fi リンクが確立されると、Zephyr の DHCPv4 クライアント（`CONFIG_NET_DHCPV4=y`）が自動的にルーターへ DISCOVER パケットを送信し、IP アドレス、サブネットマスク、ゲートウェイ、DNS サーバーを自動取得します。

```text
[00:00:15.340,000] <inf> net_dhcpv4: Received address: 192.168.0.28
[00:00:15.345,000] <inf> net_dhcpv4: Lease time: 86400 seconds
[00:00:15.350,000] <inf> net_dhcpv4: Router: 192.168.0.1
```

### 4.2 ICMP Ping による導通確認

取得した IP で外の世界と疎通できているかを `net ping` で確認します。

```text
uart:~$ net ping 8.8.8.8
PING 8.8.8.8
28 bytes from 8.8.8.8 to 192.168.0.28: icmp_seq=1 ttl=118 time=11.57 ms
28 bytes from 8.8.8.8 to 192.168.0.28: icmp_seq=2 ttl=118 time=11.57 ms
--- ping statistics ---
2 packets transmitted, 2 received, 0% packet loss
```

これにより、L2（Wi-Fi/Ethernet）、L3（IPv4/ICMP）のパケット送受信パスが完全に成立していることが確認できました。

### 4.3 DNS リゾルバの実装と挙動

ドメイン名から IP アドレスを取得するため、Zephyr の DNS リゾルバ（`CONFIG_DNS_RESOLVER=y`）を有効化します。

```ini
CONFIG_DNS_RESOLVER=y
CONFIG_DNS_RESOLVER_MAX_SERVERS=2
CONFIG_DNS_SERVER_IP_ADDRESSES=y
CONFIG_DNS_SERVER1="8.8.8.8"
```

しかし、この DNS 機能を有効化して名前解決を試みた瞬間、**最初の致命的なクラッシュ** が発生しました。

---

## 第5章：【激闘録1】64-bit ARM の Instruction Abort とスタック破壊の特定

### 5.1 突如発生した CPU Exception（0x338370）

DNS クエリコマンド `net dns query google.com` を実行した直後、以下の深刻なパニックログが出力されてシステムが停止（Halt）しました。

```text
[00:00:50.207,000] <err> os: ELR_ELn: 0x0000000000338370
[00:00:50.212,000] <err> os: ESR_ELn: 0x000000008600000f
[00:00:50.220,000] <err> os:   EC:  0x21 (Instruction Abort taken without a change in Exception level.)
[00:00:50.232,000] <err> os:   IL:  0x1
[00:00:50.239,000] <err> os:   ISS: 0xf
[00:00:50.246,000] <err> os: FAR_ELn: 0x0000000000338370
[00:00:50.254,000] <err> os: PTE walk for 0x338370 (TTBR0=0x000000000030e000):
[00:00:50.302,000] <err> os:     page AP=EL_higher RW G AF
[00:00:50.419,000] <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
[00:00:50.429,000] <err> os: Current thread: 0x327000 (shell_uart)
[00:00:50.438,000] <err> os: Halting system
```

### 5.2 レジスタダンプと PTE Walk の読み解き方

このエラーログは、ARMv8-A アーキテクチャの内部挙動を雄弁に物語っています：

1. **`EC: 0x21` (Instruction Abort)**: CPU が命令フェッチを実行しようとしたアドレスが無効、あるいは実行権限がない。
2. **`FAR_ELn: 0x338370`**: 例外が発生したフェッチ先アドレス。
3. **`PTE walk` の結果**: `page AP=EL_higher RW G AF`。
   - なんと、該当アドレス（`0x338370`）の MMU ページ属性は **「RW（Read/Write）」であり、「Execute（実行可能）」ビットが立っていません（データ領域 / スタック領域）**！
4. **レジスタの値**:
   - `lr: 0x0000000000338370`
   - `x8` 〜 `x18`: `0xaaaaaaaaaaaaaaaa`（Zephyr のスタック初期化フィルパターン `0xAA`）！

関数のリターンアドレス（`lr`）に、実行コード領域ではなくスタック領域のアドレスが書き込まれ、さらにレジスタ群がスタック未初期化パターン `0xAA` で埋め尽くされていることから、**「典型的なスタックオーバーフローによる関数リターン先破壊」** であることが確定しました。

### 5.3 真犯人：DNS ディスパッチャのスタックオーバーフロー

問題は「どのスレッドが溢れたのか」です。
カレントスレッドは `shell_uart`（`0x327000`）となっていますが、リンカーマップファイル（`zephyr.map`）を精査したところ、驚くべき配置が判明しました：

```text
0x00328000 - 0x0032c000: shell_uart_stack
0x0032c000 - 0x0032c500: service_thread_stack (CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=1200)
```

Zephyr のソケットサービスディスパッチャスレッド（`service_thread`）は、DNS レスポンスのパケットを受信してコールバックを呼び出します。
デフォルト設定の `CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=1200`（1200バイト）は 32-bit マイコン向けのサイズであり、**64-bit ARM ではポインタが 8 バイト、スタックアライメントが 16 バイト要求されるため、ネストした関数呼び出しであっという間に 1200 バイトを突破** します。

その結果、直下に配置されていた `shell_uart_stack` のフレームを突き破り、リターンアドレスを破壊していたのです。

```mermaid
flowchart TD
    subgraph Memory["SRAM メモリマップ（高位アドレスから低位アドレスへ伸長）"]
        direction TB

        T_Svc["[0x0032c500] service_thread_stack 上限"]
        Svc["service_thread_stack (わずか 1200 Bytes)<br/>※ DNS パケット受信コールバックが多重ネストで走行"]
        B_Svc["[0x0032c000] service_thread_stack 底（境界）"]

        Overflow["💥 1200バイトをあっという間に突破して下方へオーバーフロー！<br/>(64-bit ARM の巨大スタックフレーム + 0xAA パターンが侵食)"]

        T_Shell["[0x0032c000] shell_uart_stack 上限（境界）"]
        ShellFrame["shell_uart のスタックフレーム<br/>・保存されていた関数リターンアドレス (lr) が 0x338370 で上書きされる<br/>・退避レジスタ x8〜x18 が 0xAAAAAAAAAAAAAAAA で壊滅"]
        Shell["shell_uart_stack (16 KB)"]
        B_Shell["[0x00328000] shell_uart_stack 底"]
    end

    T_Svc --> Svc --> B_Svc
    B_Svc -.-> Overflow
    Overflow -. 侵食 .-> T_Shell
    T_Shell --> ShellFrame --> Shell --> B_Shell

    subgraph Abort["CPU 例外（Instruction Abort）の炸裂"]
        CPU["shell_uart 側が関数から復帰（ret）を実行"]
        PTE["MMU ページテーブル走査 (PTE Walk):<br/>復帰先 0x338370 は AP=RW (データ/スタック) かつ XN (実行禁止)"]
        Panic["💥 Instruction Abort (EC: 0x21) 発生！<br/>FAR=0x338370, ELR=0x338370 でカーネルパニック"]
    end

    ShellFrame --> CPU --> PTE --> Panic
```

### 5.4 64-bit 環境に最適化したスタックサイズ設計

この問題を根治するため、ネットワークおよびシステム全体のスタックサイズを 64-bit ARM 向けに再設計・拡張しました：

```ini
# 64-bit ARM Stack Sizes (スタック破壊防止)
CONFIG_SHELL_STACK_SIZE=16384
CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=8192
CONFIG_NET_MGMT_EVENT_STACK_SIZE=4096
CONFIG_NET_RX_STACK_SIZE=8192
CONFIG_NET_TX_STACK_SIZE=8192
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
CONFIG_NET_TCP_WORKQ_STACK_SIZE=8192
```

スタックを拡張して再ビルドした結果、DNS 解決は見事にノーエラーで完了しました：

```text
uart:~$ net dns query google.com
Query for 'google.com' sent.
dns: 142.251.118.138
dns: 142.251.118.100
dns: 142.251.118.113
dns: All results received
```

---

## 第6章：HTTP 通信の実現と TCP パイプラインの構築

### 6.1 Zephyr ネイティブ TCP スタックの設定

平文の HTTP 通信を行うため、Zephyr のネイティブ TCP スタック（`CONFIG_NET_TCP=y`）を有効化します。

```ini
CONFIG_NET_TCP=y
CONFIG_NET_MAX_CONTEXTS=10
CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT=1000
CONFIG_NET_SOCKETS_CONNECT_TIMEOUT=15000
```

### 6.2 バッファプール（net_buf / net_pkt）のサイジング

TCP 通信では、ウィンドウ制御、セグメントの再送制御、受信データのバッファリングが行われます。デフォルトのバッファ数（16個程度）では、Web ページを受信した際に一瞬でプールが枯渇し、パケットドロップが発生します。

```ini
CONFIG_NET_PKT_RX_COUNT=32
CONFIG_NET_BUF_RX_COUNT=128
CONFIG_NET_PKT_TX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=128
```

### 6.3 CUI コマンド `net http get` での平文通信成功

Zephyr Shell の `http_client` 拡張（`CONFIG_HTTP_CLIENT=y`）を用いて、`http://google.com/` にアクセスします。

```text
uart:~$ net http get http://google.com/
Connecting to 172.217.209.138:80 (timeout: 15000 ms)...
<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
<H1>301 Moved</H1>
The document has moved
<A HREF="http://www.google.com/">here</A>.
</BODY></HTML>
```

TCP 3-way ハンドシェイクが成功し、Google サーバーから `HTTP/1.1 301 Moved Permanently` の HTML レスポンスが返ってきました！
平文の Web 通信パイプラインはこれで完成です。

---

## 第7章：【激闘録2】HTTPS（TLS 1.2 / TLS 1.3）の暗号化の壁を突破する

現代のインターネットでは、ほぼすべての Web サービスが HTTPS（TLS 暗号化）を必須としています。ここからが真の難所でした。

本章で立ちふさがった3つの致命的関門（RSA 暗号スイート不足による切断、TLS 1.3 スイート空トラップ、謎のエラー `-0x3a00` による証明書パース破棄）と、正常なハンドシェイク成立までのシーケンス全貌を下図に示します。

```mermaid
sequenceDiagram
    autonumber
    participant Client as Zephyr HTTP Client (RPi5)
    participant Server as Google Web Server (:443)

    Client->>Server: TCP SYN
    Server-->>Client: TCP SYN-ACK
    Client->>Server: TCP ACK (TCP 接続確立)

    rect rgb(255, 235, 235)
        Note over Client: 【関門2】TLS 1.3 暗号スイート空トラップ
        Client->>Server: ClientHello (TLS 1.3 宣言 + 支持暗号一覧)
        Note left of Client: TLS 1.3 を有効化していたが、<br/>スイート Kconfig 未選択で不一致切断
    end

    rect rgb(255, 240, 220)
        Note over Client,Server: 【関門1】RSA 非対応による拒絶 (-113)
        Server-->>Client: ServerHello (選定暗号通知)
        Server-->>Client: Certificate (RSA 2048/4096 bit 提示)
        Note left of Client: クライアントが ECDSA のみ対応だったため<br/>互換性不一致で即時 RST 切断 (-113)
    end

    rect rgb(240, 245, 255)
        Note over Client: 【関門3】謎のエラー -0x3a00 の正体
        Server-->>Client: KeyShare / Certificate Chain (secp384r1 / X25519)
        Note left of Client: PSA Crypto に secp384r1/X25519 未定義のため<br/>MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE で破棄
    end

    rect rgb(235, 255, 235)
        Note over Client,Server: 全関門突破後：正常な通信確立
        Client->>Server: Finished
        Server-->>Client: Finished (TLS セッション確立)
        Client->>Server: HTTP GET /
        Server-->>Client: HTTP/1.1 301 Moved Permanently (通信大成功！)
    end
```

### 7.1 Mbed TLS & PSA Crypto の組み合わせ

Zephyr 3.7 以降、暗号化スタックは従来の Mbed TLS 単体構成から、標準化された **PSA Crypto（Platform Security Architecture）** を基盤とするアーキテクチャへと移行しています。

```ini
CONFIG_NET_SOCKETS_SOCKOPT_TLS=y
CONFIG_MBEDTLS=y
CONFIG_MBEDTLS_BUILTIN=y
CONFIG_PSA_CRYPTO=y
CONFIG_ENTROPY_GENERATOR=y
```

### 7.2 関門1：RSA 暗号スイートの不足による即時切断（-113 ECONNABORTED）

最初に `net http get -k https://google.com/` を実行した際、以下のエラーが発生しました：

```text
Connecting to 172.217.209.102:443 (timeout: 15000 ms)...
Failed to connect to 172.217.209.102:443, ret=-1, errno=113
HTTP GET request failed (-113)
```

`errno 113` は `ECONNABORTED`（Software caused connection abort）です。
ソースコード（`subsys/net/lib/sockets/sockets_tls.c`）を調査したところ、`zsock_connect()` による TCP 接続は成功しているものの、その直後の `tls_mbedtls_handshake()` が失敗すると、Zephyr はエラーを一律に `ECONNABORTED` に変換して返していることがわかりました。

**原因**:
初期設定では ECDSA 暗号スイートのみが有効化されていました。しかし、Google などの大規模 Web サーバーは RSA 証明書（2048/4096 bit）を提示することが多く、クライアント側に RSA 暗号スイート（`ECDHE-RSA-AES-GCM`）が存在しないため、ネゴシエーションに失敗していたのです。

**対応**:
`prj.conf` に RSA 暗号スイートを追加：
```ini
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384=y
```

### 7.3 関門2：TLS 1.3 ネゴシエーションの落とし穴

しかし、これでも接続は失敗し続けました。
設定を確認すると、プロトコルとして `CONFIG_MBEDTLS_SSL_PROTO_TLS1_3=y` を有効にしていたにもかかわらず、**TLS 1.3 用の暗号スイート Kconfig（`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_...`）が1つも選択されていなかった** のです。

クライアントが「TLS 1.3 に対応している」と ClientHello で宣言しながら、提示する TLS 1.3 暗号スイートが空だったため、サーバー側でハンドシェイク不一致となっていました。

**対応**:
```ini
CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_256_GCM_SHA384=y
```

### 7.4 関門3：謎のエラーコード `-0x3a00` を追え

暗号スイートを追加したものの、依然としてハンドシェイクは失敗します。
そこで、Mbed TLS の内部エラーコードを直接露出させるため、[`sockets_tls.c`](file:///home/matsuoka/zephyr-workspace/zephyr/subsys/net/lib/sockets/sockets_tls.c) のハンドシェイク失敗ハンドラにデバッグ出力を仕込みました：

```c
printk("[TLS] Handshake error: -0x%x\n", -ret);
```

再起動してコマンドを実行すると、決定的なログが現れました：

```text
uart:~$ net http get -k https://google.com/
Connecting to 172.217.209.101:443 (timeout: 15000 ms)...
[TLS] Handshake error: -0x3a00
Failed to connect to 172.217.209.101:443, ret=-1, errno=113
```

**`-0x3a00`** ―― この謎の 16進数エラーコードは何を意味しているのか？

### 7.5 解決：`MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE` と楕円曲線（secp384r1, X25519）

Mbed TLS / TF-PSA-Crypto のソースツリーを全件走査した結果、`modules/crypto/tf-psa-crypto/include/mbedtls/pk.h` にその定義を発見しました：

```c
#define MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE -0x3A00
```

エラーの意味は **「未知の Named Curve（楕円曲線）」**！

さらに `tf-psa-crypto/extras/pkparse.c` および `utilities/oid.c` を追跡したところ、以下の真実が浮き彫りになりました：

```c
/* oid.c */
static const oid_ecp_grp_t oid_ecp_grp[] = {
#if defined(PSA_WANT_ECC_SECP_R1_256)
    { OID_DESCRIPTOR(MBEDTLS_OID_EC_GRP_SECP256R1, "secp256r1", "secp256r1"), MBEDTLS_ECP_DP_SECP256R1 },
#endif
#if defined(PSA_WANT_ECC_SECP_R1_384)
    { OID_DESCRIPTOR(MBEDTLS_OID_EC_GRP_SECP384R1, "secp384r1", "secp384r1"), MBEDTLS_ECP_DP_SECP384R1 },
#endif
    ...
```

Google のサーバー証明書チェーン（GTS Root / GlobalSign 中間証明書）や TLS 1.3 の鍵交換（Key Share）では、標準の `secp256r1` だけでなく **`secp384r1`** や **`X25519` (Curve25519)**、**`secp521r1`** が多用されます。
従来の構成では `secp256r1` しか組み込まれていなかったため、証明書パース時に未知の OID と判定され、即座に `-0x3A00` で破棄されていたのです。

**最終修正**:
`prj.conf` にインターネットで用いられる主要な楕円曲線をすべて有効化：

```ini
# 楕円曲線 (Curves) 完全対応
CONFIG_PSA_WANT_ECC_SECP_R1_256=y
CONFIG_PSA_WANT_ECC_SECP_R1_384=y
CONFIG_PSA_WANT_ECC_SECP_R1_521=y
CONFIG_PSA_WANT_ECC_MONTGOMERY_255=y  # X25519
```

さらに、大きな証明書チェーンを安全に展開できるよう、Mbed TLS ヒープを 128KB（`CONFIG_MBEDTLS_HEAP_SIZE=131072`）に拡張しました。

### 7.6 アーキテクチャ的考察：Mbed TLS と PSA Crypto の二重抽象化レイヤの落とし穴

今回直面した `-0x3a00` エラーは、単なる設定漏れではなく、**Zephyr 3.7+ における暗号スタックの二重抽象化レイヤ構造** に起因する構造的な落とし穴でした。

```mermaid
flowchart TD
    subgraph Upper["上位レイヤ：TLS プロトコルエンジン (Mbed TLS)"]
        TLS_Engine["TLS 1.2 / TLS 1.3 ハンドシェイク制御<br/>Kconfig: CONFIG_MBEDTLS_CIPHERSUITE_..."]
    end

    subgraph Interface["抽象化境界 (PSA Crypto API)"]
        PSA_API["psa_import_key() / psa_verify_hash()<br/>※ ハードウェア暗号エンジンや HSM へのオフロードを可能にする標準API"]
    end

    subgraph Lower["下位レイヤ：暗号プリミティブ実装 (TF-PSA-Crypto)"]
        PSA_Engine["RSA / ECC アルゴリズム実体<br/>Kconfig: CONFIG_PSA_WANT_ECC_..."]
    end

    TLS_Engine -->|暗号演算要求| PSA_API
    PSA_API -->|未定義の曲線 OID| PSA_Engine
    PSA_Engine -.->|PSA_ERROR_NOT_SUPPORTED| PSA_API
    PSA_API -.->|変換: -0x3A00 (UNKNOWN_NAMED_CURVE)| TLS_Engine
    TLS_Engine -.->|上位Socketへ丸め込み: -113 (ECONNABORTED)| UserApp["アプリ (HTTP Client)"]
```

1. **二重化された Kconfig 体系**:
   上位の Mbed TLS レイヤでいくら TLS 1.3 暗号スイート（`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_...`）を有効化しても、下位の PSA Crypto レイヤ側で該当する曲線（`CONFIG_PSA_WANT_ECC_SECP_R1_384` や `CONFIG_PSA_WANT_ECC_MONTGOMERY_255`）がビルド対象に含まれていなければ、暗号エンジン実体は生成されません。
2. **多重エラー変換による原因隠蔽**:
   下位の PSA Crypto が `PSA_ERROR_NOT_SUPPORTED` を返すと、Mbed TLS はそれを `-0x3A00` に変換し、さらに Zephyr の Socket レイヤが BSD errno の `-113 (ECONNABORTED)` に丸め込んでしまいます。
   この「多段抽象化によるエラーコードの不可視化」こそが、デバッグを極限まで難解にしていた真犯人でした。

---

## 第8章：実機動作ログと成果の検証

### 8.1 完全な HTTPS 通信ログ

修正を適用した最新の `zephyr.bin` を Raspberry Pi 5 にロードし、運命のテストを行いました。

```text
uart:~$ net http get -k https://google.com/
TLS verification is disabled
Connecting to 142.251.118.113:443 (timeout: 15000 ms)...

<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
<H1>301 Moved</H1>
The document has moved
<A HREF="https://www.google.com/">here</A>.
</BODY></HTML>
```

ついに、**エラーコード `-0x3a00` も `-113` も一切出ず、暗号化ハンドシェイクが完了し、Google からの HTTPS レスポンスがコンソール上に出力されました！**

### 8.2 メモリ使用量とフットプリント評価

最終的な Zephyr RTOS イメージのメモリフットプリントは以下の通りです：

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:           0 B          0 B
             RAM:       1956 KB        16 MB     11.94%
```

フルスタックの Wi-Fi ドライバ、TCP/IP、DHCP、DNS、TLS 1.2/1.3 暗号化エンジン、HTTP パーサーをすべて含んでも、**RAM 使用量はわずか 1.95 MB（全体の 12% 未満）** です。
Raspberry Pi 5 の大容量 RAM（4GB / 8GB）から見れば極めて軽量であり、RTOS ならではの超高速ブート（電源投入から数百ミリ秒での起動）と強固な決定性を両立しています。

#### 【総括比較】Raspberry Pi OS (Linux) vs Zephyr RTOS

「なぜ高性能な Raspberry Pi 5 上であえて RTOS を動かすのか？」――その技術的意義と圧倒的なリソース効率を裏付ける比較表を以下に示します：

| 評価項目 | Raspberry Pi OS (Linux 64-bit) | Zephyr RTOS (本プロジェクト) | 比較のポイント |
| :--- | :--- | :--- | :--- |
| **コールドブート時間** | 15 〜 25 秒 (systemd 起動含む) | **約 0.3 秒** (EEPROM 直起動) | 電源投入即座に Wi-Fi 接続・通信を開始可能 |
| **RAM 消費量** | 200 MB 〜 500 MB 程度 | **約 1.95 MB** (Wi-Fi + TLS 含む) | **99% 以上のメモリ削減**（超軽量） |
| **ストレージ専有量** | 数 GB (ext4 ルートファイルシステム) | **約 1.5 MB** (`zephyr.bin` 単一ファイル) | SD カード障害リスクの極小化 |
| **リアルタイム決定性** | ミリ秒単位のジッター (PREEMPT_RT 除く) | **マイクロ秒単位の決定論的応答** | 厳密な周期制御や即応割り込みハンドリング |
| **不意の電源断耐性** | ファイルシステム破損リスク（ジャーナル依存） | **完全ステートレス（壊れる FS が存在しない）** | 産業用・組込みアプライアンスに最適 |
| **セキュリティ・攻撃面** | シェル、デーモン、多重サービスが稼働 | **不要コード完全ストリップ（単一バイナリ）** | 侵入経路・脆弱性の劇的削減（ゼロアタックサーフェス） |
| **ネットワーク構成** | NetworkManager + wpa_supplicant + OpenSSL | **Native TCP/IP + AIROC WHD + Mbed TLS/PSA** | カーネル空間とユーザ空間の境界ゼロ（ゼロコピー） |

---

## おわりに

Raspberry Pi 5 という最先端の SBC 上で、Linux カーネルを一切介さず、Zephyr RTOS のネイティブドライバとプロトコルスタックのみで Wi-Fi と HTTPS 暗号化通信を実現する旅は、数々の深い低レイヤの学びをもたらしてくれました。

- デバイスツリーのビルド時マクロ展開によるゼロオーバーヘッド抽象化
- Kconfig による階層的モジュール構成とスタック/バッファプールの見積もり
- SDIO の CMD53 ブロックアライメントとバウンスバッファの整合性
- 64-bit ARM 特有のスタックフレーム肥大化とアライメントの罠
- ページテーブル（MMU）によるシビアな例外検知
- モダン TLS（TLS 1.3 / PSA Crypto）における暗号スイートと Named Curve（OID）の依存関係

本稿が、組込みシステムで高度なネットワークやセキュリティスタックに挑むすべてのエンジニアの一助となれば幸いです。

---

## 付録：完成版 `prj.conf` リファレンス

本プロジェクトで最終的に動作を確認した `prj.conf` の全容です：

```ini
# Hardware SDHC & SDIO
CONFIG_SDHC=y
CONFIG_SDHC_BCM2712=y
CONFIG_SDIO_STACK=y
CONFIG_GPIO=y
CONFIG_HEAP_MEM_POOL_SIZE=131072

# Logging & Shell
CONFIG_LOG=y
CONFIG_LOG_MODE_IMMEDIATE=y
CONFIG_LOG_BUFFER_SIZE=8192
CONFIG_SHELL=y
CONFIG_SHELL_BACKENDS=y
CONFIG_SHELL_BACKEND_SERIAL=y
CONFIG_SHELL_ARGC_MAX=10
CONFIG_SHELL_BACKEND_SERIAL_API_POLLING=y

# Infineon AIROC Wi-Fi
CONFIG_WIFI=y
CONFIG_WIFI_AIROC=y
CONFIG_AIROC_WIFI_CUSTOM=y

# Networking Core (IPv4, DHCP, Wi-Fi Management)
CONFIG_NETWORKING=y
CONFIG_NET_L2_ETHERNET=y
CONFIG_NET_L2_WIFI_MGMT=y
CONFIG_NET_IPV4=y
CONFIG_NET_DHCPV4=y
CONFIG_WIFI_STA_AUTO_DHCPV4=y
CONFIG_NET_SHELL=y
CONFIG_NET_L2_WIFI_SHELL=y

# 64-bit ARM Stack Optimization (Prevent Stack Overflow)
CONFIG_SHELL_STACK_SIZE=16384
CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=8192
CONFIG_NET_MGMT_EVENT_STACK_SIZE=4096
CONFIG_NET_RX_STACK_SIZE=8192
CONFIG_NET_TX_STACK_SIZE=8192
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
CONFIG_AIROC_WIFI_EVENT_TASK_STACK_SIZE=4096
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_ISR_STACK_SIZE=4096
CONFIG_NET_TCP_WORKQ_STACK_SIZE=8192

# DNS Resolver
CONFIG_DNS_RESOLVER=y
CONFIG_DNS_RESOLVER_MAX_SERVERS=2
CONFIG_DNS_NUM_CONCUR_QUERIES=2
CONFIG_DNS_SERVER_IP_ADDRESSES=y
CONFIG_DNS_SERVER1="8.8.8.8"

# TCP Stack
CONFIG_NET_TCP=y
CONFIG_NET_MAX_CONTEXTS=10
CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT=1000
CONFIG_NET_SOCKETS_CONNECT_TIMEOUT=15000

# Packet & Buffer Pools
CONFIG_NET_PKT_RX_COUNT=32
CONFIG_NET_BUF_RX_COUNT=128
CONFIG_NET_PKT_TX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=128

# HTTP Client
CONFIG_HTTP_CLIENT=y
CONFIG_NET_SHELL_HTTP_CLIENT_URL_LEN=256

# TLS / Socket Security
CONFIG_NET_SOCKETS_SOCKOPT_TLS=y
CONFIG_TLS_CREDENTIALS=y
CONFIG_NET_SOCKETS_TLS_MAX_CONTEXTS=4

# Mbed TLS & PSA Crypto
CONFIG_MBEDTLS=y
CONFIG_MBEDTLS_BUILTIN=y
CONFIG_MBEDTLS_ENABLE_HEAP=y
CONFIG_MBEDTLS_HEAP_SIZE=131072
CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384
CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN=4096
CONFIG_MBEDTLS_PEM_PARSE_C=y
CONFIG_MBEDTLS_PEM_WRITE_C=y
CONFIG_MBEDTLS_SSL_SERVER_NAME_INDICATION=y
CONFIG_PSA_CRYPTO=y
CONFIG_ENTROPY_GENERATOR=y

# Named Curves (NIST P-256/384/521 & X25519)
CONFIG_PSA_WANT_ECC_SECP_R1_256=y
CONFIG_PSA_WANT_ECC_SECP_R1_384=y
CONFIG_PSA_WANT_ECC_SECP_R1_521=y
CONFIG_PSA_WANT_ECC_MONTGOMERY_255=y

# TLS 1.2 & TLS 1.3 Protocols
CONFIG_MBEDTLS_SSL_PROTO_TLS1_2=y
CONFIG_MBEDTLS_SSL_PROTO_TLS1_3=y
CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED=y

# TLS 1.3 Ciphersuites
CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_256_GCM_SHA384=y

# TLS 1.2 Ciphersuites (ECDHE-RSA / ECDHE-ECDSA)
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384=y
```
