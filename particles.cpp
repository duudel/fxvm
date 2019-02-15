
#include <GL/gl.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

#include <cstdint>
#include <cstdio>

struct Window
{
    void *hwnd;
    void *hdc;
    void *glrc;

    bool running;
    int width;
    int height;

    int mouse_x;
    int mouse_y;
    int buttons;

    int last_mouse_x;
    int last_mouse_y;

    void *wheel_user_ptr;
    void (*mouse_wheel)(float wheel, void *user_ptr);

    void *key_down_user_ptr;
    void (*key_down)(int key, void *user_ptr);
    void *key_up_user_ptr;
    void (*key_up)(int key, void *user_ptr);
};

template <class T>
void set_window_key_down(Window *w, void (*fn)(int, T*), T *user_ptr)
{
    w->key_down = (void(*)(int, void*))fn;
    w->key_down_user_ptr = user_ptr;
}

template <class T>
void set_window_key_up(Window *w, void (*fn)(int, T*), T *user_ptr)
{
    w->key_up = (void(*)(int, void*))fn;
    w->key_up_user_ptr = user_ptr;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    Window *window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message)
    {
    case WM_NCCREATE:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ((CREATESTRUCT*)lparam)->lpCreateParams);
            SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        } break;
    case WM_KEYDOWN:
        {
            if (wparam == VK_ESCAPE)
            {
                window->running = false;
                return 0;
            }
            else
            {
                int key = wparam;
                if (window->key_down) window->key_down(key, window->key_down_user_ptr);
            }
        } break;
    case WM_KEYUP:
        {
            int key = wparam;
            if (window->key_up) window->key_up(key, window->key_up_user_ptr);
        } break;
    case WM_MOUSEWHEEL:
        {
            int16_t wheel_w = HIWORD(wparam);
            float wheel = (float)wheel_w / WHEEL_DELTA;

            if (window->mouse_wheel) window->mouse_wheel(wheel, window->wheel_user_ptr);
        } break;
    case WM_MOUSEMOVE:
        {
            int mx = LOWORD(lparam);
            int my = HIWORD(lparam);
            int buttons = wparam;

            //window->last_mouse_x = window->mouse_x;
            //window->last_mouse_y = window->mouse_y;

            window->mouse_x = mx;
            window->mouse_y = my;
            window->buttons = buttons;
        } break;
    case WM_SIZE:
        {
            int width = LOWORD(lparam);
            int height = HIWORD(lparam);
            glViewport(0, 0, width, height);

            window->width = width;
            window->height = height;
        } break;
    case WM_CLOSE:
        window->running = false;
        return 0;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}

bool create_window(int width, int height, Window *window)
{
    HMODULE instance = GetModuleHandle(nullptr);

    WNDCLASS wc      = { };
    wc.lpfnWndProc   = WindowProcedure;
    wc.hInstance     = instance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = "my-wnd-class";
    wc.style         = CS_OWNDC;
    if (!RegisterClass(&wc)) return false;

    DWORD ex_style = 0;
    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

    HWND hwnd = CreateWindowEx(ex_style, wc.lpszClassName, "Particles", style, 0, 0, width, height, nullptr, nullptr, instance, window);

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

    SetWindowPos(hwnd, HWND_NOTOPMOST,0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);

    window->hwnd = hwnd;
    window->hdc = hdc;
    window->glrc = glrc;
    window->width = width;
    window->height = height;
    window->running = true;
    return true;
}

void set_window_title(Window w, const char *title)
{
    SetWindowText((HWND)w.hwnd, title);
}


#include <cmath>

//#define TRACE_FXVM
#define FXVM_IMPL
#include "fxvm.h"

float random01()
{
    return (float)rand() / RAND_MAX;
}

float clamp01(float x)
{
    return (x > 1.0f) ? 1.0f : ((x < 0.0f) ? 0.0f : x);
}

struct vec3
{
#ifdef USE_SSE
    union
    {
        struct { float x, y, z; };
        __m128 v4;
    };
#else
    float x, y, z;
#endif
};

struct vec4
{
#ifdef USE_SSE
    union
    {
        struct { float x, y, z, w; };
        __m128 v4;
    };
#else
    float x, y, z, w;
#endif
};

