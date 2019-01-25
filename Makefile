
.PHONY: build run

build: particles.cpp
	g++ -Og -g -Wall -Wextra -fno-rtti -fno-exceptions -o particles-main particles.cpp -lopengl32 -lgdi32

build_fxvm: main.cpp fxvm.h fxreg.h
	g++ -Og -g -Wall -Wextra -fno-rtti -fno-exceptions -o fxvm-main main.cpp

run: particles-main.exe
	./particles-main

run_fxvm: fxvm-main.exe
	./fxvm-main

run_debug: fxvm-main.exe
	/usr/bin/gdb fxvm-main
