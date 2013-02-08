#!/usr/bin/perl

# MojoELF; load ELF binaries from a memory buffer.
#
# Please see the file LICENSE.txt in the source's root directory.
#
#  This file written by Ryan C. Gordon.

my %fns;

while (<>) {
    my $rettype, $fname, $origargs;
    if (/^function *({([^}]*)}|([a-zA-Z0-9_]*)) [(][*]([a-zA-Z0-9_]*) ([(].*[)])/) {
        $rettype = "$2$3";
        $fname = "$4";
        $origargs = "$5";
    } elsif (/^function *({([^}]*)}|([a-zA-Z0-9_]*)) ([a-zA-Z0-9_]*) ([(].*[)])/) {
        $rettype = "$2$3";
        $fname = "$4";
        $origargs = "$5";
    }

    next if ($fns{$fname});  # already saw this one.
    $fns{$fname} = 1;

    my $ret = '';
    if ($rettype ne 'void') {
        $ret = 'return';
    }

    $origargs =~ s/\((.*)\)/$1/;

    my $args = '';
    my $params = '';
    if ($origargs eq 'void') {
        $params = 'void';
        $args = '';
    } else {
        my $count = 0;
        while ($origargs =~ s/\A(.*?),//) {
            my $str = $1;
            if ($count) {
                $args .= ',';
                $params .= ',';
            }
            my $arg = chr(ord('a') + $count);
            $args .= $arg;
            $params .= "$str $arg";
            $count++;
        }
    }

    my $str = $origargs;
    if ($count) {
        $args .= ',';
        $params .= ',';
    }
    my $arg = chr(ord('a') + $count);
    $args .= $arg;
    $params .= "$str $arg";

    print "MACTRAMPOLINE($rettype,$fname,($params),($args),$ret)\n";
}

