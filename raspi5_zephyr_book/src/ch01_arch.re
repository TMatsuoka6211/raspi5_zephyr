= Raspberry Pi 5 と Zephyr RTOS のアーキテクチャ

本プロジェクトでは、Raspberry Pi 5 上で Zephyr RTOS をベアメタル起動し、Infineon CYW43455 との SDIO 通信からネイティブ TCP/IP、そして TLS 1.3 による暗号化通信までを完全統合しました。
全体を構成するハードウェア、ドライバ、ネットワーク、セキュリティ、アプリケーションの階層構造（システム全体アーキテクチャ）を下図に示します。

//image[ch01_arch_overall][Raspberry Pi 5 × Zephyr RTOS システム全体アーキテクチャ図]{
//}

== BCM2712 (Cortex-A76 AArch64) の特徴

Raspberry Pi 5 に搭載されている SoC「BCM2712」は、ARM Cortex-A76 を4コア搭載するパワフルな 64-bit プロセッサです。従来の Cortex-M などのマイコンと異なり、以下の特徴を持ちます：

 * @<b>{64-bit アドレス空間}: ポインタやレジスタサイズが 8 バイト（64-bit）となるため、32-bit マイコンに比べてスタックフレームの消費量が 1.5〜2 倍以上に増大します。
 * @<b>{仮想記憶と MMU（メモリ管理ユニット）}: Zephyr でも MMU によるページテーブル（Translation Table）が構築され、アクセス権限（AP）や実行禁止（XN: Execute Never）が厳格に管理されます。
 * @<b>{キャッシュライン}: 64 バイトのキャッシュラインと厳格なアライメントが要求されます。

== Zephyr RTOS のブートシーケンスとビルド環境

Zephyr RTOS において、Raspberry Pi 5 は @<tt>{rpi_5/bcm2712} ボード定義としてサポートが進められています。
ビルドツール @<tt>{west} を用いて、サンプルアプリケーション（@<tt>{samples/hello_world}）に Wi-Fi、TCP、TLS、HTTP Client の Kconfig を重層的に追加していく構成をとります。

//emlist{
# ビルドコマンド例
cd /path/to/zephyr-workspace/zephyr
west build -d build -b rpi_5/bcm2712 samples/hello_world
//}

生成された @<tt>{build/zephyr/zephyr.bin} を SD カードのルートディレクトリに @<tt>{kernel8.img} として配置することで、Raspberry Pi 5 の EEPROM ブートローダーからダイレクトに起動させることができます。

