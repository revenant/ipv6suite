import  
    "WorldProcessor", 
    "Router6", 
    "UDPNode", 
    "EtherHub"; 
                
                
//
// Please write comments here ;)
// 
module PrefixedTunnelNet
    submodules: 
        worldProcessor: WorldProcessor; 
            parameters: 
            display: "p=72,258;i=bwgen;b=40,28";
        router1: Router6; 
            parameters: 

                IPv6routingFile = "network.xml"; // TBD
            gatesizes: 
                in[2], // TBD
                out[2]; // TBD
            display: "p=103,199;i=router;b=40,28";
        router2: Router6; 
            parameters: 

                IPv6routingFile = "network.xml"; // TBD
            gatesizes: 
                in[2], // TBD
                out[2]; // TBD
            display: "p=195,148;i=router;b=40,28";
        router3: Router6; 
            parameters: 

                IPv6routingFile = "network.xml"; // TBD
            gatesizes: 
                in[2], // TBD
                out[2]; // TBD
            display: "p=275,100;i=router;b=40,28";
        hub1: Hub; 
            gatesizes: 
                in[3], // TBD
                out[3]; // TBD
            display: "p=83,121;i=cogwheel2_s;b=40,28";
        hub2: Hub; 
            gatesizes: 
                in[3], // TBD
                out[3]; // TBD
            display: "p=322,182;i=cogwheel2_s;b=15,10";
        client1: UDPNode; //
            parameters: 
                IPv6routingFile = "network.xml", 

                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=40,72;i=comp2_s;b=18,18";
        client2: UDPNode; //
            parameters: 
                IPv6routingFile = "network.xml", 

                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=111,52;i=comp2;b=32,32";
        server1: UDPNode; //
            parameters: 
                IPv6routingFile = "network.xml", 

                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=260,248;i=server1;b=30,34";
        server2: UDPNode; //
            parameters: 
                IPv6routingFile = "network.xml", 

                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=336,256;i=server1;b=30,34";
    connections: 
        router1.out[0] --> hub1.in[0]; 
        hub1.out[0] --> router1.in[0]; 
                                 
        router1.out[1] --> router2.in[0]; 
        router2.out[0] --> router1.in[1]; 
                                    
        router2.out[1] --> router3.in[0]; 
        router3.out[0] --> router2.in[1]; 
                                    
        router3.out[1] --> hub2.in[0]; 
        hub2.out[0] --> router3.in[1]; 

        client1.out[0] --> hub1.in[1];
        hub1.out[1] --> client1.in[0];
                                 
        client2.out[0] --> hub1.in[2]; 
        hub1.out[2] --> client2.in[0]; 
                                 
        server1.out[0] --> hub2.in[1]; 
        hub2.out[1] --> server1.in[0]; 

        server2.out[0] --> hub2.in[2]; 
        hub2.out[2] --> server2.in[0]; 

    display: "p=10,10;b=368,288";
endmodule 

network prefixedTunnelNet : PrefixedTunnelNet 
endnetwork 

