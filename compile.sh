#!/bin/bash
mkdir -p testing
gcc -lpipewire-0.3 -I/usr/include/pipewire-0.3/ -I/usr/include/spa-0.2/ -g -o testing/pwmixer src/*.c
