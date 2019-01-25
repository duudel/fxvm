
#include <GL/gl.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>

bool running = true;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
        case WM_KEYDOWN:
        {
            if (wparam == VK_ESCAPE)
            {
                running = false;
                return 0;
            }
        } break;
        case WM_CLOSE:
            running = false;
            return 0;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}

struct Window
{
    void *hwnd;
    void *hdc;
    void *glrc;
};

Window create_window(int width, int height)
{
    HMODULE instance = GetModuleHandle(nullptr);

    WNDCLASS wc      = { };
    wc.lpfnWndProc   = WindowProcedure;
    wc.hInstance     = instance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = "my-wnd-class";
    wc.style         = CS_OWNDC;
    if (!RegisterClass(&wc)) return {nullptr, nullptr, nullptr};

    DWORD ex_style = 0;
    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

    HWND hwnd = CreateWindowEx(ex_style, wc.lpszClassName, "Particles", style, 0, 0, width, height, nullptr, nullptr, instance, nullptr);

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    HDC hdc = GetDC(hwnd);

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixel_format, &pfd);

    HGLRC glrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, glrc);

    return {hwnd, hdc, glrc};
}

void set_window_title(Window w, const char *title)
{
    SetWindowText((HWND)w.hwnd, title);
}

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

vec3 operator + (vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3 operator - (vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3 operator * (vec3 a, vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
vec3 operator / (vec3 a, vec3 b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }
vec3 operator * (vec3 a, float t) { return {a.x * t, a.y * t, a.z * t}; }

struct Emitter
{
    float rate;
    float fractional_particles;
    float life;

    int last_index;
};

struct Particles
{
    enum { MAX = 1000 };
    vec3 position[MAX];
    vec3 velocity[MAX];
    vec3 acceleration[MAX];
    float life_seconds[MAX];
    float life_max[MAX];
    float size[MAX];
};

#include <cmath>

float random01()
{
    return (float)rand() / RAND_MAX;
}

void emit(Emitter *E, Particles *P, float dt)
{
    float num = E->rate * dt;
    float to_emit = num + E->fractional_particles;
    num = trunc(to_emit);
    E->fractional_particles = to_emit - num;
    int num_to_emit = (int)num;

    printf("num to emit %d, fractional_particles %f\n", num_to_emit, E->fractional_particles);

    int last_i = E->last_index;
    int i = last_i + 1;
    int search = 0;
    while (search < Particles::MAX && num_to_emit > 0)
    {
        int index = i % Particles::MAX;
        if (P->life_seconds[index] <= 0.0f)
        {
            P->position[index] = vec3{0.0f, 0.0f, 0.0f};
            P->velocity[index] = vec3{random01()-0.5f, 1.0f+random01()*0.5f, random01()-0.5f};
            P->acceleration[index] = vec3{0.0f, -1.0f, 0.0f};

            float initial_life = E->life + random01();
            P->life_seconds[index] = initial_life;
            P->life_max[index] = initial_life;
            P->size[index] = 0.01f;

            num_to_emit--;
        }
        i++;
        search++;
        last_i = index;
    }
    E->last_index = last_i;
}

void simulate(Emitter *E, Particles *P, float dt)
{
    emit(E, P, dt);
    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            P->life_seconds[i] -= dt;
            P->velocity[i] = P->velocity[i] + P->acceleration[i] * dt;
            P->position[i] = P->position[i] + P->velocity[i] * dt;
        }
    }
}

#include "fxvm.h"
struct Emitter_Parameters
{
    float rate;
    FXVM_Bytecode rate_bc;

    vec3 initial_velocity;
    FXVM_Bytecode initial_velocity_bc;

    float drag;
    FXVM_Bytecode drag_bc;

    vec3 initial_position;
    FXVM_Bytecode initial_position_bc;
};

struct Particle_System
{
    Emitter_Parameters emitter;

    FXVM_Bytecode size;
};

void emit(Particle_System *PS, Emitter *E, Particles *P, float dt)
{
    float num = PS->emitter.rate * dt;
    float to_emit = num + E->fractional_particles;
    num = trunc(to_emit);
    E->fractional_particles = to_emit - num;
    int num_to_emit = (int)num;

    //printf("num to emit %d, fractional_particles %f\n", num_to_emit, E->fractional_particles);

    int last_i = E->last_index;
    int i = last_i + 1;
    int search = 0;
    while (search < Particles::MAX && num_to_emit > 0)
    {
        int index = i % Particles::MAX;
        if (P->life_seconds[index] <= 0.0f)
        {
            P->position[index] = vec3{0.0f, 0.0f, 0.0f};
            P->velocity[index] = vec3{random01()-0.5f, 1.0f+random01()*0.5f, random01()-0.5f};
            P->acceleration[index] = vec3{0.0f, -1.0f, 0.0f};

            float initial_life = E->life + random01();
            P->life_seconds[index] = initial_life;
            P->life_max[index] = initial_life;
            P->size[index] = 0.01f;

            num_to_emit--;
        }
        i++;
        search++;
        last_i = index;
    }
    E->last_index = last_i;
}

float eval_f1(float *input, FXVM_Bytecode bytecode)
{
    FXVM_State state = { };
    exec(state, (uint8_t*)input, &bytecode);
    return state.r[0].v[0];
}

void simulate(Particle_System *PS, Emitter *E, Particles *P, float dt)
{
    emit(PS, E, P, dt);
    float global_input[16];
    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            global_input[0] = P->life_seconds[i];
            P->life_seconds[i] -= dt;
            P->velocity[i] = P->velocity[i] + P->acceleration[i] * dt;
            P->position[i] = P->position[i] + P->velocity[i] * dt;
            P->size[i] = eval_f1(global_input, PS->size);
        }
    }
}

