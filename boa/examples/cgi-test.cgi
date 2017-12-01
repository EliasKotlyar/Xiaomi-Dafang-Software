#! /usr/bin/perl -wT

# Remember that CGI programs have to close out the HTTP header
# (with a pair of newlines), after giving the Content-type:
# and any other relevant or available header information.

# Unlike CGI programs running under Apache, CGI programs under Boa
# should understand some simple HTTP options.  The header (and the
# double-newline) should not be printed if the incoming request was
# in HTTP/0.9.  Also, we should stop after the header if
# REQUEST_METHOD == "HEAD".  Under Apache, nph- programs also have
# to worry about such stuff.

# Feb 3, 2000 -- updated to support POST, and avoid passing
# Malicious HTML Tags as described in CERT's CA-2000-02 advisory.
#
# 20 Aug 2002 -- Big internal changes, to support much more
# than just a printout of the environment. Now the CGI can
# do various, GET, isindex, and POST requests, and respond
# to them as well.

# 26 Sep 2002 -- Additional security paranoia by Landon Curt Noll
# http://www.isthe.com/chongo/index.html

# paranoia
#
delete $ENV{IFS};
delete $ENV{CDPATH};
delete $ENV{ENV};
delete $ENV{BASH_ENV};
#$ENV{PATH} = "/bin:/usr/bin";
$SIG{ALRM} = sub { die "</pre>\n<p>timeout on stdin<p></body></html>\n"; };
alarm(3);

# initial setup
#
use strict;
use POSIX qw(strftime getegid);

# Print Content-type, if allowed
#
if (defined $ENV{"SERVER_PROTOCOL"} &&
    $ENV{"SERVER_PROTOCOL"} !~ m{HTTP/0.9}i) {
    print "Content-type: text/html; charset=ISO-8859-1\r\n\r\n";
}

# Nothing to do if just a HEAD request
#
if (defined $ENV{"REQUEST_METHOD"} && $ENV{"REQUEST_METHOD"} =~ /^HEAD$/i) {
   exit 0;
}

# Initial HTML lines
#
print "<html><head><title>Boa CGI test</title></head><body>\n";
print "<H2>Boa CGI test</H2>\n\n";
print "Date: ", strftime("%a %b %e %H:%M:%S %Y\n", localtime);
print "<p>\n";

# Main form code
#
if (defined $ENV{"REQUEST_METHOD"}) {
    print "Method: $ENV{\"REQUEST_METHOD\"}\n";
} else {
    print "Method: <<undefined>>\n";
}
print "<p>\n";

print "<table border=1>\n";
print "<tr><td>Basic GET Form:<br>";
print " <form method=\"get\">\n\
  <input type=\"text\" name=\"parameter_1\" size=5 maxlength=5>\
    <select name=\"select_1\">\
      <option>foo</option>\
      <option>bar</option>\
    </select>\
  <input type=\"submit\" NAME=SUBMIT VALUE=\"Submit\">\
 </form>";
print "</td>";
print "<td>Basic POST Form:<br>";
print "<form method=\"post\">\n\
  <input type=\"text\" name=\"parameter_1\" size=5 maxlength=5>\
    <select name=\"select_1\">\
      <option>foo</option>\
      <option>bar</option>\
    </select>\
  <input type=\"submit\" NAME=SUBMIT VALUE=\"Submit\">\
  </form>";
print "</td>";
print "</tr>\n";
print "<tr><td colspan=2>Sample ISINDEX form:<br>\n";
if (defined $ENV{"SCRIPT_NAME"}) {
    print "<a href=\"$ENV{\"SCRIPT_NAME\"}?param1+param2+param3\">$ENV{\"SCRIPT_NAME\"}?param1+param2+param3</a>\n";
} else {
    print "undefined SCRIPT_NAME\n";
}
print "</td></tr>";
print "</table>\n";

if (defined $ENV{"QUERY_STRING"}) {
    print "<p>Query String: $ENV{\"QUERY_STRING\"}\n";
} else {
    print "<p>Query String: undefined QUERY_STRING\n";
}

# Print the arguments
#
print "<p>\nArguments:\n<ol>\n";
if ($#ARGV >= 0) {
    while ($a=shift(@ARGV)) {
        $a=~s/&/&amp;/g;
        $a=~s/</&lt;/g;
        $a=~s/>/&gt;/g;
        print "<li>$a\n";
   }
}
print "</ol>\n";

# Print environment list
#
print "<P>\nEnvironment:\n<UL>\n";
foreach my $i (keys %ENV) {
    $a=$ENV{$i};
    $a=~s/&/&amp;/g;
    $a=~s/</&lt;/g;
    $a=~s/>/&gt;/g;
    $i=~s/&/&amp;/g;
    $i=~s/</&lt;/g;
    $i=~s/>/&gt;/g;
    print "<li>$i = $a\n";
}
print "</UL>\n";

# Print posted data, if any
#
my $line_cnt = 0;
my $line;
if (defined $ENV{REQUEST_METHOD} &&
    $ENV{REQUEST_METHOD} =~ /POST/i) {
    print "Input stream:<br><hr>\n";
    while (defined($line = <stdin>)) {
    	if (++$line_cnt > 100) {
	    print "<p>... ignoring the rest of the input data<p>";
	    last;
	}
	$line =~ s/&/&amp;/g;
	$line =~ s/</&lt;/g;
	$line =~ s/>/&gt;/g;
	print "<pre>" if $line_cnt == 1;
        print "$line";
    }
    print "</pre>" if $line_cnt > 0;
    print "<hr>\n";
} else {
    print "No input stream: (not POST)<p>\n";
}

# Print a little additional server information
#
print "uid: $>  gid: ", getegid(), "\n<p>\n";

# Disabled use of this call due to DoS attack potential
#
#if (defined $ENV{"QUERY_STRING"} && defined $ENV{"REMOTE_PORT"} &&
#    $ENV{"QUERY_STRING"} =~ /ident/i && $ENV{"REMOTE_PORT"} =~ /^\s*$/) {
#
## Uses idlookup-1.2 from Peter Eriksson  <pen at lysator dot liu dot se>
## ftp://coast.cs.purdue.edu/pub/tools/unix/ident/tools/idlookup-1.2.tar.gz
## Could use modification to timeout and trap stderr messages
#	my $a="idlookup ".
#	   $ENV{"REMOTE_ADDR"}." ".$ENV{"REMOTE_PORT"}." ".$ENV{"SERVER_PORT"};
#	my $b=qx/$a/;
#	print "ident output:<br><pre>\n$b</pre>\n";
#}

# End of HTML
#
print "\n<EM>Boa http server</EM>\n";
print "</body></html>\n";

# All done!  :-)
#
exit 0;

