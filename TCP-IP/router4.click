//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

define($out_mac1 12:fa:81:7e:41:11, $in_mac1 8e:b2:8f:d8:da:6e, $dev1 veth8)
define($out_mac2 46:71:6b:56:42:8a, $in_mac2 26:40:bf:09:4a:f3, $dev2 veth10)
define($out_mac3 fa:4d:ca:05:0e:0c, $in_mac3 ee:51:c3:e3:9c:9d, $dev3 veth11)

/*rp1 :: LossyRouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1, LOSS 0.9, DELAY 0.2 );
rp2 :: LossyRouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2, LOSS 0.9, DELAY 0.2 );
rp3 :: LossyRouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3, LOSS 0.9, DELAY 0.2 );*/

rp1 :: RouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1);
rp2 :: RouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2);
rp3 :: RouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3);

router::BasicRouter(MY_IP 5, OUT_PORT_NUM 3, TOTAL_NODE_NUM 6);

rp1->[0]router[0]->rp1;
rp2->[1]router[1]->rp2;
rp3->[2]router[2]->rp3;
