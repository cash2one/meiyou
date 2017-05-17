#!/bin/bash


[ -e simpic ] && rm -rf simpic
export PKG_CONFIG_PATH=/usr/lib/pkgconfig/:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH 
g++  simpic.cpp `pkg-config opencv --libs --cflags opencv` -o simpic
