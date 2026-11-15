= 【激闘録2】HTTPS（TLS 1.2 / TLS 1.3）の暗号化の壁を突破する

現代のインターネットでは、ほぼすべての Web サービスが HTTPS（TLS 暗号化）を必須としています。ここからが真の難所でした。

本章で立ちふさがった3つの致命的関門（RSA 暗号スイート不足による切断、TLS 1.3 スイート空トラップ、謎のエラー @<tt>{-0x3a00} による証明書パース破棄）と、正常なハンドシェイク成立までのシーケンス全貌を下図に示します。

//image[ch07_tls_handshake][TLS 1.3 / 1.2 ハンドシェイクと 3 大関門の衝突ポイント]{
//}

== Mbed TLS & PSA Crypto の組み合わせ

Zephyr 3.7 以降、暗号化スタックは従来の Mbed TLS 単体構成から、標準化された @<b>{PSA Crypto（Platform Security Architecture）} を基盤とするアーキテクチャへと移行しています。

//emlist{
CONFIG_NET_SOCKETS_SOCKOPT_TLS=y
CONFIG_MBEDTLS=y
CONFIG_MBEDTLS_BUILTIN=y
CONFIG_PSA_CRYPTO=y
CONFIG_ENTROPY_GENERATOR=y
//}

== 関門1：RSA 暗号スイートの不足による即時切断（-113 ECONNABORTED）

