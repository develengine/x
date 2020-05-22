#!/usr/bin/env bash

g++ -Wall -o program main.cpp glad/src/gl.c -DENG_LINUX -Iglad/include -lGL -lX11 -ldl