#ifndef USE_SSE
vec3 operator - (vec3 a) { return {-a.x, -a.y, -a.z}; }
vec3 operator + (vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3 operator - (vec3 a, vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3 operator * (vec3 a, vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
vec3 operator / (vec3 a, vec3 b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }
vec3 operator * (vec3 a, float t) { return {a.x * t, a.y * t, a.z * t}; }
vec3 operator / (vec3 a, float t) { return {a.x / t, a.y / t, a.z / t}; }

float dot(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 cross(vec3 a, vec3 b)
{
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

vec3 normalize(vec3 a)
{
    float L2 = dot(a, a);
    float L = sqrt(L2);
    return a / L;
}

vec4 operator - (vec4 a) { return {-a.x, -a.y, -a.z, -a.w}; }
vec4 operator + (vec4 a, vec4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
vec4 operator - (vec4 a, vec4 b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
vec4 operator * (vec4 a, vec4 b) { return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }
vec4 operator / (vec4 a, vec4 b) { return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }
vec4 operator * (vec4 a, float t) { return {a.x * t, a.y * t, a.z * t, a.w * t}; }
vec4 operator / (vec4 a, float t) { return {a.x / t, a.y / t, a.z / t, a.w / t}; }

float dot(vec4 a, vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

vec4 normalize(vec4 a)
{
    float L2 = dot(a, a);
    float L = sqrt(L2);
    return a / L;
}

#else
// USE_SSE

vec3 operator - (vec3 a) { return vec3{ .v4 = _mm_xor_ps(a.v4, _mm_set1_ps(-0.0f)) }; }
vec3 operator + (vec3 a, vec3 b) { return vec3{ .v4 = _mm_add_ps(a.v4, b.v4) }; }
vec3 operator - (vec3 a, vec3 b) { return vec3{ .v4 = _mm_sub_ps(a.v4, b.v4) }; }
vec3 operator * (vec3 a, vec3 b) { return vec3{ .v4 = _mm_mul_ps(a.v4, b.v4) }; }
vec3 operator / (vec3 a, vec3 b) { return vec3{ .v4 = _mm_div_ps(a.v4, b.v4) }; }
vec3 operator * (vec3 a, float t) { return vec3{ .v4 = _mm_mul_ps(a.v4, _mm_set1_ps(t)) }; }
vec3 operator / (vec3 a, float t) { return vec3{ .v4 = _mm_div_ps(a.v4, _mm_set1_ps(t)) }; }

float dot(vec3 a, vec3 b)
{
    //vec3 r = vec3{ .v4 = _mm_mul_ps(a.v4, b.v4) };
    //return r.x + r.y + r.z;
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 cross(vec3 a, vec3 b)
{
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

vec3 normalize(vec3 a)
{
    float L2 = dot(a, a);
    __m128 L = _mm_rsqrt_ps(_mm_set1_ps(L2));
    return vec3{ .v4 = _mm_mul_ps(a.v4, L) };
}

vec3 lerp(vec3 a, vec3 b, float t)
{
    return a * (1.0f - t) + b * t;
}

vec4 operator - (vec4 a) { return vec4{ .v4 = _mm_xor_ps(a.v4, _mm_set1_ps(-0.0f)) }; }
vec4 operator + (vec4 a, vec4 b) { return vec4{ .v4 = _mm_add_ps(a.v4, b.v4) }; }
vec4 operator - (vec4 a, vec4 b) { return vec4{ .v4 = _mm_sub_ps(a.v4, b.v4) }; }
vec4 operator * (vec4 a, vec4 b) { return vec4{ .v4 = _mm_mul_ps(a.v4, b.v4) }; }
vec4 operator / (vec4 a, vec4 b) { return vec4{ .v4 = _mm_div_ps(a.v4, b.v4) }; }
vec4 operator * (vec4 a, float t) { return vec4{ .v4 = _mm_mul_ps(a.v4, _mm_set1_ps(t)) }; }
vec4 operator / (vec4 a, float t) { return vec4{ .v4 = _mm_div_ps(a.v4, _mm_set1_ps(t)) }; }

float dot(vec4 a, vec4 b)
{
    vec4 r = vec4{ .v4 = _mm_mul_ps(a.v4, b.v4) };
    return r.x + r.y + r.z + r.w;
}

vec4 normalize(vec4 a)
{
    float L2 = dot(a, a);
    __m128 L = _mm_rsqrt_ps(_mm_set1_ps(L2));
    return vec4{ .v4 = _mm_mul_ps(a.v4, L) };
}
#endif

struct mat4
{
    float m[16];

    float operator[](int i) const { return m[i]; }
};

mat4 transpose(mat4 m)
{
    return {
        m[0], m[4],  m[8], m[12],
        m[1], m[5],  m[9], m[13],
        m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15]
    };
}

mat4 rotation_y(float theta)
{
    float s = sin(theta);
    float c = cos(theta);
    return {
        c,  0, -s, 0,
        0,  1,  0, 0,
        s,  0,  c, 0,
        0,  0,  0, 1
    };
}

mat4 rotation_x(float theta)
{
    float s = sin(theta);
    float c = cos(theta);
    return {
        1,  0,  0, 0,
        0,  c,  s, 0,
        0, -s,  c, 0,
        0,  0,  0, 1
    };
}

mat4 translation(vec3 v)
{
    return {
        1, 0, 0, v.x,
        0, 1, 0, v.y,
        0, 0, 1, v.z,
        0, 0, 0,   1
    };
}

mat4 operator * (mat4 a, mat4 b)
{
    // Rows
    vec4 a0 = vec4{a[0],  a[1],  a[2],  a[3]};
    vec4 a1 = vec4{a[4],  a[5],  a[6],  a[7]};
    vec4 a2 = vec4{a[8],  a[9],  a[10], a[11]};
    vec4 a3 = vec4{a[12], a[13], a[14], a[15]};
    // Columns
    vec4 b0 = vec4{b[0], b[4], b[8],  b[12]};
    vec4 b1 = vec4{b[1], b[5], b[9],  b[13]};
    vec4 b2 = vec4{b[2], b[6], b[10], b[14]};
    vec4 b3 = vec4{b[3], b[7], b[11], b[15]};
    return {
        dot(a0, b0), dot(a0, b1), dot(a0, b2), dot(a0, b3),
        dot(a1, b0), dot(a1, b1), dot(a1, b2), dot(a1, b3),
        dot(a2, b0), dot(a2, b1), dot(a2, b2), dot(a2, b3),
        dot(a3, b0), dot(a3, b1), dot(a3, b2), dot(a3, b3),
    };
}

mat4 PerspectiveOffCenter_lh(float left, float right,
                             float bottom, float top,
                             float nearZ, float farZ)
{
    const float X = (2.0f * nearZ) / (right - left);
    const float Y = (2.0f * nearZ) / (top - bottom);

    const float A = (right + left) / (right - left);
    const float B = (top + bottom) / (top - bottom);
    const float C = -(farZ + nearZ) / (farZ - nearZ);
    const float D = (-2.0f * farZ * nearZ) / (farZ - nearZ);

    return mat4{
        X,    0.0f, A,    0.0f,
        0.0f, Y,    B,    0.0f,
        0.0f, 0.0f, C,    D,
        0.0f, 0.0f, -1.0f, 0.0f
    };
}

mat4 Perspective_lh(float fov_y, float aspect_ratio, float near, float far)
{
    const float deg2rad = 3.14159265358f / 180.0f;
    const float tan_half_fov_y = std::tan(fov_y * 0.5f * deg2rad);
    const float y = near * tan_half_fov_y;
    const float x = y * aspect_ratio;
    return PerspectiveOffCenter_lh(-x, x, -y, y, near, far);
}

vec4 transform(mat4 tr, vec4 p)
{
    // | 0  1  2  3|   |x|
    // | 4  5  6  7| x |y| = |0x+1y+2z+3w, 4x+5y+6z+7w, 8x+9y+10z+11w, 12x+13y+14z+15w|
    // | 8  9 10 11|   |z|
    // |12 13 14 15|   |w|
    float x =  tr[0] * p.x +  tr[1] * p.y +  tr[2] * p.z +  tr[3] * p.w;
    float y =  tr[4] * p.x +  tr[5] * p.y +  tr[6] * p.z +  tr[7] * p.w;
    float z =  tr[8] * p.x +  tr[9] * p.y + tr[10] * p.z + tr[11] * p.w;
    float w = tr[12] * p.x + tr[13] * p.y + tr[14] * p.z + tr[15] * p.w;
    return {x, y, z, w};
}

struct Particles
{
    enum { MAX = 1000 };
    vec3 position[MAX];
    vec3 velocity[MAX];
    vec3 acceleration[MAX];
    float life_seconds[MAX];
    float life_max[MAX];
    float life_01[MAX];
    float size[MAX];
    vec4 color[MAX];
    vec4 random[MAX];
};


struct Emitter_Instance
{
    float fractional_particles;
    float life;
    vec3 position;

    int last_index;

    int particles_alive;
    Particles P[2];

    int frame;
};

struct Emitter_Parameters
{
    float life_max;
    bool loop;

    float rate;
    FXVM_Program rate_p;

    vec3 acceleration;

    float initial_life;
    FXVM_Program initial_life_p;

    vec3 initial_position;
    FXVM_Program initial_position_p;

    vec3 initial_velocity;
    FXVM_Program initial_velocity_p;

    float drag;
    FXVM_Program drag_p;

    int life_i;
    int random_i;
};

struct Particle_System
{
    Emitter_Parameters emitter;
    bool stretch;
    bool additive;

    vec3 acceleration;
    FXVM_Program acceleration_p;

    vec4 color;
    FXVM_Program color_p;

    float size;
    FXVM_Program size_p;

    int attrib_life;
    int attrib_position;
    int attrib_velocity;
    int attrib_acceleration;
    int attrib_particle_random;

    int random_i;
    int emitter_life_i;
};

Emitter_Instance new_emitter(Particle_System *PS, vec3 position)
{
    Emitter_Instance result = { };
    result.life = PS->emitter.life_max;
    result.position = position;
    return result;
}

float get_emitter_life(Emitter_Parameters *EP, Emitter_Instance *E)
{
    float life10 = E->life / EP->life_max;
    return clamp01(1.0f - life10);
}

#include <intrin.h> // for rtdsc


//float eval_f1(FXVM_Machine *vm, float *input, float **attributes, int instance_index, FXVM_Bytecode bytecode)
//{
//    FXVM_State state = { };
//    exec(vm, state, input, attributes, instance_index, &bytecode);
//    exec(vm, state, instance_index, &bytecode);
//    return state.r[0].v[0];
//}

//vec3 eval_f3(FXVM_Machine *vm, float *input, float **attributes, int instance_index, FXVM_Bytecode bytecode)
//{
//    FXVM_State state = { };
//    exec(vm, state, input, attributes, instance_index, &bytecode);
//    auto r = state.r[0];
//    return vec3{r.v[0], r.v[1], r.v[2]};
//}

float eval_f1(FXVM_Machine *vm, int instance_index, FXVM_Program *program)
{
    FXVM_State state = { };
    exec(vm, state, instance_index, program);
    return state.r[0].v[0];
}

vec3 eval_f3(FXVM_Machine *vm, int instance_index, FXVM_Program *program)
{
    FXVM_State state = { };
    exec(vm, state, instance_index, program);
    auto r = state.r[0];
    return vec3{r.v[0], r.v[1], r.v[2]};
}

template <int MAX_GROUP>
void eval_f1(FXVM_Machine *vm, float *dest, int instance_index, int instance_count, FXVM_Program *program)
{
    FXVM_State state[MAX_GROUP] = { };
    exec(vm, state, instance_index, instance_count, program);
    for (int i = 0; i < instance_count; i++)
    {
        auto r = state[i].r[0];
        dest[i] = r.v[0];
    }
}

template <int MAX_GROUP>
void eval_f3(FXVM_Machine *vm, vec3 *dest, int instance_index, int instance_count, FXVM_Program *program)
{
    FXVM_State state[MAX_GROUP] = { };
    exec(vm, state, instance_index, instance_count, program);
    for (int i = 0; i < instance_count; i++)
    {
        auto r = state[i].r[0];
        dest[i] = vec3{r.v[0], r.v[1], r.v[2]};
    }
}

template <int MAX_GROUP>
void eval_f4(FXVM_Machine *vm, vec4 *dest, int instance_index, int instance_count, FXVM_Program *program)
{
    FXVM_State state[MAX_GROUP] = { };
    exec(vm, state, instance_index, instance_count, program);
    for (int i = 0; i < instance_count; i++)
    {
        auto r = state[i].r[0];
        dest[i] = vec4{r.v[0], r.v[1], r.v[2], r.v[3]};
    }
}

void emit(FXVM_Machine *vm, Particle_System *PS, Emitter_Instance *E, float dt)
{
    //Particles *P = &E->P[E->frame & 1];
    Particles *P = &E->P[(E->frame+1) & 1];

    float num = PS->emitter.rate * dt;
    float to_emit = num + E->fractional_particles;
    num = trunc(to_emit);
    E->fractional_particles = to_emit - num;
    int num_to_emit = (int)num;

    //printf("num to emit %d, fractional_particles %f\n", num_to_emit, E->fractional_particles);
    //float global_input[16];
    //global_input[PS->emitter.life_i] = get_emitter_life(&PS->emitter, E);

    float emitter_life = get_emitter_life(&PS->emitter, E);
    set_uniform_f1(&PS->emitter.initial_position_p, PS->emitter.life_i, &emitter_life);
    set_uniform_f1(&PS->emitter.initial_velocity_p, PS->emitter.life_i, &emitter_life);

    E->life -= dt;
    if (E->life <= 0.0f)
    {
        if (PS->emitter.loop) E->life = PS->emitter.life_max;
    }

    int index = E->particles_alive;
    while (num_to_emit > 0 && index < Particles::MAX)
    {
        //P->position[index] = eval_f3(vm, global_input, nullptr, 0, PS->emitter.initial_position_p) + E->position;
        //P->velocity[index] = eval_f3(vm, global_input, nullptr, 0, PS->emitter.initial_velocity_p);
        vec3 position = E->position + PS->emitter.initial_position;
        if (PS->emitter.initial_position_p.bytecode.code)
        {
            position = position + eval_f3(vm, 0, &PS->emitter.initial_position_p);
        }
        P->position[index] = position;

        P->velocity[index] = PS->emitter.initial_velocity;
        if (PS->emitter.initial_velocity_p.bytecode.code)
        {
            P->velocity[index] = eval_f3(vm, 0, &PS->emitter.initial_velocity_p);
        }
        //fflush(stdout);exit(0);
        //{random01()-0.5f, 1.0f+random01()*0.5f, random01()-0.5f};
        P->acceleration[index] = PS->emitter.acceleration;//vec3{0.0f, -1.0f, 0.0f};

        float initial_life = PS->emitter.initial_life; // + random01();
        if (PS->emitter.initial_life_p.bytecode.code)
        {
            initial_life = eval_f1(vm, 0, &PS->emitter.initial_life_p);
        }

        P->life_seconds[index] = initial_life;
        P->life_max[index] = initial_life;
        P->life_01[index] = 0.0f;
        P->size[index] = PS->size;
        P->color[index] = PS->color; //vec4{1.0f, 1.0f, 1.0f, 1.0f};
        P->random[index] = vec4{random01(), random01(), random01(), random01()};

        num_to_emit--;
        index++;
    }
    E->particles_alive = index;
}

template <class T>
void swap(T &a, T &b)
{
    T temp = a;
    a = b;
    b = temp;
}

int compact(Emitter_Instance *E)
{
    Particles *P0 = &E->P[E->frame];
    Particles *P1 = &E->P[(E->frame + 1) & 1];

    int j = 0;
    for (int i = 0; i < E->particles_alive; )
    {
        while (i < Particles::MAX && P1->life_seconds[i] <= 0.0f)
        {
            i++;
        }
        if (i < Particles::MAX)
        {
            P0->life_seconds[j] = P1->life_seconds[i];
            P0->life_max[j] = P1->life_max[i];
            P0->position[j] = P1->position[i];
            P0->velocity[j] = P1->velocity[i];
            P0->acceleration[j] = P1->acceleration[i];
            P0->size[j] = P1->size[i];
            P0->color[j] = P1->color[i];
            P0->random[j] = P1->random[i];
            i++, j++;
        }
    }

    E->particles_alive = j;
    return j;
}

void simulate(FXVM_Machine *vm, Particle_System *PS, Emitter_Instance *E, float dt, uint64_t *emit_cycles, uint64_t *compact_cycles)
{
    uint64_t start_cycles = __rdtsc();
    compact(E);
    uint64_t end_cycles = __rdtsc();
    *compact_cycles += end_cycles - start_cycles;

    //E->particles_alive = 0;
    start_cycles = __rdtsc();
    if (E->life > 0.0f)
    {
        emit(vm, PS, E, dt);
    }
    end_cycles = __rdtsc();
    *emit_cycles += end_cycles - start_cycles;

    Particles *P = &E->P[E->frame & 1];

    float drag = PS->emitter.drag;
#if 0
    float global_input[16];
    float *attributes[16];
    attributes[PS->attrib_life] = P->life_01;
    attributes[PS->attrib_position] = (float*)&P->position;
    attributes[PS->attrib_velocity] = (float*)&P->velocity;
    attributes[PS->attrib_acceleration] = (float*)&P->acceleration;
    attributes[PS->attrib_particle_random] = (float*)&P->random;

    global_input[PS->emitter_life_i] = get_emitter_life(&PS->emitter, E);

    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            global_input[PS->random_i] = random01();

            P->life_seconds[i] -= dt;
            if (P->life_seconds[i] < 0.0f) P->life_seconds[i] = 0.0f;
            P->life_01[i] = 1.0f - P->life_seconds[i] * (1.0f / P->life_max[i]);

            if (PS->acceleration.code)
            {
                P->acceleration[i] = eval_f3(global_input, attributes, i, PS->acceleration);
            }

            vec3 vel = P->velocity[i];
            float v2 = -dot(vel, vel);
            vec3 Fd = normalize(vel) * v2 * drag;   // drag force
            vec3 ad = Fd;                           // drag acceleration F = ma => a = F/m; m = 1.0f => ad = Fd
            P->acceleration[i] = P->acceleration[i] + ad;

            //P->velocity[i] = (P->velocity[i] + P->acceleration[i] * dt) * drag;
            P->velocity[i] = P->velocity[i] + P->acceleration[i] * dt;
            P->position[i] = P->position[i] + P->velocity[i] * dt;
            P->size[i] = eval_f1(global_input, attributes, i, PS->size);
            P->color[i] = eval_f3(global_input, attributes, i, PS->color);
            //printf("P color: %.2f %.2f %.2f\n", P->color[i].x, P->color[i].y, P->color[i].z);

            E->particles_alive++;
        }
    }
#elif 1
    //float global_input[16];
    //float *attributes[16];
    //attributes[PS->attrib_life] = P->life_01;
    //attributes[PS->attrib_position] = (float*)&P->position;
    //attributes[PS->attrib_velocity] = (float*)&P->velocity;
    //attributes[PS->attrib_acceleration] = (float*)&P->acceleration;
    //attributes[PS->attrib_particle_random] = (float*)&P->random;

    //global_input[PS->emitter_life_i] = get_emitter_life(&PS->emitter, E);
    float emitter_life = get_emitter_life(&PS->emitter, E);

    FXVM_AttributeBindings attr_bindings = { };
    bind_attribute(&attr_bindings, PS->attrib_life, FXTYP_F1, sizeof(float), P->life_01);
    bind_attribute(&attr_bindings, PS->attrib_position, FXTYP_F3, sizeof(vec3), P->position);
    bind_attribute(&attr_bindings, PS->attrib_velocity, FXTYP_F3, sizeof(vec3), P->velocity);
    bind_attribute(&attr_bindings, PS->attrib_acceleration, FXTYP_F3, sizeof(vec3), P->acceleration);
    bind_attribute(&attr_bindings, PS->attrib_particle_random, FXTYP_F4, sizeof(vec4), P->random);

    vm->bindings = &attr_bindings;

    if (PS->acceleration_p.bytecode.code)
    {
        set_uniform_f1(&PS->acceleration_p, PS->emitter_life_i, &emitter_life);
        const int group_size = 16;
        int i = 0;
        for (; i + group_size - 1 < E->particles_alive; i += group_size)
        {
            //eval_f3<16>(vm, &P->acceleration[i], global_input, attributes, i, group_size, PS->acceleration);
            eval_f3<16>(vm, &P->acceleration[i], i, group_size, &PS->acceleration_p);
        }
        if (i < E->particles_alive)
        {
            int last_group = E->particles_alive - i;
            //eval_f3<16>(vm, &P->acceleration[i], global_input, attributes, i, last_group, PS->acceleration);
            eval_f3<16>(vm, &P->acceleration[i], i, last_group, &PS->acceleration_p);
        }
    }

    for (int i = 0; i < E->particles_alive; i++)
    {
        float life_seconds = P->life_seconds[i] - dt;
        if (life_seconds < 0.0f) life_seconds = 0.0f;

        P->life_seconds[i] = life_seconds;
        P->life_01[i] = 1.0f - life_seconds * (1.0f / P->life_max[i]);

        vec3 acceleration = PS->emitter.acceleration;
        if (PS->acceleration_p.bytecode.code) acceleration = P->acceleration[i];

        vec3 vel = P->velocity[i];
        float v2 = -dot(vel, vel);
        vec3 Fd = normalize(vel) * v2 * drag;   // drag force
        vec3 ad = Fd;                           // drag acceleration F = ma => a = F/m; m = 1.0f => ad = Fd
        acceleration = acceleration + ad;

        vec3 velocity = vel + acceleration * dt;
        vec3 position = P->position[i] + velocity * dt;

        P->acceleration[i] = acceleration;
        P->velocity[i] = velocity;
        P->position[i] = position;
    }
    if (PS->size_p.bytecode.code)
    {
        set_uniform_f1(&PS->size_p, PS->emitter_life_i, &emitter_life);
        const int group_size = 16;
        int i = 0;
        for (; i + group_size - 1 < E->particles_alive; i += group_size)
        {
            //eval_f1<16>(vm, &P->size[i], global_input, attributes, i, group_size, PS->size);
            eval_f1<16>(vm, &P->size[i], i, group_size, &PS->size_p);
        }
        if (i < E->particles_alive)
        {
            int last_group = E->particles_alive - i;
            //eval_f1<16>(vm, &P->size[i], global_input, attributes, i, last_group, PS->size);
            eval_f1<16>(vm, &P->size[i], i, last_group, &PS->size_p);
        }
    }
    if (PS->color_p.bytecode.code)
    {
        set_uniform_f1(&PS->color_p, PS->emitter_life_i, &emitter_life);
        const int group_size = 16;
        int i = 0;
        for (; i + group_size - 1 < E->particles_alive; i += group_size)
        {
            //printf("-- color:\n");
            //eval_f3<16>(vm, &P->color[i], global_input, attributes, i, group_size, PS->color);
            eval_f4<16>(vm, &P->color[i], i, group_size, &PS->color_p);
            //if (P->life_01[i] > 0.5f) exit(0);
            //printf("--\n");
        }
        if (i < E->particles_alive)
        {
            int last_group = E->particles_alive - i;
            //eval_f3<16>(vm, &P->color[i], global_input, attributes, i, last_group, PS->color);
            eval_f4<16>(vm, &P->color[i], i, last_group, &PS->color_p);
        }
    }
#else
    auto P1 = &E->P[(E->frame + 0) & 1];

    float global_input[16];
    float *attributes[16];
    attributes[PS->attrib_life] = P1->life_01;
    attributes[PS->attrib_position] = (float*)&P1->position;
    attributes[PS->attrib_velocity] = (float*)&P1->velocity;
    attributes[PS->attrib_acceleration] = (float*)&P1->acceleration;
    attributes[PS->attrib_particle_random] = (float*)&P1->random;

    global_input[PS->emitter_life_i] = get_emitter_life(&PS->emitter, E);

    for (int i = 0; i < E->particles_alive; i++)
    {
        global_input[PS->random_i] = random01();

        float life_seconds = P1->life_seconds[i] - dt;
        if (life_seconds < 0.0f) life_seconds = 0.0f;

        P->life_01[i] = 1.0f - life_seconds * (1.0f / P1->life_max[i]);
        P->life_seconds[i] = life_seconds;

        vec3 acceleration = PS->emitter.acceleration;
        if (PS->acceleration.code)
        {
            acceleration = eval_f3(vm, global_input, attributes, i, PS->acceleration);
        }

        vec3 vel = P1->velocity[i];
        float v2 = -dot(vel, vel);
        vec3 Fd = normalize(vel) * v2 * drag;   // drag force
        vec3 ad = Fd;                           // drag acceleration F = ma => a = F/m; m = 1.0f => ad = Fd
        acceleration = acceleration + ad;

        vec3 velocity = vel + acceleration * dt;
        vec3 position = P1->position[i] + velocity * dt;

        P->acceleration[i] = acceleration;
        P->velocity[i] = velocity;
        P->position[i] = position;

        P->size[i] = eval_f1(vm, global_input, attributes, i, PS->size);
        P->color[i] = eval_f3(vm, global_input, attributes, i, PS->color);

        P->random[i] = P1->random[i];
        //printf("P color: %.2f %.2f %.2f\n", P->color[i].x, P->color[i].y, P->color[i].z);
    }
#endif

    E->frame = (E->frame + 1) & 1;
}

struct Camera
{
    vec3 position;
    vec3 rotation;
    float zoom;
};

void move_camera(Camera *camera, int delta_x, int delta_y)
{
    camera->rotation.y += (float)delta_x * 0.015f;
    camera->rotation.x += (float)delta_y * 0.015f;
}

mat4 camera_matrix(Camera camera)
{
    return
        translation(vec3{0, -2.0, -1.0f * camera.zoom}) *
        rotation_x(-camera.rotation.x-0.5) *
        rotation_y(-camera.rotation.y)
        ;
}

#define FXVM_COMPILER_IMPL
#include "fxcomp.h"

void report_compile_error(const char *err) { printf("Error: %s\n", err); }

FXVM_Program compile_particle_expr(Particle_System *PS, const char *source, int source_len)
{
    FXVM_Compiler compiler = { };
    compiler.report_error = report_compile_error;

    PS->random_i = register_global_input_variable(&compiler, "random01", FXTYP_F1);
    PS->emitter_life_i = register_global_input_variable(&compiler, "emitter_life", FXTYP_F1);

    PS->attrib_life = register_attribute(&compiler, "particle_life", FXTYP_F1);
    PS->attrib_position = register_attribute(&compiler, "particle_position", FXTYP_F3);
    PS->attrib_velocity = register_attribute(&compiler, "particle_velocity", FXTYP_F3);
    PS->attrib_acceleration = register_attribute(&compiler, "particle_acceleration", FXTYP_F3);
    PS->attrib_particle_random = register_attribute(&compiler, "particle_random", FXTYP_F1);

    compile(&compiler, source, source + source_len);
    FXVM_Bytecode bytecode = { compiler.codegen.buffer_len, compiler.codegen.buffer };
#if 1
    printf("----\n");
    printf("%s\n", source);
    printf("====\n");
    disassemble(&bytecode);
    printf("----\n");
#endif
    FXVM_Program result = fxvm_program_new(bytecode);
    return result;
}

FXVM_Program compile_particle_expr(Particle_System *PS, const char *source)
{
    return compile_particle_expr(PS, source, strlen(source));
}

FXVM_Program compile_emitter_expr(Particle_System *PS, const char *source, int source_len)
{
    FXVM_Compiler compiler = { };
    compiler.report_error = report_compile_error;

    PS->emitter.life_i = register_global_input_variable(&compiler, "emitter_life", FXTYP_F1);
    PS->emitter.random_i = register_global_input_variable(&compiler, "random01", FXTYP_F1);

    compile(&compiler, source, source + source_len);
    FXVM_Bytecode bytecode = { compiler.codegen.buffer_len, compiler.codegen.buffer };
#if 0
    printf("----\n");
    printf("%s\n", source);
    printf("====\n");
    disassemble(&result);
    printf("----\n");
#endif
    FXVM_Program result = fxvm_program_new(bytecode);
    return result;
}

FXVM_Program compile_emitter_expr(Particle_System *PS, const char *source)
{
    return compile_emitter_expr(PS, source, strlen(source));
}

#define SOURCE(x) #x

Particle_System load_psys()
{
    Particle_System result = { };
    result.stretch = true;
    result.emitter.life_max = 8.0f;
    result.emitter.loop = true;
    result.emitter.rate = 800.0f;
    result.emitter.initial_life = 2.0f;
    result.emitter.acceleration = vec3{0.0f, 0.0f, 0.0f};
    result.emitter.drag = 0.95f;
    result.emitter.initial_position_p = compile_emitter_expr(&result, SOURCE(
        Rx = rand01();
        Ry = rand01();
        theta = Rx * 2.0 * PI * emitter_life * 5.0;
        r = Ry * 1.1415; // * emitter_life;
        sr = sqrt(r);
        sr * vec3(cos(theta), 0.0, sin(theta));
    ));
    result.emitter.initial_velocity = vec3{0.0f, 1.0f, 0.0f};
    result.emitter.initial_velocity_p = compile_emitter_expr(&result, SOURCE(
        vec3(random01-0.5, 8, sin(random01*3.2)-0.5) * 0.25;
    ));

    result.acceleration_p = compile_particle_expr(&result, SOURCE(
        Rx = rand01();
        Ry = rand01();
        Rz = rand01();
        target = vec3(0, 5 * emitter_life, 0);
        v = target - particle_position;
        v * 10.0 * emitter_life + vec3(Rx * 2.0 - 1.0, Ry * 2.0 - 1.0, Rz * 2.0 - 1.0) * 10.0 * emitter_life;
    ));
    result.size = 1.0f;
    result.size_p = compile_particle_expr(&result, SOURCE(
        emitter_life * 0.08 + 0.08 + 0.02 * particle_random - 0.04 * particle_life;// + random01 * 0.018;
    ));
    result.color = vec4{1, 1, 1, 1};
    result.color_p = compile_particle_expr(&result, SOURCE(
        c0 = lerp(vec4(1.0, 1.0, 0.5, 1.0), vec4(1.0, 0.5, 0.0, 1.0), clamp01(particle_life * 2.0));
        lerp(c0, vec4(0.5, 0.0, 0.0, 0.0), clamp01(particle_life * 2.0 - 1.0));
    ));
    return result;
}

Particle_System create_psys2()
{
    Particle_System result = { };
    result.stretch = true;
    result.emitter.life_max = 8.0f;
    result.emitter.loop = true;
    result.emitter.rate = 800.0f;
    result.emitter.initial_life = 2.0f;
    result.emitter.acceleration = vec3{0.0f, 5.5f, 0.0f};
    //result.emitter.drag = 0.95;
    result.emitter.initial_position_p = compile_emitter_expr(&result, SOURCE(
        Rx = rand01();
        Ry = rand01();
        theta = Rx * 2.0 * PI;
        //r = fract(Ry * 12771.2359);
        r = Ry;
        //sr = sqrt(r) * 2.0;
        sr = r * 2.0;
        vec3(sr * cos(theta), 0.0, sr * sin(theta));
    ));
    result.emitter.initial_velocity = vec3{0.0f, 1.0f, 0.0f};
    result.emitter.initial_velocity_p = compile_emitter_expr(&result, SOURCE(
        Rx = rand01();
        Ry = rand01();
        //vec3(R.x - 0.5, 2, R.y - 0.5);
        vec3(Rx, 2, Ry) - vec3(0.5, 0, 0.5); // vec3(R.x - 0.5, 2, R.y - 0.5);
    ));

    result.acceleration_p = compile_particle_expr(&result, SOURCE(
        p = particle_position;
        cs = cos(p * PI);
        sn = sin(p * PI * 2);
        wind = cs * sn + cs * cs;
        //wind = vec3(cos(emitter_life*PI), 0, sin(emitter_life*PI));
    ));
    result.size_p = compile_particle_expr(&result, SOURCE(
        t = particle_life - 0.5;
        t = 1.0 - t * t;
        t * 1.2;
        //0.2;
    ));
    result.color_p = compile_particle_expr(&result, SOURCE(
        c0 = lerp(vec4(0.01, 0.01, 2.2, 1.0), vec4(1.0, 1.0, 0.5, 1.0), clamp01(particle_position.y * 0.5));
        t0 = 2 * clamp01(particle_life) - 1;
        t = t0 * t0;
        lerp(c0, vec4(1, 1, 1, 0), t);
        //+ clamp01(particle_position.y) * vec3(0.2, 0.2, 0.2);
    ));
    //exit(0);
    return result;
}

Particle_System create_psys3()
{
    Particle_System result = { };
    result.stretch = false;
    result.emitter.life_max = 8.0f;
    result.emitter.loop = true;
    result.emitter.rate = 800.0f;
    result.emitter.initial_life = 2.0f;
    result.emitter.initial_life_p = compile_emitter_expr(&result, SOURCE(
        1.0 + rand01()*0.5;
    ));
    result.emitter.acceleration = vec3{0.0f, 5.5f, 0.0f};
    //result.emitter.drag = 0.95;
    result.emitter.initial_position_p = compile_emitter_expr(&result, SOURCE(
        Rx = rand01();
        Ry = rand01();
        theta = Rx * 2.0 * PI;
        r = Ry;
        sr = r * 0.2;
        vec3(sr * cos(theta), 0.0, sr * sin(theta));
    ));
    result.emitter.initial_velocity = vec3{0.0f, 1.0f, 0.0f};
    result.emitter.initial_velocity_p = compile_emitter_expr(&result, SOURCE(
        Rx = rand01();
        Ry = rand01();
        //vec3(R.x - 0.5, 2, R.y - 0.5);
        vec3(Rx, 4, Ry) - vec3(0.5, 0, 0.5); // vec3(R.x - 0.5, 2, R.y - 0.5);
    ));

    result.acceleration_p = compile_particle_expr(&result, SOURCE(
        p = particle_position;
        wind = cos(p) * 0.4 + vec3(0, 0.8, 0);
        //wind = vec3(cos(emitter_life*PI), 0, sin(emitter_life*PI));
    ));
    result.size_p = compile_particle_expr(&result, SOURCE(
        t = particle_life;
        0.4 + t * 1.2;
    ));
    result.color_p = compile_particle_expr(&result, SOURCE(
        c0 = lerp(vec4(1.0, 0.8, 0.6, 1.0), vec4(0.2, 0.2, 0.2, 1.0), clamp01(particle_position.y * 0.25));
        t0 = clamp01(2 * particle_life - 1);
        t = t0 * t0;
        lerp(c0, vec4(0, 0, 0, 0), t);
        //+ clamp01(particle_position.y) * vec3(0.2, 0.2, 0.2);
    ));
    //exit(0);
    return result;
}
struct Particle_DrawBuffer
{
    int cap;
    int size;

    uint32_t *sort_key;
    vec3 *P;
    vec3 *h0;
    vec3 *h1;
    vec4 *color;
};

int add_particle_buffer_particle(Particle_DrawBuffer *buffer)
{
    if (buffer->size + 1 > buffer->cap)
    {
        int new_cap = buffer->size + 256;
        buffer->sort_key = (uint32_t*)realloc(buffer->sort_key, sizeof(uint32_t) * new_cap);
        buffer->P = (vec3*)realloc(buffer->P, sizeof(vec3) * new_cap);
        buffer->h0 = (vec3*)realloc(buffer->h0, sizeof(vec3) * new_cap);
        buffer->h1 = (vec3*)realloc(buffer->h1, sizeof(vec3) * new_cap);
        buffer->color = (vec4*)realloc(buffer->color, sizeof(vec4) * new_cap);
        buffer->cap = new_cap;
    }

    int i = buffer->size;
    buffer->size += 1;
    return i;
}

void emit_particle_buffer_particle(Particle_DrawBuffer *buffer, Particles *P, int i, mat4 view_mat, vec3 right, vec3 up, vec3 look, bool stretch)
{
    int bi = add_particle_buffer_particle(buffer);

    float size = P->size[i] * 0.5f;

    vec3 w_pos = P->position[i];
    vec3 h0, h1;

    if (stretch)
    {
        vec3 w_vel = P->velocity[i];

        vec3 e3 = look;
        vec3 v = normalize(w_vel);
        //vec3 hh0 = normalize(w_vel - e3 * dot(w_vel, e3));
        //vec3 hh1 = normalize(cross(hh0, e3));
        vec3 hh0 = w_vel; //normalize(w_vel - e3 * dot(w_vel, e3));
        vec3 hh1 = normalize(cross(v, e3));
        vec3 hh12 = normalize(cross(v, up));

        float kk = dot(look, v);
        kk *= kk;
        kk *= kk;
        kk *= kk;
        hh1 = lerp(hh1, hh12, kk*kk);

        h0 = hh0 * size;
        h1 = hh1 * size;

        float k = dot(look, v) * 0.9f;
        k *= k;
        k *= k;

        k = 0.0f;

        vec3 v_vel_x = w_vel - e3 * dot(w_vel, e3);
        float t = dot(v_vel_x, v_vel_x);
        vec3 v_vel = lerp(right, normalize(v_vel_x), t > 0.1f ? 1.0f : t * 10.0f);
        vec3 vv0 = v_vel;
        vec3 vv1 = normalize(cross(v_vel, e3));

        vec3 v0 = vv0 * size;
        vec3 v1 = vv1 * size;

        h0 = lerp(h0, v0, k);
        h1 = lerp(h1, v1, k);
    }
    else
    {
        h0 = right * size;
        h1 = up * size;
    }

    vec4 v_pos = transform(view_mat, vec4{w_pos.x, w_pos.y, w_pos.z, 1.0f});
    // coordinate system => negative z is forward
    union {
        float depth_f;
        uint32_t depth_u;
    };
    depth_f = v_pos.z;

    // 1  8        7
    // s  eeeeeeee xxxxxxx
    // s  eeeeee   xxxxxxx
    uint32_t depth = depth_u ^ 0xfffff000;
    //depth = (depth & 0x80000000) | ((depth & 0x3ffff000) << 2);
    depth &= 0xffff0000;

    buffer->sort_key[bi] = (uint32_t)depth | (uint32_t)bi;
    buffer->P[bi] = w_pos;
    buffer->h0[bi] = h0;
    buffer->h1[bi] = h1;
    buffer->color[bi] = P->color[i];
}

void draw_to_buffer(Particle_DrawBuffer *buffer, Camera camera, Particle_System *PS, Emitter_Instance *E)
{
    Particles *P = &E->P[(E->frame + 1) & 1];

    mat4 view_mat = camera_matrix(camera);
    vec3 right = {view_mat[0], view_mat[1], view_mat[2]};
    vec3 up = {view_mat[4], view_mat[5], view_mat[6]};
    vec3 look = {view_mat[8], view_mat[9], view_mat[10]};

    for (int i = 0; i < E->particles_alive; i++)
    {
        emit_particle_buffer_particle(buffer, P, i, view_mat, right, up, look, PS->stretch);
    }
}

void reset_particle_buffer(Particle_DrawBuffer *buffer)
{
    buffer->size = 0;
}

void draw_particle_buffer_particle(Particle_DrawBuffer *buffer, uint32_t i)
{
    vec3 w_pos = buffer->P[i];
    vec3 h0 = buffer->h0[i];
    vec3 h1 = buffer->h1[i];
    vec4 color = buffer->color[i];

    vec3 w_pos0 = w_pos - h0 - h1;
    vec3 w_pos1 = w_pos - h0 + h1;
    vec3 w_pos2 = w_pos + h0 + h1;
    vec3 w_pos3 = w_pos + h0 - h1;

    glColor4f(color.x, color.y, color.z, color.w);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(&w_pos0.x);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3fv(&w_pos1.x);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(&w_pos2.x);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(&w_pos0.x);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(&w_pos2.x);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3fv(&w_pos3.x);
}

#include <algorithm>

void draw_particle_buffer(Particle_DrawBuffer *buffer, Camera camera, int width, int height)
{
    std::sort(buffer->sort_key, buffer->sort_key + buffer->size);

    mat4 camera_proj = Perspective_lh(90.0f, (float)width / (float)height, 0.1f, 1000.0f);
    camera_proj = transpose(camera_proj);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_proj.m);

    mat4 view_mat = camera_matrix(camera);
    view_mat = transpose(view_mat);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view_mat.m);

    /*
    if (PS->additive)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
    }
    else
    */
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glDisable(GL_BLEND);
    }

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < buffer->size; i++)
    {
        uint32_t key = buffer->sort_key[i];
        uint32_t index = key & 0xffff;

        draw_particle_buffer_particle(buffer, index);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
}


void draw_particle(Particles *P, int i, vec3 right, vec3 up, vec3 look, bool stretch)
{
    float size = P->size[i] * 0.5f;

    vec3 w_pos = P->position[i];
    vec3 w_vel = P->velocity[i];

    vec3 w_pos0;
    vec3 w_pos1;
    vec3 w_pos2;
    vec3 w_pos3;

    if (stretch)
    {
        vec3 e3 = look;
        vec3 v = normalize(w_vel);
        //vec3 hh0 = normalize(w_vel - e3 * dot(w_vel, e3));
        //vec3 hh1 = normalize(cross(hh0, e3));
        vec3 hh0 = w_vel; //normalize(w_vel - e3 * dot(w_vel, e3));
        vec3 hh1 = normalize(cross(v, e3));
        vec3 hh12 = normalize(cross(v, up));

        float kk = dot(look, v);
        kk *= kk;
        kk *= kk;
        kk *= kk;
        hh1 = lerp(hh1, hh12, kk*kk);

        vec3 h0 = hh0 * size;
        vec3 h1 = hh1 * size;

        float k = dot(look, v) * 0.9f;
        k *= k;
        k *= k;

        k = 0.0f;

        vec3 v_vel_x = w_vel - e3 * dot(w_vel, e3);
        float t = dot(v_vel_x, v_vel_x);
        vec3 v_vel = lerp(right, normalize(v_vel_x), t > 0.1f ? 1.0f : t * 10.0f);
        vec3 vv0 = v_vel;
        vec3 vv1 = normalize(cross(v_vel, e3));

        vec3 v0 = vv0 * size;
        vec3 v1 = vv1 * size;

        h0 = lerp(h0, v0, k);
        h1 = lerp(h1, v1, k);

        //h0 = lerp(h0, right * size, k);
        //h1 = lerp(h1, up * size, k);

        //w_pos0 = w_pos - h0 - h1;
        //w_pos1 = w_pos - h0;
        //w_pos2 = w_pos + h0;
        //w_pos3 = w_pos + h0 - h1;
        w_pos0 = w_pos - h0 - h1;
        w_pos1 = w_pos - h0 + h1;
        w_pos2 = w_pos + h0 + h1;
        w_pos3 = w_pos + h0 - h1;
    }
    else
    {
        vec3 h0 = right * size;
        vec3 h1 = up * size;

        w_pos0 = w_pos - h0 - h1;
        w_pos1 = w_pos - h0 + h1;
        w_pos2 = w_pos + h0 + h1;
        w_pos3 = w_pos + h0 - h1;
    }

    glColor4f(P->color[i].x, P->color[i].y, P->color[i].z, P->color[i].w);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(&w_pos0.x);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3fv(&w_pos1.x);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(&w_pos2.x);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(&w_pos0.x);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(&w_pos2.x);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3fv(&w_pos3.x);
}

void draw(Camera camera, int width, int height, Particle_System *PS, Emitter_Instance *E)
{
    Particles *P = &E->P[(E->frame + 1) & 1];

    mat4 camera_proj = Perspective_lh(90.0f, (float)width / (float)height, 0.1f, 1000.0f);
    camera_proj = transpose(camera_proj);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_proj.m);

    mat4 view_mat = camera_matrix(camera);
    vec3 right = {view_mat[0], view_mat[1], view_mat[2]};
    vec3 up = {view_mat[4], view_mat[5], view_mat[6]};
    vec3 look = {view_mat[8], view_mat[9], view_mat[10]};

    view_mat = transpose(view_mat);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view_mat.m);

    if (PS->additive)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
    }
    else
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        //glDisable(GL_BLEND);
    }

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < E->particles_alive; i++)
    {
        draw_particle(P, i, right, up, look, PS->stretch);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();
}

