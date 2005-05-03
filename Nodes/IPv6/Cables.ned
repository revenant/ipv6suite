//
// Internet link. Delay only.
//# FIXME data rate?
//
channel internetCable 
    delay 0.0; //0.012
    datarate 100000000;
endchannel 


//
// Intranet link. Delay only.
//# FIXME data rate?
//
channel intranetCable 
    delay 0.0;
    datarate 500000;//100000000; 
endchannel 

channel homeLink
    delay 0.0;
    datarate 10000000;
endchannel;
