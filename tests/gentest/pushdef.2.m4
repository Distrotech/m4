#!/bin/sh

# gentest/pushdef.2.m4 is part of the GNU m4 testsuite
# generated from example in ../doc/m4.texinfo line 1318

. ${srcdir}/defs

cat <<\EOF >in
define(`foo', `Expansion one.')
foo
pushdef(`foo', `Expansion two.')
foo
define(`foo', `Second expansion two.')
foo
undefine(`foo')
foo
EOF

cat <<\EOF >ok

Expansion one.

Expansion two.

Second expansion two.

foo
EOF

$M4 -d in >out

$CMP -s out ok

