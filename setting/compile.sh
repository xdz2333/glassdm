#!/usr/bin/env bash
mkdir -p build
yes|rm -rf build/*
cd build
cmake ..
make -j$(nproc)
cp kcms/libkcm_lgdm.so /usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_lgdm.so
systemsettings kcm_glassdm
