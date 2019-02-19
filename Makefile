
.PHONY: build run

SOURCES := particles.cpp
IMGUI_SOURCES := imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/examples/imgui_impl_opengl2.cpp
IMGUI_OBJECTS := imgui.o imgui_draw.o imgui_widgets.o imgui_impl_opengl2.o

build: particles.cpp libimgui.a
	#g++ -O2 -ffast-math -Wall -Wextra -fno-rtti -fno-exceptions -S -o particles-main.asm particles.cpp -lopengl32 -lgdi32
	#g++ -O2 -ffast-math -Wall -Wextra -fno-rtti -fno-exceptions -o particles-main particles.cpp -lopengl32 -lgdi32 -lFreeImage
	g++ -Og -g -ffast-math -Wall -Wextra -fno-rtti -fno-exceptions -Iimgui -L. -o particles-main $(SOURCES) -limgui -lopengl32 -lgdi32 -lFreeImage

build_fxvm: main.cpp fxvm.h fxreg.h
	g++ -Og -g -Wall -Wextra -fno-rtti -fno-exceptions -o fxvm-main main.cpp

libimgui.a: $(IMGUI_OBJECTS)
	ar rcs libimgui.a $(IMGUI_OBJECTS)

COMPILE_IM := g++ -Og -Wall -Wextra -fno-rtti -fno-exceptions -Iimgui -c

imgui.o: imgui/imgui.cpp
	$(COMPILE_IM) imgui/imgui.cpp

imgui_draw.o: imgui/imgui_draw.cpp
	$(COMPILE_IM) imgui/imgui_draw.cpp

imgui_widgets.o: imgui/imgui_widgets.cpp
	$(COMPILE_IM) imgui/imgui_widgets.cpp

imgui_impl_opengl2.o: imgui/examples/imgui_impl_opengl2.cpp
	$(COMPILE_IM) imgui/examples/imgui_impl_opengl2.cpp

run: particles-main.exe
	./particles-main

run_fxvm: fxvm-main.exe
	./fxvm-main

run_debug: particles-main.exe
	/usr/bin/gdb particles-main

run_debug_fxvm: fxvm-main.exe
	/usr/bin/gdb fxvm-main
