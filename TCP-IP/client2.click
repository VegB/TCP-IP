require(library /home/comnetsii/elements/lossyrouterport.click);
//require(library /home/comnetsii/elements/routerport.click);

define($in_mac 52:b0:1f:be:95:ac, $out_mac 22:c9:ae:81:8c:6e, $dev veth4)

rp :: LossyRouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac, LOSS 0.8, DELAY 0.2 );
//rp::RouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac);
tcp :: ReceiverTCP(MY_ADDRESS 2, OTHER_ADDRESS 1);
buffer :: ReceiverBuffer;
ip :: BasicIP;

ip[2]->rp->[2]ip;
tcp[0]->[0]buffer[0]->[0]tcp;
ip[3]->[1]buffer[1]->[3]ip;
ip[0]->Discard;
ip[1]->Discard;
Idle->[0]ip;
Idle->[1]ip;