void draw_particle(Particles *P, int i)
{
    float r = (i & 1) ? 1.0f : 1.0f;
    float g = (i & 3) ? 1.0f : 0.0f;
    float fade = P->life_seconds[i] / P->life_max[i];
    //float size = 0.002f + (1.0 - fade) * 0.05f;
    float size = P->size[i];
    float x = P->position[i].x;
    float y = P->position[i].y;
    glColor4f(1.0f * fade * r, 1.0f * fade * g, 0.0f, fade);
    glVertex3f(x - size, y, 0.0f);
    glVertex3f(x, y + size, 0.0f);
    glVertex3f(x + size, y, 0.0f);
}

void draw(Particles *P)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            draw_particle(P, i);
        }
    }
    glEnd();
}

#include "fxcomp.h"

void report_compile_error(const char *err) { printf("Error: %s\n", err); }

FXVM_Bytecode compile(const char *source)
{
    FXVM_Compiler compiler = { };
    compiler.report_error = report_compile_error;

    register_input_variable(&compiler, "particle_life", FXTYP_F1);

    compile(&compiler, source, source + strlen(source));
    return { compiler.codegen.buffer_len, compiler.codegen.buffer };
}

#define SOURCE(x) #x

Particle_System load_psys()
{
    Particle_System result = { };
    result.emitter.rate = 20.0f;
    result.emitter.initial_velocity = vec3{0.0f, 1.0f, 0.0f};
    result.size = compile(
SOURCE(
    0.02 * sin(particle_life * 4.0);
));
    return result;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    Window window = create_window(1000, 800);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    Particles P = { };
    Emitter E = { };
    E.rate = 30.0f;
    E.life = 1.0f;

    Particle_System psys = load_psys();

    float time_accum = 0.0f;

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    float sim_dt = 0.01666f;

    float sim_time = 0.0f;

    MSG msg = { };
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }

        LARGE_INTEGER last_counter = counter;
        QueryPerformanceCounter(&counter);
        LONGLONG tick_count = counter.QuadPart - last_counter.QuadPart;
        float dt = (float)tick_count / (float)freq.QuadPart;
        time_accum += dt;

        if (time_accum > 0.1f) time_accum = 0.1f;

        //printf("ticks %f\n", dt); fflush(stdout);
        while (time_accum >= sim_dt)
        {
            //simulate(&E, &P, sim_dt);
            LARGE_INTEGER start_counter;
            QueryPerformanceFrequency(&start_counter);

            simulate(&psys, &E, &P, sim_dt);

            LARGE_INTEGER end_counter;
            QueryPerformanceFrequency(&end_counter);
            LONGLONG ticks = end_counter.QuadPart - start_counter.QuadPart;
            sim_time = sim_time * 0.95f + ((float)ticks / (float)freq.QuadPart) * 0.05f;

            time_accum -= sim_dt;
        }
        draw(&P);

        float fps = 1.0f / dt;

        char buf[64];
        snprintf(buf, 64, "FPS %.3f; SIM %.6f", fps, sim_time);
        set_window_title(window, buf);

        SwapBuffers((HDC)window.hdc);
    }
    wglMakeCurrent((HDC)window.hdc, nullptr);
    wglDeleteContext((HGLRC)window.glrc);
    DestroyWindow((HWND)window.hwnd);
    return 0;
}