const char* read_file(const char *filename, int *len)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return nullptr;

    fseek(fp, 0, SEEK_END);
    int file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *s = (char*)malloc(file_len);
    fread(s, 1, file_len, fp);

    *len = file_len;
    return s;
}

struct StringRef
{
    const char *s;
    int len;
};

bool str_equals(StringRef s1, const char *s2)
{
    int len = 0;
    while (s2[0] != '\0' && len < s1.len)
    {
        if (s1.s[len] != s2[0]) return false;
        s2++;
        len++;
    }

    return (len == s1.len);
}

bool skip_whitespace(const char *&p, const char *end)
{
    bool ws_found = false;
    while (p < end)
    {
        switch (p[0])
        {
            case '\n': case '\r': case ' ': case '\t':
                ws_found = true;
                p++;
                continue;
        }
        break;
    }
    return ws_found;
}

StringRef read_attribute(const char *&p, const char *end)
{
    if (p >= end) return { };
    if (!isalpha(p[0])) return { };

    const char *start = p;
    while ((p < end) && (isalpha(p[0]) || p[0] == '_'))
    {
        p++;
    }
    int len = p - start;
    return {start, len};
}

enum Attribute_ValueType
{
    ATTR_BOOLEAN,
    ATTR_F1, ATTR_F2, ATTR_F3, ATTR_F4,
    ATTR_PROGRAM,
};

