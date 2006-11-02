for d in %w|EthNetwork PingNetwork TestNetwork TunnelNet MIPv6Network HMIPv6Network|
`bash -c ". ~/bash/aliases;cd #{d} && opplink && cd -"`
end
