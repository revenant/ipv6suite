cd %~dp0
call ..\omnetpp\setenv-vc71.bat
nmake ROOT=%~dp0 MAKEMAKE=opp_nmakemake EXT=.vc -f makemakefiles || echo *** ERROR GENERATING MAKEFILES ***

echo on
dir /s/b *.ned > nedfiles.lst
perl -i.bak -pe "s/.*[^d]\n$//;s|\\|/|g;s|.*?INET.*?/||" nedfiles.lst
perl -i.bak -pe "s|^Examples/.*||" nedfiles.lst
perl -i.bak -pe "s|^Unsupported/.*||" nedfiles.lst
perl -i.bak -pe "s|^Tests/.*||" nedfiles.lst

