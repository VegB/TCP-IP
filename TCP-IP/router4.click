//require(library /home/comnetsii/elements/lossyrouterport.click);
require(library /home/comnetsii/elements/routerport.click);

<<<<<<< HEAD
define($out_mac1 12:fa:81:7e:41:11, $in_mac1 8e:b2:8f:d8:da:6e, $dev1 veth8)
define($out_mac2 46:71:6b:56:42:8a, $in_mac2 26:40:bf:09:4a:f3, $dev2 veth10)
define($out_mac3 fa:4d:ca:05:0e:0c, $in_mac3 ee:51:c3:e3:9c:9d, $dev3 veth11)
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
router::BasicRouter(MY_IP 5, OUT_PORT_NUM 3, TOTAL_NODE_NUM 6);
=======
router::BasicRouter(MY_IP 2, OUT_PORT_NUM 2, TOTAL_NODE_NUM 3);
>>>>>>> df2ef33b31a8d1bbd2b1ff3e09352885f035b844
rp1->[0]router[0]->rp1;
rp2->[1]router[1]->rp2;
rp3->[2]router[2]->rp3;
