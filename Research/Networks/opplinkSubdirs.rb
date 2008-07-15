Dir.chdir "Research/Networks" if not Dir.pwd =~ /Networks/
Dir.chdir "Networks" if not Dir.pwd =~ /Networks/
for d in %w|EthNetwork TestNetwork TunnelNet MIPv6Network HMIPv6Network Baseline|
#puts "dir is #{d}"
`bash -c ". ~/bash/functions && cd #{d} && opplink && cd -"`
#puts Dir.pwd
#exit 1 if $? != 0
end
