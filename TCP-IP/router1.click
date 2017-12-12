//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

<<<<<<< HEAD
define($out_mac1 d6:d4:0f:00:8a:bd, $in_mac1 46:cd:6d:c0:4d:f3, $dev1 veth2)
define($out_mac2 f6:c3:60:0a:e5:a7, $in_mac2 7a:60:bd:25:a9:f2, $dev2 veth3)
define($out_mac3 b6:3a:2d:f5:5a:cc, $in_mac3 12:e2:0c:30:4a:5f, $dev3 veth5)
=======
define($out_mac1 7e:c2:a3:b4:b5:0d, $in_mac1 ea:2f:5c:27:19:99, $dev1 veth2)
define($out_mac2 52:b0:1f:be:95:ac, $in_mac2 22:c9:ae:81:8c:6e, $dev2 veth3)
define($out_mac3 ca:04:a4:16:9b:65, $in_mac3 d6:5b:f0:20:03:23, $dev3 veth5)
>>>>>>> df2ef33b31a8d1bbd2b1ff3e09352885f035b844

/*rp1 :: LossyRouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1, LOSS 0.9, DELAY 0.2 );
rp2 :: LossyRouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2, LOSS 0.9, DELAY 0.2 );
rp3 :: LossyRouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3, LOSS 0.9, DELAY 0.2 );*/

rp1 :: RouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1);
rp2 :: RouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2);
rp3 :: RouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3);

<<<<<<< HEAD
router::BasicRouter(MY_IP 2, OUT_PORT_NUM 3, TOTAL_NODE_NUM 6);
=======
router::BasicRouter(MY_IP 2, OUT_PORT_NUM 2, TOTAL_NODE_NUM 3);
>>>>>>> df2ef33b31a8d1bbd2b1ff3e09352885f035b844
rp1->[0]router[0]->rp1;
rp2->[1]router[1]->rp2;
rp3->[2]router[2]->rp3;
