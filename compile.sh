#!/usr/bin/env bash
useradd -r -d /var/lib/glassdm -c "glassdm greeter user" -s /usr/bin/nologin glassdm
mkdir -p /var/lib/glassdm
gcc src/main.c -lpam -o main -O2 -Wall
mv main /var/lib/glassdm
g++ src/greeter.cpp src/shader.cpp src/utils.cpp -lGL -lglfw -lfreetype -I /usr/include/freetype2 -o greeter -O2 -Wall
mv greeter /var/lib/glassdm
cp resources/* /var/lib/glassdm
cp configs/glassdm.service /etc/systemd/system
cp configs/glassdmgreeter /etc/pam.d
cp configs/glassdmlogin /etc/pam.d
systemctl daemon-reload
systemctl enable glassdm.service

