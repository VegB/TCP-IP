//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

define($in_mac fa:4d:ca:05:0e:0c, $out_mac ee:51:c3:e3:9c:9d , $dev veth12)

//rp :: LossyRouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac, LOSS 0.97, DELAY 0.2 );
rp::RouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac);
tcp :: ReceiverTCP(MY_ADDRESS 1, OTHER_ADDRESS 0);
buffer :: ReceiverBuffer;
ip :: BasicIP;

ip[2]->rp->[2]ip;
tcp[0]->[0]buffer[0]->[0]tcp;
ip[3]->[1]buffer[1]->[3]ip;
ip[0]->Discard;
ip[1]->Discard;
Idle->[0]ip;
Idle->[1]ip;

