= 【激闘録1】64-bit ARM の Instruction Abort とスタック破壊の特定

== 突如発生した CPU Exception（0x338370）

DNS クエリコマンド @<tt>{net dns query google.com} を実行した直後、以下の深刻なパニックログが出力されてシステムが停止（Halt）しました。

//emlist{
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
//}

== レジスタダンプと PTE Walk の読み解き方

このエラーログは、ARMv8-A アーキテクチャの内部挙動を雄弁に物語っています：

 1. @<tt>{EC: 0x21} (Instruction Abort): CPU が命令フェッチを実行しようとしたアドレスが無効、あるいは実行権限がない。
 2. @<tt>{FAR_ELn: 0x338370}: 例外が発生したフェッチ先アドレス。
 3. @<tt>{PTE walk} の結果: @<tt>{page AP=EL_higher RW G AF}。
   - なんと、該当アドレス（@<tt>{0x338370}）の MMU ページ属性は @<b>{「RW（Read/Write）」であり、「Execute（実行可能）」ビットが立っていません（データ領域 / スタック領域）}！
 4. @<b>{レジスタの値}:
   - @<tt>{lr: 0x0000000000338370}
   - @<tt>{x8} 〜 @<tt>{x18}: @<tt>{0xaaaaaaaaaaaaaaaa}（Zephyr のスタック初期化フィルパターン @<tt>{0xAA}）！

関数のリターンアドレス（@<tt>{lr}）に、実行コード領域ではなくスタック領域のアドレスが書き込まれ、さらにレジスタ群がスタック未初期化パターン @<tt>{0xAA} で埋め尽くされていることから、@<b>{「典型的なスタックオーバーフローによる関数リターン先破壊」} であることが確定しました。

== 真犯人：DNS ディスパッチャのスタックオーバーフロー

問題は「どのスレッドが溢れたのか」です。
カレントスレッドは @<tt>{shell_uart}（@<tt>{0x327000}）となっていますが、リンカーマップファイル（@<tt>{zephyr.map}）を精査したところ、驚くべき配置が判明しました：

//emlist{
0x00328000 - 0x0032c000: shell_uart_stack
0x0032c000 - 0x0032c500: service_thread_stack (CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=1200)
//}

Zephyr のソケットサービスディスパッチャスレッド（@<tt>{service_thread}）は、DNS レスポンスのパケットを受信してコールバックを呼び出します。
デフォルト設定の @<tt>{CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=1200}（1200バイト）は 32-bit マイコン向けのサイズであり、@<b>{64-bit ARM ではポインタが 8 バイト、スタックアライメントが 16 バイト要求されるため、ネストした関数呼び出しであっという間に 1200 バイトを突破} します。

その結果、直下に配置されていた @<tt>{shell_uart_stack} のフレームを突き破り、リターンアドレスを破壊していたのです。

//image[ch05_stack_panic][SRAM メモリマップと隣接スレッドスタック侵食のメカニズム]{
//}

== 64-bit 環境に最適化したスタックサイズ設計

この問題を根治するため、ネットワークおよびシステム全体のスタックサイズを 64-bit ARM 向けに再設計・拡張しました：

//emlist{
# 64-bit ARM Stack Sizes (スタック破壊防止)
CONFIG_SHELL_STACK_SIZE=16384
CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE=8192
CONFIG_NET_MGMT_EVENT_STACK_SIZE=4096
CONFIG_NET_RX_STACK_SIZE=8192
CONFIG_NET_TX_STACK_SIZE=8192
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=8192
CONFIG_NET_TCP_WORKQ_STACK_SIZE=8192
//}

スタックを拡張して再ビルドした結果、DNS 解決は見事にノーエラーで完了しました：

//emlist{
uart:~$ net dns query google.com
Query for 'google.com' sent.
dns: 142.251.118.138
dns: 142.251.118.100
dns: 142.251.118.113
dns: All results received
//}

