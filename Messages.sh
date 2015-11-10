#! /usr/bin/env bash

$EXTRACTRC `find updater/ -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> updaterrc.cpp
$EXTRACTRC --context="Category" --tag-group=none --tag=Name `find libdiscover -name "*-categories.xml"` >> categoriesxml.cpp
$XGETTEXT categoriesxml.cpp `find libdiscover -name \*.cpp` -o $podir/libdiscover.pot
$XGETTEXT rc.cpp updaterrc.cpp `find updater -name \*.cpp` -o $podir/muon-updater.pot
$XGETTEXT `find discover -name \*.cpp -o -name \*.qml -o -name \*.js` -o $podir/muon-discover.pot
$XGETTEXT `find exporter -name \*.cpp` -o $podir/muon-exporter.pot
$XGETTEXT `find notifier -name \*.cpp` -o $podir/muon-notifier.pot
rm -f muonrc.cpp
rm -f updaterrc.cpp
rm -f categoriesxml.cpp
