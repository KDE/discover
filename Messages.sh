#! /usr/bin/env bash
$EXTRACTRC `find src/ -name \*.rc` >> rc.cpp
$XGETTEXT `find src/ -name \*.cpp` -o $podir/muon.pot
rm -f rc.cpp
