import
    "Router6",
    "UDPNode",
    "WorldProcessor";

module  SingleRouter
    submodules:
     worldProcessor: WorldProcessor;
            display: "p=165,156;i=bwgen_s";
     client1: UDPNode;
         parameters:
            IPForward = false;
         gatesizes:
            in[1],
            out[1];
         display: "p=42,56;i=comp";
     client2: UDPNode;
         parameters:
           IPForward = false;
         gatesizes:
           in[1],
           out[1];
         display: "p=42,156;i=comp";
     server: UDPNode;
         parameters:
           IPForward = false;
         gatesizes:
           in[1],
           out[1];
         display: "p=288,106;i=comp";
     router: Router6;
         gatesizes:
           in[3],
           out[3];
         display: "p=165,106;i=ipc";
  connections nocheck:
      client1.in[0] <-- router.out[0];
      client1.out[0] -->router.in[0];

      client2.in[0] <-- router.out[1];
      client2.out[0] -->router.in[1];

      server.in[0] <-- router.out[2];
      server.out[0] -->router.in[2];
endmodule

network singleRouter : SingleRouter
endnetwork