bool convert_boolean(const char *s, int len)
{
    (void)len;
    return s[0] == 't';
}

float convert_f1(const char *s, int len)
{
    char buf[32];
    memcpy(buf, s, len);
    buf[len] = '\0';
    return atof(buf);
}

void convert_f2(float *v, const char *s, int len)
{
    (void)len;
    s += 5; // vec2(
    int i = 0;
    while (s[0] != ')')
    {
        while (s[0] == ' ') s++;
        const char *start = s;
        while (s[0] != ',' && s[0] != ')' && s[0] != ' ') s++;

        v[i] = convert_f1(start, s - start);

        while (s[0] == ' ') s++;
        if (s[0] == ',') s++;
        i++;
    }
}

void convert_f3(float *v, const char *s, int len)
{
    (void)len;
    s += 5; // vec3(
    int i = 0;
    while (s[0] != ')')
    {
        while (s[0] == ' ') s++;
        const char *start = s;
        while (s[0] != ',' && s[0] != ')' && s[0] != ' ') s++;

        v[i] = convert_f1(start, s - start);

        while (s[0] == ' ') s++;
        if (s[0] == ',') s++;
        i++;
    }
}

void convert_f4(float *v, const char *s, int len)
{
    (void)len;
    s += 5; // vec4(
    int i = 0;
    while (s[0] != ')')
    {
        while (s[0] == ' ') s++;
        const char *start = s;
        while (s[0] != ',' && s[0] != ')' && s[0] != ' ') s++;

        v[i] = convert_f1(start, s - start);

        while (s[0] == ' ') s++;
        if (s[0] == ',') s++;
        i++;
    }
}

