
.PHONY: build run

build: main.cpp fxvm.h fxreg.h
	g++ -Og -g -Wall -Wextra -fno-rtti -fno-exceptions -o fxvm-main main.cpp

run: fxvm-main.exe
	./fxvm-main

run_debug: fxvm-main.exe
	/usr/bin/gdb fxvm-main
