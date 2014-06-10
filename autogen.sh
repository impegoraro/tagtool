#!/bin/sh
# Run this to generate all the initial makefiles, etc.


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.ac) || {
    echo -n "*** Error: Directory "\`$srcdir\'" does not look like the"
    echo " project top-level directory"
    exit 1
}


OSTYPE=`uname -s`

echo "=== Running aclocal"
aclocal -I m4 || {
    echo "*** Error: aclocal failed."
    exit 1
}

echo "=== Running autoheader"
autoheader || {
    echo "*** Error: autoheader failed."
    exit 1
}

#echo "=== Running glib-gettextize"
#gettextize -f --no-changelog || {
#    echo "*** Error: gettextize failed."
#    exit 1
#}
#
#echo "=== Running intltoolize"
#intltoolize -f --automake || {
#    echo "*** Error: intltoolize failed."
#    exit 1
#}

echo "=== Running automake"
automake --add-missing || {
    echo "*** Error: automake failed."
    exit 1
}

echo "=== Running autoconf"
autoconf || {
    echo "*** Error: autoconf failed."
    exit 1
}


echo
echo "Now you are ready to run './configure'"
echo