StringRef read_value(const char *&p, const char *file_end, Attribute_ValueType *type)
{
    switch (p[0])
    {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        {
            *type = ATTR_F1;
            const char *start = p;
            while (p < file_end && isdigit(p[0])) p++;
            if (p < file_end && p[0] == '.')
            {
                p++;
                while (p < file_end && isdigit(p[0])) p++;
                if (p < file_end && (p[0] == 'e' || p[0] == 'E'))
                {
                    p++;
                    if (p < file_end && (p[0] == '-' || p[0] == '+')) p++;
                    while (p < file_end && isdigit(p[0])) p++;
                }
            }
            int len = p - start;
            return {start, len};
        } break;
    case 'v':
        {
            const char *start = p;
            if (p + 5 < file_end &&
                (p[0] == 'v' && p[1] == 'e' && p[2] == 'c' && p[4] == '('))
            {
                p += 5;
                switch (p[-2])
                {
                case '2':
                    {
                        *type = ATTR_F2;
                    } break;
                case '3':
                    {
                        *type = ATTR_F3;
                    } break;
                case '4':
                    {
                        *type = ATTR_F4;
                    } break;
                }

                while (p < file_end && p[0] != ')') p++;
                if (p == file_end)
                {
                    printf("Error: unexpected end of file\n");
                    return { };
                }
                p++; // skip ')'
            }
            else
            {
                printf("Error: Expected vec2, vec3 or vec4\n");
                return { };
            }
            int len = p - start;
            return {start, len};
        } break;
    case '{':
        {
            if (p + 1 >= file_end || p[1] != '{')
            {
                printf("Error: Expected {{ to start expression\n");
                return { };
            }
            p += 2;

            *type = ATTR_PROGRAM;
            const char *start = p;
            while (p < file_end && p[0] != '}')
            {
                p++;
            }

            const char *end = p;
            if (p + 1 >= file_end && p[1] != '}')
            {
                printf("Error: Expected }} to end expression\n");
                return { };
            }
            p += 2;

            int len = end - start;
            return {start, len};
        } break;
    }
    printf("ERROR: INVALID VALUE\n");
    return { };
}

