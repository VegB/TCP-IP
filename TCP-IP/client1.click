//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

define($in_mac 7e:c2:a3:b4:b5:0d , $out_mac ea:2f:5c:27:19:99, $dev veth1)

//rp :: LossyRouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac, LOSS 0.9, DELAY 0.2 );
rp::RouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac);

client::BasicTCP(MY_ADDRESS 1, OTHER_ADDRESS 2, DELAY 2);
bc::BasicIP;
client->rp->bc
bc[0]->[0]client;
bc[1]->Discard;
bc[2]->[1]client;

