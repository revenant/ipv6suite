for d in %w|EthNetwork TestNetwork TunnelNet MIPv6Network HMIPv6Network Baseline|
`bash -c ". ~/bash/aliases;cd #{d} && opplink && cd -"`
end
