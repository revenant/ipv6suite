// XXX to be eliminated on the long term (should use controlInfo() instead)  --AV
message IPInterfacePacket extends cPacket
{
    properties:
        omitGetVerb=true;
    fields:
        string destAddr;
        string srcAddr;
        short timeToLive;
};
