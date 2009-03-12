#!/bin/sh

set -x
libtoolize --automake
aclocal
autoconf
automake --add-missing --foreign
