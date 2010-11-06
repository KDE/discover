#! /usr/bin/env bash
$EXTRACTRC `find muon/ -name \*.rc` >> muonrc.cpp
$EXTRACTRC `find updater/ -name \*.rc` >> updaterrc.cpp
$EXTRACTRC `find kded/ -name \*.rc` >> notifierrc.cpp
$XGETTEXT `find libmuon -name \*.cpp` -o $podir/libmuon.pot
$XGETTEXT muonrc.cpp `find muon -name \*.cpp` -o $podir/muon.pot
$XGETTEXT updaterrc.cpp `find updater -name \*.cpp` -o $podir/muon-updater.pot
$XGETTEXT `find installer -name \*.cpp` -o $podir/muon-installer.pot
$XGETTEXT notifierrc.cpp `find kded/ -name \*.cpp` -o $podir/muon-notifier.pot
rm -f rc.cpp
rm -f updaterrc.cpp