Particle_System load_particle_system(const char *filename)
{
    Particle_System result = { };
    result.stretch = false;
    result.emitter.life_max = 8.0f;
    result.emitter.loop = true;
    result.emitter.rate = 800.0f;
    result.emitter.initial_life = 2.0f;
    result.emitter.acceleration = vec3{0.0f, 0.0f, 0.0f};
    result.emitter.drag = 0.95f;
    result.emitter.initial_velocity = vec3{0.0f, 1.0f, 0.0f};
    result.size = 1.2f;
    result.color = vec4{1, 1, 1, 1};

    //return result;

    int file_len = 0;
    const char *file_str = read_file(filename, &file_len);

    if (!file_str) return result;

    const char *file_end = file_str + file_len;
    const char *p = file_str;

    enum { EMITTER_ATTRIBUTE_NUM = 7, PARTICLE_ATTRIBUTE_NUM = 3 };
    struct {
        const char *name;
        Attribute_ValueType type;
        float *value;
        FXVM_Program *p_value;
        bool *b_value;
    } emitter_attribute_map[EMITTER_ATTRIBUTE_NUM] = {
        {"emitter_life_max", ATTR_BOOLEAN, &result.emitter.life_max, nullptr, nullptr},
        {"emitter_loop", ATTR_F1, nullptr, nullptr, &result.emitter.loop},
        {"emitter_rate", ATTR_F1, &result.emitter.rate, &result.emitter.rate_p, nullptr},
        {"drag", ATTR_F1, &result.emitter.drag, &result.emitter.drag_p, nullptr},
        {"initial_life", ATTR_F1, &result.emitter.initial_life, &result.emitter.initial_life_p, nullptr},
        {"initial_position", ATTR_F3, &result.emitter.initial_position.x, &result.emitter.initial_position_p, nullptr},
        {"initial_velocity", ATTR_F3, &result.emitter.initial_velocity.x, &result.emitter.initial_velocity_p, nullptr},
    }, particle_attribute_map[PARTICLE_ATTRIBUTE_NUM] = {
        {"acceleration", ATTR_F3, &result.acceleration.x, &result.acceleration_p, nullptr},
        {"color", ATTR_F4, &result.color.x, &result.color_p, nullptr},
        {"size", ATTR_F1, &result.size, &result.size_p, nullptr},
    };

    while (p < file_end)
    {
        skip_whitespace(p, file_end);
        if (p == file_end)
        {
            break;
        }

        StringRef attribute = read_attribute(p, file_end);
        if (!attribute.s)
        {
            // TODO: line and column
            printf("Error: was expecting for an attribute\n");
            goto err;
        }

        skip_whitespace(p, file_end);
        if (p == file_end || p[0] != '=')
        {
            // TODO: line and column
            printf("Error: was expecting =\n");
            goto err;
        }

        p++; // skip '='

        skip_whitespace(p, file_end);
        if (p == file_end)
        {
            printf("Error: unexpected end of file\n");
            goto err;
        }

        printf("Got attribute %.5s\n", attribute.s);

        Attribute_ValueType type;
        StringRef value = read_value(p, file_end, &type);

        printf("Got value %.17s type %d\n", value.s, type);

        bool emitter_attrib_found = false;
        for (int i = 0; i < EMITTER_ATTRIBUTE_NUM; i++)
        {
            auto attrib = emitter_attribute_map[i];
            if (str_equals(attribute, attrib.name))
            {
                emitter_attrib_found = true;
                switch (type)
                {
                case ATTR_BOOLEAN:
                    {
                        if (!attrib.b_value)
                        {
                            printf("Error: no boolean value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        *attrib.b_value = convert_boolean(value.s, value.len);
                    } break;
                case ATTR_F1:
                    {
                        if (!attrib.value)
                        {
                            printf("Error: no scalar value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        *attrib.value = convert_f1(value.s, value.len);
                    } break;
                case ATTR_F2:
                    {
                        if (!attrib.value)
                        {
                            printf("Error: no vec2 value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        convert_f2(attrib.value, value.s, value.len);
                    } break;
                case ATTR_F3:
                    {
                        if (!attrib.value)
                        {
                            printf("Error: no vec3 value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        convert_f3(attrib.value, value.s, value.len);
                    } break;
                case ATTR_F4:
                    {
                        if (!attrib.value)
                        {
                            printf("Error: no vec4 value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        convert_f4(attrib.value, value.s, value.len);
                    } break;
                case ATTR_PROGRAM:
                    {
                        if (!attrib.p_value)
                        {
                            printf("Error: no program value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        *attrib.p_value = compile_emitter_expr(&result, value.s, value.len);
                    } break;
                }
            }
        }
        if (emitter_attrib_found) continue;

        bool particle_attrib_found = false;
        for (int i = 0; i < PARTICLE_ATTRIBUTE_NUM; i++)
        {
            auto attrib = particle_attribute_map[i];
            if (str_equals(attribute, attrib.name))
            {
                particle_attrib_found = true;
                switch (type)
                {
                case ATTR_BOOLEAN:
                    {
                        if (!attrib.b_value || attrib.type != type)
                        {
                            printf("Error: no boolean value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        *attrib.b_value = convert_boolean(value.s, value.len);
                    } break;
                case ATTR_F1:
                    {
                        if (!attrib.value || attrib.type != type)
                        {
                            printf("Error: no scalar value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        *attrib.value = convert_f1(value.s, value.len);
                    } break;
                case ATTR_F2:
                    {
                        if (!attrib.value || attrib.type != type)
                        {
                            printf("Error: no vec2 value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        convert_f2(attrib.value, value.s, value.len);
                    } break;
                case ATTR_F3:
                    {
                        if (!attrib.value || attrib.type != type)
                        {
                            printf("Error: no vec3 value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        convert_f3(attrib.value, value.s, value.len);
                    } break;
                case ATTR_F4:
                    {
                        if (!attrib.value || attrib.type != type)
                        {
                            printf("Error: no vec4 value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        convert_f4(attrib.value, value.s, value.len);
                    } break;
                case ATTR_PROGRAM:
                    {
                        if (!attrib.p_value) // TODO: get the return type of the program, ensure compatible
                        {
                            printf("Error: no program value allowed for %s\n", attrib.name);
                            goto err;
                        }
                        *attrib.p_value = compile_particle_expr(&result, value.s, value.len);
                    } break;
                }
            }
        }
        if (particle_attrib_found) continue;

        char buf[64];
        strncpy(buf, attribute.s, attribute.len);
        buf[attribute.len] = '\0';
        printf("Error: unknown attribute '%s'\n", buf);
        goto err;
    }

    free((void*)file_str);
    return result;

err:
    free((void*)file_str);
    return { };
}



void mouse_wheel(float wheel, Camera *camera)
{
    camera->zoom += wheel * 0.05f;
}

void key_down(int key, bool *keys)
{
    keys[key] = true;
}

void key_up(int key, bool *keys)
{
    keys[key] = false;
}

void (__stdcall *glGenerateMipmap)(GLenum);

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    Window window = { };
    create_window(1000, 800, &window);

    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);

    /*
    uint8_t tex_data[64] = {
        0, 1,  1,   2,   2,  1, 1, 0,
        1, 4,  8,   8,   8,  8, 4, 1,
        1, 8, 16,  32,  32, 16, 8, 1,
        2, 8, 32, 128, 192, 48, 8, 2,
        2, 8, 48, 192, 128, 32, 8, 2,
        1, 8, 16,  32,  32, 16, 8, 1,
        1, 4,  8,   8,   8,  8, 4, 1,
        0, 1,  1,   2,   2,  1, 1, 0,
    };
    uint8_t tex_data[64] = {
        0,   1,   1,   4,  32,   2,  1,  0,
        1,   8,   8,  16,  64,  16,  8,  1,
        1,   8,   8,  32, 128,  32, 16,  2,
        4,  16,  32, 190, 192, 128, 64, 32,
        32, 64, 128, 192, 190,  32, 16,  4,
        2,  16,  32, 128,  32,   8,  8,  1,
        1,   8,  16,  64,  16,   8,  8,  1,
        0,   1,   2,  32,   4,   1,  1,  0,
    };
    */

    //uint8_t tex_data[128] = {
    //    0,  0,   1,  1,   1,   1,   4,   4,  32,  32,   2,   2,  1,  1,  0,  0,
    //    1,  1,   8,  8,   8,   8,  16,  16,  64,  64,  16,  16,  8,  8,  1,  1,
    //    1,  1,   8,  8,   8,   8,  32,  32, 128, 128,  32,  32, 16, 16,  2,  2,
    //    4,  4,  16, 16,  32,  32, 190, 190, 192, 192, 128, 128, 64, 64, 32, 32,
    //    32, 32, 64, 64, 128, 128, 192, 192, 190, 190,  32,  32, 16, 16,  4,  4,
    //    2,  2,  16, 16,  32,  32, 128, 128,  32,  32,   8,   8,  8,  8,  1,  1,
    //    1,  1,   8,  8,  16,  16,  64,  64,  16,  16,   8,   8,  8,  8,  1,  1,
    //    0,  0,   1,  1,   2,   2,  32,  32,   4,   4,   1,   1,  1,  1,  0,  0,
    //};

    uint8_t tex_data[128] = {
        255,  2, 255,  4, 255,   8, 255,  16, 255,  16, 255,   8, 255,  4, 255,  2,
        255,  4, 255,  8, 255,  16, 255,  32, 255,  32, 255,  16, 255,  8, 255,  4,
        255,  8, 255, 16, 255,  16, 255,  64, 255, 128, 255,  32, 255, 16, 255,  8,
        255, 16, 255, 32, 255,  64, 255, 190, 255, 192, 255, 128, 255, 32, 255, 16,
        255, 16, 255, 32, 255, 128, 255, 192, 255, 190, 255,  64, 255, 32, 255, 16,
        255,  8, 255, 16, 255,  32, 255, 128, 255,  64, 255,  16, 255, 16, 255,  8,
        255,  4, 255,  8, 255,  16, 255,  32, 255,  32, 255,  16, 255,  8, 255,  4,
        255,  2, 255,  4, 255,   8, 255,  16, 255,  16, 255,   8, 255,  4, 255,  2,
    };

    glGenerateMipmap = (void (*)(GLenum))(void*)wglGetProcAddress("glGenerateMipmap");

    GLuint particle_tex;
    glGenTextures(1, &particle_tex);
    glBindTexture(GL_TEXTURE_2D, particle_tex);
    glTexImage2D(GL_TEXTURE_2D,
            0, // level
            GL_RGBA,
            8,
            8,
            0, // border
            GL_LUMINANCE_ALPHA,
            GL_UNSIGNED_BYTE,
            tex_data
            );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    Particle_System PS1 = load_psys();
    Emitter_Instance E1 = new_emitter(&PS1, vec3{0, 0, 0});

    Particle_System PS2 = create_psys2();
    Emitter_Instance E2 = new_emitter(&PS2, vec3{2, 0, 0});

    //Particle_System PS3 = create_psys3();
    Particle_System PS3 = load_particle_system("particle_systems/simple.psys");
    Emitter_Instance E3 = new_emitter(&PS3, vec3{-2, 0, 0});

    FXVM_Machine vm = fxvm_new();

    //exit(0);

    Camera camera = { };
    camera.zoom = 5.0f;

    window.wheel_user_ptr = &camera;
    window.mouse_wheel = (void (*)(float, void*))mouse_wheel;

    bool keys[512] = { };
    set_window_key_down(&window, key_down, keys);
    set_window_key_up(&window, key_up, keys);

    float time_accum = 0.0f;

    float sim_dt = 0.01666f;
    uint64_t sim_ticks = 0;
    float sim_ticks_smooth = 0.0f;
    float sim_ticks_avg = 0.0f;
    uint64_t sim_count = 0;
    float avg_particles = 0;

    uint64_t sim_emit_ticks = 0;
    uint64_t sim_compact_ticks = 0;

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    //LARGE_INTEGER start_time = counter;

    Particle_DrawBuffer particle_buffer = { };

    bool last_L = false;

    MSG msg = { };
    while (window.running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (window.buttons & MK_LBUTTON)
        {
            int delta_x = window.mouse_x - window.last_mouse_x;
            int delta_y = window.mouse_y - window.last_mouse_y;
            move_camera(&camera, delta_x, delta_y);
        }
        window.last_mouse_x = window.mouse_x;
        window.last_mouse_y = window.mouse_y;

        if (!last_L && keys['L'])
        {
            PS3 = load_particle_system("particle_systems/simple.psys");
        }
        last_L = keys['L'];

        LARGE_INTEGER last_counter = counter;
        QueryPerformanceCounter(&counter);
        LONGLONG tick_count = counter.QuadPart - last_counter.QuadPart;
        float dt = (float)tick_count / (float)freq.QuadPart;
        time_accum += dt;

        //LONGLONG total_ticks = counter.QuadPart - start_time.QuadPart;
        //if (total_ticks / freq.QuadPart >= 10) break;

        if (time_accum > 0.1f) time_accum = 0.1f;

        while (time_accum >= sim_dt)
        {
            uint64_t start_cycles = __rdtsc(), emit_cycles = 0, compact_cycles = 0;
            //simulate(&vm, &PS1, &E1, sim_dt * 0.125f * 1, &emit_cycles, &compact_cycles);
            //simulate(&vm, &PS2, &E2, sim_dt * 0.125f * 1, &emit_cycles, &compact_cycles);
            simulate(&vm, &PS3, &E3, sim_dt * 0.125f * 1, &emit_cycles, &compact_cycles);
            uint64_t end_cycles = __rdtsc();

            sim_emit_ticks = emit_cycles;
            sim_compact_ticks = compact_cycles;

            sim_ticks = end_cycles - start_cycles;
            sim_ticks_smooth = sim_ticks_smooth * 0.99f + sim_ticks * 0.01f;
            sim_ticks_avg += sim_ticks;
            time_accum -= sim_dt;

            avg_particles += (float)E1.particles_alive + (float)E2.particles_alive + (float)E3.particles_alive;
            sim_count++;
        }
        glClear(GL_COLOR_BUFFER_BIT);

        reset_particle_buffer(&particle_buffer);
        draw_to_buffer(&particle_buffer, camera, &PS1, &E1);
        draw_to_buffer(&particle_buffer, camera, &PS2, &E2);
        draw_to_buffer(&particle_buffer, camera, &PS3, &E3);
        draw_particle_buffer(&particle_buffer, camera, window.width, window.height);

        //draw(camera, window.width, window.height, &PS1, &E1);
        //draw(camera, window.width, window.height, &PS2, &E2);
        //draw(camera, window.width, window.height, &PS3, &E3);

        float fps = 1.0f / dt;

        char buf[128];
        snprintf(buf, 128, "FPS %.3f; SIM %lld cycles (%.0f) (%llu, %llu), particles alive %d",
                fps, sim_ticks, sim_ticks_smooth, sim_emit_ticks, sim_compact_ticks, E1.particles_alive + E2.particles_alive + E3.particles_alive);
        set_window_title(window, buf);

        SwapBuffers((HDC)window.hdc);
    }

    float avg_ticks_per_particle = sim_ticks_avg / avg_particles;

    sim_ticks_avg /= sim_count;
    avg_particles /= sim_count;

    printf("avg ticks\t smooth\t avg particle\t avg ticks per particle\n");
    printf("%.0f\t %0.f\t  %.3f\t %.3f\n", sim_ticks_avg, sim_ticks_smooth, avg_particles, avg_ticks_per_particle);

    wglMakeCurrent((HDC)window.hdc, nullptr);
    wglDeleteContext((HGLRC)window.glrc);
    DestroyWindow((HWND)window.hwnd);
    return 0;
}

