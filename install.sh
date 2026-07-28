#!/usr/bin/env bash
useradd -r -d /var/lib/lgdm -c "lgdm greeter user" -s /usr/bin/nologin lgdm
mkdir -p /var/lib/lgdm
gcc dm/main.c -lpam -o main -O2 -Wall
mv main /var/lib/lgdm
gcc dm/greeter.c dm/shader.c dm/utils.c dm/readcfg.c -lGL -lglfw -lfreetype -lm -I /usr/include/freetype2 -o greeter -O2 -Wall
mv greeter /var/lib/lgdm
cp resources/* /var/lib/lgdm
cp configs/lgdm.service /etc/systemd/system
cp configs/lgdmgreeter /etc/pam.d
cp configs/lgdmlogin /etc/pam.d
cp configs/config.cfg /var/lib/lgdm/config.cfg
systemctl daemon-reload
systemctl enable lgdm.service

cd setting
mkdir -p build
yes|rm -rf build/*
cd build
cmake ..
make -j$(nproc)
cp kcms/libkcm_lgdm.so /usr/lib/qt6/plugins/plasma/kcms/systemsettings/kcm_lgdm.so

