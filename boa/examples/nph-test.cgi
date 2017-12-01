#!/usr/bin/perl

# Remember that CGI programs have to close out the HTTP header
# (with a pair of newlines), after giving the Content-type:
# and any other relevant or available header information.

# This test program always reports success (200 OK), and
# correctly uses SERVER_PROTOCOL and REQUEST_METHOD to decide
# whether or not to send the headers and content.

# Feb 3, 2000 -- updated to support POST, and avoid passing
# Malicious HTML Tags as described in CERT's CA-2000-02 advisory.

$now=`date`;
chomp($now);
if ($ENV{"SERVER_PROTOCOL"} ne "HTTP/0.9") {
 print "HTTP/1.0 200 OK\r\nDate: $now\r\n";
 print "Connection: close\r\n";
 print "Content-type: text/html; charset=ISO-8859-1\r\n\r\n";
}

exit 0 if ($ENV{"REQUEST_METHOD"} eq "HEAD");

print "<html><head><title>Boa nph-CGI test</title></head><body>\n";
print "<H2>Boa nph-CGI test</H2>\n\n";

print "Date: $now\n";

print "<P>\n\n<UL>\n";

foreach (keys %ENV) {
	$a= $ENV{$_};
	$a=~s/&/&amp;/g;
	$a=~s/</&lt;/g;
	$a=~s/>/&gt;/g;
	print "<LI>$_ == $a\n";
}

print "</UL>\n";

if ($ENV{REQUEST_METHOD} eq "POST") {
    print "Input stream:<br><hr><pre>\n";
    while (<stdin>) {
	s/&/&amp;/g;
	s/</&lt;/g;
	s/>/&gt;/g;
        print "$_";
    }
    print "</pre><hr>\n";
}

print "id: ", `id`, "\n<p>\n";

if ($ENV{"QUERY_STRING"}=~/ident/ && $ENV{"REMOTE_PORT"} ne "") {

# Uses idlookup-1.2 from Peter Eriksson  <pen@lysator.liu.se>
# ftp://coast.cs.purdue.edu/pub/tools/unix/ident/tools/idlookup-1.2.tar.gz
# Could use modification to timeout and trap stderr messages
	$a="idlookup ".
	   $ENV{"REMOTE_ADDR"}." ".$ENV{"REMOTE_PORT"}." ".$ENV{"SERVER_PORT"};
	$b=qx/$a/;
	print "ident output:<br><pre>\n$b</pre>\n";
}

print "\n<EM>Boa http server</EM>\n";
print "</body></html>\n";

exit 0;

