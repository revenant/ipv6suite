// XXX to be eliminated on the long term (should use controlInfo() instead)  --AV
enum IPProtocolId;
message IPInterfacePacket
{
    properties:
        omitGetVerb=true;
    fields:
        int protocol enum(IPProtocolId);
        string destAddr;
        string srcAddr;
        short timeToLive;
};
