
.PHONY: build run

build: particles.cpp
	#g++ -O2 -ffast-math -Wall -Wextra -fno-rtti -fno-exceptions -S -o particles-main.asm particles.cpp -lopengl32 -lgdi32
	g++ -O2 -ffast-math -Wall -Wextra -fno-rtti -fno-exceptions -o particles-main particles.cpp -lopengl32 -lgdi32
	#g++ -Og -g -ffast-math -Wall -Wextra -fno-rtti -fno-exceptions -o particles-main particles.cpp -lopengl32 -lgdi32

build_fxvm: main.cpp fxvm.h fxreg.h
	g++ -Og -g -Wall -Wextra -fno-rtti -fno-exceptions -o fxvm-main main.cpp

run: particles-main.exe
	./particles-main

run_fxvm: fxvm-main.exe
	./fxvm-main

run_debug: particles-main.exe
	/usr/bin/gdb particles-main

run_debug_fxvm: fxvm-main.exe
	/usr/bin/gdb fxvm-main
