#!/usr/bin/perl

# IP address resolver for Boa

# If you want an "in place" change to the log file,
# change the first line to
#!/usr/local/bin/perl -i.bak
# Otherwise, send the output of this program wherever you want:
#  resolver.pl access_log >access_log_resolved

$AF_INET = 2;

while(<>) {
    next unless (($ip, $rest) = /([\d\.]+) (.*)/o);

    if(!$hosts{$ip}) {
        $packed_ip = pack('C4', split(/\./, $ip));
        $host = (gethostbyaddr($packed_ip, $AF_INET))[0];
        $hosts{$ip} = ($host ? $host : $ip);
    }

    print "$hosts{$ip} $rest\n";
}

