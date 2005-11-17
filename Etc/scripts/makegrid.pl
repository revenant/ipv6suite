#!/usr/bin/perl -w

$size = shift || 7;

for ($y=0; $y<$size; $y++) {
	for ($x=0; $x<$size; $x++) {
		$n = $y * $size + $x + 1;
		$nx = $n + 1;
		$ny = $n + $size;
		print "$n,$nx, " unless $x==$size-1;
		print "$n,$ny, " unless $y==$size-1;
	}
}
print $size*int($size/2) + int($size/2) + 1;
print ",0\n";

for ($y=0; $y<$size; $y++) {
	for ($x=0; $x<$size; $x++) {
		print "$x,$y, ";
	}
}
print "\n";

for ($y=0; $y<$size; $y++) {
	for ($x=0; $x<$size; $x++) {
		print ((($x&1) ^ ($y&1))?"1, ":"6, ");
	}
}
print "\n";
