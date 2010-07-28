#! /usr/bin/env bash
$EXTRACTRC `find src/ -name \*.rc` >> rc.cpp
$XGETTEXT rc.cpp `find src/ -name \*.cpp` -o $podir/muon.pot
