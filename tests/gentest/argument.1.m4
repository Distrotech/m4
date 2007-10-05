#!/bin/sh

# gentest/argument.1.m4 is part of the GNU m4 testsuite
# generated from example in ../doc/m4.texinfo line 1036

. ${srcdir}/defs

cat <<\EOF >in
define(`exch', `$2, $1')
exch(arg1, arg2)
EOF

cat <<\EOF >ok

arg2, arg1
EOF

$M4 -d in >out

$CMP -s out ok

