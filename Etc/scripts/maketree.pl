#!/usr/bin/perl -w

$seed = shift || scalar time();
$nnodes = shift || 50;
$length = shift || sqrt($nnodes);
$minrange = shift || 0.8;
$maxrange = shift || 1.2;
$chanrange = shift || $maxrange;

srand($seed);

@coords = ([$length/2,$length/2]);
foreach $x (1 .. $nnodes-1) {

	CHOOSE: while (!keys %{$tree[$x]}) {
		$coords[$x] = [ rand()*$length, rand()*$length ];
		$chan[$x] = int(rand()*3)*5+1;
		$tree[$x] = {};

		foreach $y (0 .. $x-1) {
			$dx = $coords[$x][0] - $coords[$y][0];
			$dy = $coords[$x][1] - $coords[$y][1];
			$dd = sqrt($dx*$dx + $dy*$dy);
			
			if ($y > 0 && (
				($dd < $minrange) ||
				($dd < $chanrange && $chan[$x] == $chan[$y])
			)) {
				$tree[$x] = {};
				next CHOOSE;
			}
			if ($dd <= $maxrange) {
				$tree[$x]{$y} = 1;
			}
		} 
	}
	print STDERR "PLACED $x ($coords[$x][0],$coords[$x][1])\n";
}

foreach $x (1 .. $nnodes-1) {
	$y = (sort {$a <=> $b} keys %{$tree[$x]})[0];
	print STDERR "$x: $y\n";
	print "$x,$y, ";
}
print "\n";

foreach $x (1 .. $nnodes-1) {
	printf "%.3f,%.3f, ", @{$coords[$x]};
}
print "\n";

print join ',', @chan[1 .. $nnodes-1];
print "\n";
