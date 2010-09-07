#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$XGETTEXT rc.cpp `find . -name \*.cpp` -o $podir/muon.pot
rm -f rc.cpp
