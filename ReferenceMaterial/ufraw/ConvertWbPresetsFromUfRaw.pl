#!/usr/bin/perl -w

open INPUT,"tmp_presets";

my $Line;
my $LastModel = "";
my $LastMake  = "";
while ($Line = <INPUT>) {
  if ($Line =~ m/^\s*{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"?(\w+)"?\s*,\s*(-?\d+)\s*,\s*({\s*[0-9\.]+\s*,\s*[0-9\.]+\s*,\s*[0-9\.]+\s*,\s*[0-9\.]+\s})\s*}\s*,/) {
  $Make = $1;
  $Model = $2;
  $BalanceName = $3;
  $Tuning = $4;
  $Presets = $5;
  if (($Model ne $LastModel) || ($Make ne $LastMake) ) {
    $LastModel = $Model;
    $LastMake  = $Make;
    print "\n// $Make , $Model\n";
  }
  next if $Tuning != 0;
  print "{ \"$Make\",\n  \"$Model\",\n  QObject::tr(\"$BalanceName\"),\n  $Presets },\n";
  }
}
