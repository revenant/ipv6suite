:rem *** ADOPT THE NEXT TWO LINES ACCORDING TO YOUR OMNET++ INSTALLATION ***
call ..\omnetpp\setenv-vc71.bat
set MAKEMAKE=cmd /c d:\home\omnetpp\bin\opp_nmakemake

: #--------------------------------------

set root=%~dp0
set OPTS=-f -N -b %root% -c %root%\inetconfig.vc

set ALL_INET_INCLUDES=-I%root%/Network/IPv4 -I%root%/Network/IPv4d -I%root%/Network/AutoRouting -I%root%/Transport/TCP -I%root%/Transport/UDP -I%root%/NetworkInterfaces -I%root%/NetworkInterfaces/_802 -I%root%/NetworkInterfaces/ARP -I%root%/NetworkInterfaces/Ethernet -I%root%/NetworkInterfaces/PPP -I%root%/Applications/Generic -I%root%/Applications/Ethernet -I%root%/Applications/TCPApp -I%root%/Applications/UDPApp -I%root%/Applications/PingApp -I%root%/Base -I%root%/Util -I%root%/Nodes/INET
set ALL_MPLS_INET_INCLUDES=%ALL_INET_INCLUDES% -I%root%/Network/MPLS -I%root%/Network/LDP -I%root%/Network/RSVP_TE -I%root%/Network/Scenario -I%root%/Nodes/MPLS
set ALL_IPv6_INCLUDES=%ALL_INET_INCLUDES% -I%root%/Network/IPv6

:set ALL_MODEL_OPTS=%OPTS% -w %ALL_MPLS_INET_INCLUDES%
set ALL_MODEL_OPTS=%OPTS% -n

: #--------------------------------------

echo on
echo @%root%@
%MAKEMAKE% %OPTS% -n -r

cd %root%\Applications && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples && %MAKEMAKE% %OPTS% -n -r
cd %root%\Tests && %MAKEMAKE% %OPTS% -n -r
cd %root%\Network && %MAKEMAKE% %OPTS% -n -r
cd %root%\NetworkInterfaces && %MAKEMAKE% %OPTS% -n -r -I..\Util
cd %root%\Nodes && %MAKEMAKE% %OPTS% -n -r
cd %root%\PHY && %MAKEMAKE% %OPTS% -n -r
cd %root%\Transport && %MAKEMAKE% %OPTS% -n -r
cd %root%\Base && %MAKEMAKE% %OPTS% -n -r
:FIXME Util should not depend on IPv6 stuff and on PHY\Mobility!
cd %root%\Util && %MAKEMAKE% %OPTS% -n -r -I. -I..\PHY\Mobility -I..\Network\IPv6 -I..\Network\IPv4 -I..\World -I..\Base

:---------------
:FIXME try to eliminate dep in NetworkInterfaces, Ethernet6, PPP6, Wireless, Mobility!!
cd %root%\Network\IPv6 && %MAKEMAKE% %OPTS% -n -r -I..\..\NetworkInterfaces -I..\..\NetworkInterfaces\PPP6 -I..\..\NetworkInterfaces\Ethernet6 -I..\..\NetworkInterfaces\Wireless  -I..\HMIPv6  -I..\MIPv6 -I..\..\Util -I..\..\Util\Loki -I..\..\World -I..\..\PHY\Mobility -I. -I..\.. -I..\IPv4 -I..\..\Base
:FIXME try to eliminate MIP dependency on HMIP and on Wireless!
cd %root%\Network\MIPv6 && %MAKEMAKE% %OPTS% -n -r -I..\IPv6 -I..\HMIPv6 -I..\..\NetworkInterfaces -I..\..\NetworkInterfaces\Ethernet6 -I..\..\NetworkInterfaces\Wireless -I..\..\Util -I..\..\Util\Loki -I..\..\World -I..\..\PHY -I. -I..\.. -I..\IPv4 -I..\..\Base
cd %root%\Network\HMIPv6 && %MAKEMAKE% %OPTS% -n -r -I..\IPv6 -I..\MIPv6 -I..\..\Util -I..\..\Util\Loki -I..\..\World -I..\..\PHY -I. -I..\.. -I..\IPv4 -I..\..\Base
cd %root%\Network\RIP && %MAKEMAKE% %OPTS% -n -r

cd %root%\Util\Topology && %MAKEMAKE% %OPTS% -n -r
cd %root%\Util\XML && %MAKEMAKE% %OPTS% -n -r
cd %root%\Util\adHocSim && %MAKEMAKE% %OPTS% -n -r
cd %root%\Util\adHocSim\h && %MAKEMAKE% %OPTS% -n -r

