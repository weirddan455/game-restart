#!/bin/sh

gcc restart.c -o restart -O3 -Wall
sudo setcap cap_syslog+eip restart