最初に @<tt>{net http get -k https://google.com/} を実行した際、以下のエラーが発生しました：

//emlist{
Connecting to 172.217.209.102:443 (timeout: 15000 ms)...
Failed to connect to 172.217.209.102:443, ret=-1, errno=113
HTTP GET request failed (-113)
//}

@<tt>{errno 113} は @<tt>{ECONNABORTED}（Software caused connection abort）です。
ソースコード（@<tt>{subsys/net/lib/sockets/sockets_tls.c}）を調査したところ、@<tt>{zsock_connect()} による TCP 接続は成功しているものの、その直後の @<tt>{tls_mbedtls_handshake()} が失敗すると、Zephyr はエラーを一律に @<tt>{ECONNABORTED} に変換して返していることがわかりました。

@<b>{原因}:
初期設定では ECDSA 暗号スイートのみが有効化されていました。しかし、Google などの大規模 Web サーバーは RSA 証明書（2048/4096 bit）を提示することが多く、クライアント側に RSA 暗号スイート（@<tt>{ECDHE-RSA-AES-GCM}）が存在しないため、ネゴシエーションに失敗していたのです。

@<b>{対応}:
@<tt>{prj.conf} に RSA 暗号スイートを追加：
//emlist{
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384=y
//}

== 関門2：TLS 1.3 ネゴシエーションの落とし穴

しかし、これでも接続は失敗し続けました。
設定を確認すると、プロトコルとして @<tt>{CONFIG_MBEDTLS_SSL_PROTO_TLS1_3=y} を有効にしていたにもかかわらず、TLS 1.3 用の暗号スイート Kconfig（@<tt>{CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_...\）が1つも選択されていなかった} のです。

クライアントが「TLS 1.3 に対応している」と ClientHello で宣言しながら、提示する TLS 1.3 暗号スイートが空だったため、サーバー側でハンドシェイク不一致となっていました。

@<b>{対応}:
//emlist{
CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_GCM_SHA256=y
CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_256_GCM_SHA384=y
//}

== 関門3：謎のエラーコード @<tt>{-0x3a00} を追え

暗号スイートを追加したものの、依然としてハンドシェイクは失敗します。
そこで、Mbed TLS の内部エラーコードを直接露出させるため、@<href>{file:///home/matsuoka/zephyr-workspace/zephyr/subsys/net/lib/sockets/sockets_tls.c, sockets_tls.c} のハンドシェイク失敗ハンドラにデバッグ出力を仕込みました：

//emlist{
printk("[TLS] Handshake error: -0x%x\n", -ret);
//}

再起動してコマンドを実行すると、決定的なログが現れました：

//emlist{
uart:~$ net http get -k https://google.com/
Connecting to 172.217.209.101:443 (timeout: 15000 ms)...
[TLS] Handshake error: -0x3a00
Failed to connect to 172.217.209.101:443, ret=-1, errno=113
//}

@<tt>{-0x3a00} ―― この謎の 16進数エラーコードは何を意味しているのか？

== 解決：@<tt>{MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE} と楕円曲線（secp384r1, X25519）

Mbed TLS / TF-PSA-Crypto のソースツリーを全件走査した結果、@<tt>{modules/crypto/tf-psa-crypto/include/mbedtls/pk.h} にその定義を発見しました：

//emlist{
#define MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE -0x3A00
//}

エラーの意味は @<b>{「未知の Named Curve（楕円曲線）」}！

さらに @<tt>{tf-psa-crypto/extras/pkparse.c} および @<tt>{utilities/oid.c} を追跡したところ、以下の真実が浮き彫りになりました：

//emlist{
/* oid.c */
static const oid_ecp_grp_t oid_ecp_grp[] = {
#if defined(PSA_WANT_ECC_SECP_R1_256)
    { OID_DESCRIPTOR(MBEDTLS_OID_EC_GRP_SECP256R1, "secp256r1", "secp256r1"), MBEDTLS_ECP_DP_SECP256R1 },
#endif
#if defined(PSA_WANT_ECC_SECP_R1_384)
    { OID_DESCRIPTOR(MBEDTLS_OID_EC_GRP_SECP384R1, "secp384r1", "secp384r1"), MBEDTLS_ECP_DP_SECP384R1 },
#endif
    ...
//}

Google のサーバー証明書チェーン（GTS Root / GlobalSign 中間証明書）や TLS 1.3 の鍵交換（Key Share）では、標準の @<tt>{secp256r1} だけでなく @<tt>{secp384r1} や @<tt>{X25519} (Curve25519)、@<tt>{secp521r1} が多用されます。
従来の構成では @<tt>{secp256r1} しか組み込まれていなかったため、証明書パース時に未知の OID と判定され、即座に @<tt>{-0x3A00} で破棄されていたのです。

@<b>{最終修正}:
@<tt>{prj.conf} にインターネットで用いられる主要な楕円曲線をすべて有効化：

//emlist{
# 楕円曲線 (Curves) 完全対応
CONFIG_PSA_WANT_ECC_SECP_R1_256=y
CONFIG_PSA_WANT_ECC_SECP_R1_384=y
CONFIG_PSA_WANT_ECC_SECP_R1_521=y
CONFIG_PSA_WANT_ECC_MONTGOMERY_255=y  # X25519
//}

さらに、大きな証明書チェーンを安全に展開できるよう、Mbed TLS ヒープを 128KB（@<tt>{CONFIG_MBEDTLS_HEAP_SIZE=131072}）に拡張しました。

== アーキテクチャ的考察：Mbed TLS と PSA Crypto の二重抽象化レイヤの落とし穴

今回直面した @<tt>{-0x3a00} エラーは、単なる設定漏れではなく、@<b>{Zephyr 3.7+ における暗号スタックの二重抽象化レイヤ構造} に起因する構造的な落とし穴でした。

//image[ch07_psa_layers][Mbed TLS と PSA Crypto の二重抽象化レイヤ構造]{
//}

 1. @<b>{二重化された Kconfig 体系}:
   上位の Mbed TLS レイヤでいくら TLS 1.3 暗号スイート（@<tt>{CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_...}）を有効化しても、下位の PSA Crypto レイヤ側で該当する曲線（@<tt>{CONFIG_PSA_WANT_ECC_SECP_R1_384} や @<tt>{CONFIG_PSA_WANT_ECC_MONTGOMERY_255}）がビルド対象に含まれていなければ、暗号エンジン実体は生成されません。
 2. @<b>{多重エラー変換による原因隠蔽}:
   下位の PSA Crypto が @<tt>{PSA_ERROR_NOT_SUPPORTED} を返すと、Mbed TLS はそれを @<tt>{-0x3A00} に変換し、さらに Zephyr の Socket レイヤが BSD errno の @<tt>{-113 (ECONNABORTED)} に丸め込んでしまいます。
   この「多段抽象化によるエラーコードの不可視化」こそが、デバッグを極限まで難解にしていた真犯人でした。