:FIXME remove -IWireless from Eth!!!!! --AV
cd %root%\NetworkInterfaces\Ethernet6 && %MAKEMAKE% %OPTS% -n -r -I..\..\PHY\Wireless -I.. -I..\.. -I..\..\Util -I..\..\Network\IPv6 -I..\..\World -I..\..\PHY
:FIXME remove -IEthernet6 from PPP6!!!!! (dep on MACAddress) --AV
cd %root%\NetworkInterfaces\PPP6 && %MAKEMAKE% %OPTS% -n -r -I..\Ethernet6 -I.. -I..\.. -I..\..\Util -I..\..\Network\IPv6 -I..\..\World -I..\..\PHY
:FIXME sure we need -IMobility here? --AV
:FIXME remove -IPPP6 !!!!!! (dep on IPv6PPPAPInterface ???!!!!
cd %root%\NetworkInterfaces\Wireless && %MAKEMAKE% %OPTS% -n -r -I../PPP6 -I.. -I..\.. -I..\..\Util -I..\Ethernet6 -I..\..\Network\IPv6 -I..\..\World -I..\..\PHY\Mobility -I..\..\PHY\Wireless

cd %root%\Applications\MLD && %MAKEMAKE% %OPTS% -n -r
cd %root%\Applications\Ping6 && %MAKEMAKE% %OPTS% -n -r
cd %root%\Applications\VideoStream && %MAKEMAKE% %OPTS% -n -r

cd %root%\PHY\Mobility && %MAKEMAKE% %OPTS% -n -r
cd %root%\PHY\Wireless && %MAKEMAKE% %OPTS% -n -r

cd %root%\Transport\UDP6 && %MAKEMAKE% %OPTS% -n -r
cd %root%\World && %MAKEMAKE% %OPTS% -n -r -I..\PHY\Mobility
cd %root%\Nodes\IPv6 && %MAKEMAKE% %OPTS% -n -r
----




cd %root%\Applications\Generic && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Base -I..\..\Util
cd %root%\Applications\Ethernet && %MAKEMAKE% %OPTS% -n -r -I..\..\NetworkInterfaces\Ethernet -I..\..\NetworkInterfaces\_802 -I..\..\Base -I..\..\Util
cd %root%\Applications\PingApp && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Base -I..\..\Util
cd %root%\Applications\TCPApp && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Transport\TCP -I..\..\Base -I..\..\Util
cd %root%\Applications\UDPApp && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Transport\UDP -I..\..\Base -I..\..\Util

cd %root%\Network\IPv4 && %MAKEMAKE% %OPTS% -n -r -I..\..\Base -I..\..\Util
cd %root%\Network\IPv4d && %MAKEMAKE% %OPTS% -n -r -I..\IPv4 -I..\..\Base -I..\..\Util
cd %root%\Network\AutoRouting && %MAKEMAKE% %OPTS% -n -r -I..\IPv4  -I..\..\Base -I..\..\Util
cd %root%\Network\MPLS && %MAKEMAKE% %OPTS% -n -r -I..\IPv4 -I..\IPv4d -I..\..\Base -I..\..\Util
cd %root%\Network\LDP && %MAKEMAKE% %OPTS% -n -r -I..\IPv4 -I..\IPv4d -I..\..\Transport\UDP -I..\..\Transport\TCP -I..\MPLS -I..\..\Base -I..\..\Util
cd %root%\Network\RSVP_TE && %MAKEMAKE% %OPTS% -n -r -I..\IPv4 -I..\IPv4d -I..\MPLS -I..\..\Base -I..\..\Util
cd %root%\Network\Scenario && %MAKEMAKE% %OPTS% -n -r -I..\IPv4 -I..\IPv4d -I..\MPLS -I..\RSVP_TE -I..\..\Base -I..\..\Util

cd %root%\NetworkInterfaces\PPP && %MAKEMAKE% %OPTS% -n -r -I..\..\Base -I..\..\Util -I..\..\Network\IPv4
cd %root%\NetworkInterfaces\_802 && %MAKEMAKE% %OPTS% -n -r -I..\..\Base -I..\..\Util -I..\..\Network\IPv4
cd %root%\NetworkInterfaces\Ethernet && %MAKEMAKE% %OPTS% -n -r -I..\..\Base -I..\..\Util -I..\_802 -I..\..\Network\IPv4
cd %root%\NetworkInterfaces\ARP && %MAKEMAKE% %OPTS% -n -r -I..\..\Base -I..\..\Util -I..\..\Network\IPv4 -I..\_802 -I..\Ethernet

cd %root%\Nodes\INET && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Network\IPv4d -I..\..\Network\IPv4\QoS -I..\..\Transport\UDP -I..\..\NetworkInterfaces\PPP -I..\..\NetworkInterfaces  -I..\..\Applications\Generic -I..\..\Applications\TCPApp -I..\..\Applications\UDPApp -I..\..\Applications\PingApp -I..\..\Base -I..\..\Util
cd %root%\Nodes\MPLS && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Network\IPv4d -I..\..\Network\IPv4\QoS -I..\..\Network\MPLS -I..\..\Network\LDP -I..\..\Network\RSVP_TE -I..\..\Transport\UDP -I..\..\NetworkInterfaces\PPP -I..\..\NetworkInterfaces  -I..\..\Applications\Generic -I..\..\Applications\TCPApp -I..\..\Applications\PingApp -I..\INET -I..\..\Base -I..\..\Util

cd %root%\Transport\UDP && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Base -I..\..\Util
cd %root%\Transport\RTP && %MAKEMAKE% %OPTS% -n -r -I..\..\Network\IPv4 -I..\..\Base -I..\..\Util
cd %root%\Transport\TCP && %MAKEMAKE% %OPTS% -n -I..\..\Network\IPv4 -I..\..\Base -I..\..\Util

cd %root%\Examples\bin && %MAKEMAKE% %OPTS% -w -o INET %ALL_MPLS_INET_INCLUDES%

cd %root%\Examples\Ethernet && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\INET && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\MPLS && %MAKEMAKE% %OPTS% -n -r

cd %root%\Examples\Ethernet\ARPTest && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\Ethernet\LANs && %MAKEMAKE% %ALL_MODEL_OPTS%

cd %root%\Examples\INET\NClients && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\INET\FlatNet && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\INET\KIDSNw1 && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\INET\Multicast && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\INET\RouterPerf && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\INET\BulkTransfer && %MAKEMAKE% %ALL_MODEL_OPTS%

cd %root%\Examples\MPLS\ldp-mpls1 && %MAKEMAKE% %ALL_MODEL_OPTS%
cd %root%\Examples\MPLS\TestTE1 && %MAKEMAKE% %ALL_MODEL_OPTS% -I..\Tester
cd %root%\Examples\MPLS\TestTE2 && %MAKEMAKE% %ALL_MODEL_OPTS% -I..\Tester
cd %root%\Examples\MPLS\TestTE3 && %MAKEMAKE% %ALL_MODEL_OPTS% -I..\Tester
cd %root%\Examples\MPLS\TestTE4 && %MAKEMAKE% %ALL_MODEL_OPTS% -I..\Tester
cd %root%\Examples\MPLS\TestTE5 && %MAKEMAKE% %ALL_MODEL_OPTS% -I..\Tester
cd %root%\Examples\MPLS\TestTE6 && %MAKEMAKE% %ALL_MODEL_OPTS% -I..\Tester


:-----------------
cd %root%\Examples\IPv6 && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\EthNetwork && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\HMIPv6Network && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\LargeTestNetwork && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\MelbourneNetwork && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\MIPv6Network && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\PingNetwork && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\TestNetwork && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\TunnelNet && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\UMLNet && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\WirelessEtherNetwork && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\WirelessEtherNetwork2 && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\WirelessEtherNetworkDual && %MAKEMAKE% %OPTS% -n -r
cd %root%\Examples\IPv6\WirelessTest && %MAKEMAKE% %OPTS% -n -r
:-----------------

cd %root%\Tests\MPLS && %MAKEMAKE% %OPTS% -n -r
cd %root%\Tests\MPLS\LDP1 && %MAKEMAKE% %OPTS% -w %ALL_MPLS_INET_INCLUDES%
cd %root%\Tests\NewTCP && %MAKEMAKE% %OPTS% -w %ALL_MPLS_INET_INCLUDES%

: #--------------------------------------

cd %root%
dir /s/b *.ned > nedfiles.lst
perl -i.bak -pe "s/.*[^d]\n$//;s|\\|/|g;s|.*?INET.*?/||" nedfiles.lst
perl -i.bak -pe "s|^Examples/.*||" nedfiles.lst
perl -i.bak -pe "s|^Unsupported/.*||" nedfiles.lst
perl -i.bak -pe "s|^Tests/.*||" nedfiles.lst
