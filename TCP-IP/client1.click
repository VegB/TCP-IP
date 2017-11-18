//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

define($in_mac 7e:c2:a3:b4:b5:0d , $out_mac ea:2f:5c:27:19:99, $dev veth1)

//rp :: LossyRouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac, LOSS 0.9, DELAY 0.2 );
rp::RouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac);

tcp :: SenderTCP(MY_ADDRESS 0, OTHER_ADDRESS 1, DELAY 2);
buffer :: SenderBuffer;
ip :: BasicIP;

ip[1]->rp->[1]ip;
tcp[0]->[0]buffer[0]->[0]tcp;
ip[0]->[1]buffer[1]->[0]ip;
ip[2]->Discard;
ip[3]->Discard;
Idle->[2]ip;
Idle->[3]ip;
