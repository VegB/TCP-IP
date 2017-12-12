//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

define($out_mac1 12:e2:0c:30:4a:5f, $in_mac1 b6:3a:2d:f5:5a:cc, $dev1 veth6)
define($out_mac2 26:40:bf:09:4a:f3, $in_mac2 46:71:6b:56:42:8a, $dev2 veth9)
//define($out_mac3 ca:04:a4:16:9b:65, $in_mac3 d6:5b:f0:20:03:23, $dev3 veth5)


/*rp1 :: LossyRouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1, LOSS 0.9, DELAY 0.2 );
rp2 :: LossyRouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2, LOSS 0.9, DELAY 0.2 );
rp3 :: LossyRouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3, LOSS 0.9, DELAY 0.2 );*/

rp1 :: RouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1);
rp2 :: RouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2);
//rp3 :: RouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3);

router::BasicRouter(MY_IP 4, OUT_PORT_NUM 2, TOTAL_NODE_NUM 6);
rp1->[0]router[0]->rp1;
rp2->[1]router[1]->rp2;
//rp3->[2]router[2]->rp3;

