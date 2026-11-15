= ネットワークスタック（L3/L4）と DNS 名前解決

== DHCPv4 による自動 IP 取得

Wi-Fi リンクが確立されると、Zephyr の DHCPv4 クライアント（@<tt>{CONFIG_NET_DHCPV4=y}）が自動的にルーターへ DISCOVER パケットを送信し、IP アドレス、サブネットマスク、ゲートウェイ、DNS サーバーを自動取得します。

//emlist{
[00:00:15.340,000] <inf> net_dhcpv4: Received address: 192.168.0.28
[00:00:15.345,000] <inf> net_dhcpv4: Lease time: 86400 seconds
[00:00:15.350,000] <inf> net_dhcpv4: Router: 192.168.0.1
//}

== ICMP Ping による導通確認

取得した IP で外の世界と疎通できているかを @<tt>{net ping} で確認します。

//emlist{
uart:~$ net ping 8.8.8.8
PING 8.8.8.8
28 bytes from 8.8.8.8 to 192.168.0.28: icmp_seq=1 ttl=118 time=11.57 ms
28 bytes from 8.8.8.8 to 192.168.0.28: icmp_seq=2 ttl=118 time=11.57 ms
--- ping statistics ---
2 packets transmitted, 2 received, 0% packet loss
//}

これにより、L2（Wi-Fi/Ethernet）、L3（IPv4/ICMP）のパケット送受信パスが完全に成立していることが確認できました。

== DNS リゾルバの実装と挙動

ドメイン名から IP アドレスを取得するため、Zephyr の DNS リゾルバ（@<tt>{CONFIG_DNS_RESOLVER=y}）を有効化します。

//emlist{
CONFIG_DNS_RESOLVER=y
CONFIG_DNS_RESOLVER_MAX_SERVERS=2
CONFIG_DNS_SERVER_IP_ADDRESSES=y
CONFIG_DNS_SERVER1="8.8.8.8"
//}

しかし、この DNS 機能を有効化して名前解決を試みた瞬間、@<b>{最初の致命的なクラッシュ} が発生しました。

