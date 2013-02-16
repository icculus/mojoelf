#!/usr/bin/perl -w

# MojoELF; load ELF binaries from a memory buffer.
#
# Please see the file LICENSE.txt in the source's root directory.
#
#  This file written by Ryan C. Gordon.

# run this against the outout of something like this:
# cpp MostOfXlib.h |perl -pi -e 's/\#.*\Z//; s/\t/ /; s/\,//; s/\s+/ /g; s/\Aextern //; s/_X_SENTINEL\(\d+\)//; s/\;//; s/\A\s+//; s/\(//; s/\s+\Z//; s/ void\)\Z/\nvoid\n)/; if ($_ ne "") { $_ .= "\n"; }'

use warnings;
use strict;

my $script = $0;
$script =~ s/\A.*\///;

print <<__EOF__
/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file was autogenerated from $script. Do not edit by hand!
 */

// Do not #pragma once this file, it's intentionally included multiple times.

__EOF__
;

my @output_overrides;
my @output;

while (<>) {
    chomp;
    my ($rettype,$ptrs,$fn) = /\A(.*)\s+(\**)([a-zA-Z0-9_]*?)\Z/;
    $rettype .= $ptrs;
    my @p;
    while (<>) {
        chomp;
        last if ($_ eq ')');
        push @p, $_;
    }

    my $ret = ($rettype eq 'void') ? '' : 'return';
    my $args = '';
    my $params = '';
    my $isvarargs = 0;
    if ($p[0] eq 'void') {
        $params = 'void';
    } else {
        my $argcount = 0;
        foreach (@p) {
            my $thisp = $_;
            $thisp =~ s/\A(.*)\s+(\**)([a-zA-Z0-9_]*?)\Z/$1$2/;
            if ($argcount) {
                $args .= ',';
                $params .= ',';
            }
            my $arg = chr(ord('a') + $argcount);
            if ($thisp eq '...') {
                $isvarargs = 1;
                $params .= "$thisp";
            } else {
                $args .= $arg;
                # Hack for XQueryKeymap
                if ($thisp eq 'char [32]') {
                    $params .= "char $arg\[32\]";
                } elsif (/\(\*\)/) {
                    $thisp =~ s/\(\*\)/(\*$arg)/;
                    $params .= $thisp;
                } else {
                    $params .= "$thisp $arg";
                }
            }
            $argcount++;
        }
    }

    if ($isvarargs) {
        push @output_overrides, "MACTRAMPOLINE_OVERRIDE($rettype,$fn,($params))\n";
    } else {
        push @output, "MACTRAMPOLINE($rettype,$fn,($params),($args),$ret)\n";
    }
}

# !!! FIXME
print("#if 0  // !!! FIXME: write me; have to look these functions up.\n");
foreach (@output_overrides) {
    print $_;
}
print("#endif\n");

print("\n");

foreach (@output) {
    print $_;
}

print("\n// end of mactrampolines_x11.h ...\n\n");

# end of dumpx11fn.pl ...
