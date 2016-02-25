#!/usr/bin/perl

$host = "127.0.0.1";
$count = "3";

$ENV{'PATH'} = "/bin:/usr/bin:/sbin:/usr/sbin:" . $ENV{'PATH'};
if (`uname -s` eq "SunOS\n") {
    $cmd = "ping -s $host 56 $count";
} else {
    $cmd = "ping -c $count $host";
}
$out = `$cmd 2>&1`;
if ($? != 0) {
    &fail($out);
}

@result = split('\n', $out);
print "header 'Invoked from " . $context . ": " . $result[0] . "'\n";
for ($i = 0; $i < $count; $i++) {
    print "response __BEGIN data '" . $result[$i+1] . "' response __END\n";
}
$packets = $result[$#result-1];
$times = $result[$#result];
print "statistics __BEGIN\n";
print "packet '" . $packets . "' time '" . $times . "'\n";
print "statistics __END\n";

exit 0;

sub fail {
    chomp($msg = join(' ', @_));
    print $msg;
    exit 1;
}
