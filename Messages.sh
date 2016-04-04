#! /usr/bin/env bash

$EXTRACTRC --context="Category" --tag-group=none --tag=Name `find libdiscover -name "*-categories.xml"` >> categoriesxml.cpp
$XGETTEXT categoriesxml.cpp `find libdiscover -name \*.cpp` -o $podir/libdiscover.pot
$XGETTEXT `find discover -name \*.cpp -o -name \*.qml -o -name \*.js` -o $podir/plasma-discover.pot
$XGETTEXT `find exporter -name \*.cpp` -o $podir/plasma-discover-exporter.pot
$XGETTEXT `find notifier -name \*.cpp` -o $podir/plasma-discover-notifier.pot
rm -f muonrc.cpp
rm -f categoriesxml.cpp
