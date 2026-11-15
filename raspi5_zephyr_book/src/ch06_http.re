= HTTP 通信の実現と TCP パイプラインの構築

== Zephyr ネイティブ TCP スタックの設定

平文の HTTP 通信を行うため、Zephyr のネイティブ TCP スタック（@<tt>{CONFIG_NET_TCP=y}）を有効化します。

//emlist{
CONFIG_NET_TCP=y
CONFIG_NET_MAX_CONTEXTS=10
CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT=1000
CONFIG_NET_SOCKETS_CONNECT_TIMEOUT=15000
//}

== バッファプール（net_buf / net_pkt）のサイジング

TCP 通信では、ウィンドウ制御、セグメントの再送制御、受信データのバッファリングが行われます。デフォルトのバッファ数（16個程度）では、Web ページを受信した際に一瞬でプールが枯渇し、パケットドロップが発生します。

//emlist{
CONFIG_NET_PKT_RX_COUNT=32
CONFIG_NET_BUF_RX_COUNT=128
CONFIG_NET_PKT_TX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=128
//}

== CUI コマンド @<tt>{net http get} での平文通信成功

Zephyr Shell の @<tt>{http_client} 拡張（@<tt>{CONFIG_HTTP_CLIENT=y}）を用いて、@<tt>{http://google.com/} にアクセスします。

//emlist{
uart:~$ net http get http://google.com/
Connecting to 172.217.209.138:80 (timeout: 15000 ms)...
<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
<H1>301 Moved</H1>
The document has moved
<A HREF="http://www.google.com/">here</A>.
</BODY></HTML>
//}

TCP 3-way ハンドシェイクが成功し、Google サーバーから @<tt>{HTTP/1.1 301 Moved Permanently} の HTML レスポンスが返ってきました！
平文の Web 通信パイプラインはこれで完成です。

