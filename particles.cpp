
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
};

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


/*
void emit(Emitter *E, Particles *P, float dt)
{
    float num = E->rate * dt;
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
            P->color[index] = vec3{1.0f, 1.0f, 1.0f};
            P->random[index] = vec4{random01(), random01(), random01(), random01()};

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
    E->particles_alive = 0;
    emit(E, P, dt);
    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            P->life_seconds[i] -= dt;
            P->velocity[i] = P->velocity[i] + P->acceleration[i] * dt;
            P->position[i] = P->position[i] + P->velocity[i] * dt;

            E->particles_alive++;
        }
    }
}
*/

#include <cmath>
#include "fxvm.h"

float random01()
{
    return (float)rand() / RAND_MAX;
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


struct Emitter_Instance
{
    float rate;
    float fractional_particles;
    float life;

    int last_index;

    int particles_alive;
};

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
    vec3 color[MAX];
    vec4 random[MAX];
};

struct Emitter_Parameters
{
    float rate;
    FXVM_Bytecode rate_bc;

    float initial_life;
    FXVM_Bytecode initial_life_bc;

    vec3 initial_velocity;
    FXVM_Bytecode initial_velocity_bc;

    vec3 initial_position;
    FXVM_Bytecode initial_position_bc;

    float drag;
    FXVM_Bytecode drag_bc;

    int random_i;
};

struct Particle_System
{
    Emitter_Parameters emitter;

    FXVM_Bytecode color;
    FXVM_Bytecode size;

    int attrib_life;
    int attrib_velocity;
    int attrib_particle_random;

    int random_i;
};

float eval_f1(float *input, float **attributes, int instance_index, FXVM_Bytecode bytecode)
{
    FXVM_State state = { };
    exec(state, input, attributes, instance_index, &bytecode);
    return state.r[0].v[0];
}

vec3 eval_f3(float *input, float **attributes, int instance_index, FXVM_Bytecode bytecode)
{
    FXVM_State state = { };
    exec(state, input, attributes, instance_index, &bytecode);
    auto r = state.r[0];
    return vec3{r.v[0], r.v[1], r.v[2]};
}

void emit(Particle_System *PS, Emitter_Instance *E, Particles *P, float dt)
{
    float num = PS->emitter.rate * dt;
    float to_emit = num + E->fractional_particles;
    num = trunc(to_emit);
    E->fractional_particles = to_emit - num;
    int num_to_emit = (int)num;

    //printf("num to emit %d, fractional_particles %f\n", num_to_emit, E->fractional_particles);
    float global_input[16];

    int last_i = E->last_index;
    int i = last_i + 1;
    int search = 0;
    while (search < Particles::MAX && num_to_emit > 0)
    {
        int index = i % Particles::MAX;
        if (P->life_seconds[index] <= 0.0f)
        {
            global_input[PS->emitter.random_i] = random01();
            //P->position[index] = vec3{0.0f, 0.0f, 0.0f};
            P->position[index] = eval_f3(global_input, nullptr, 0, PS->emitter.initial_position_bc);
            P->velocity[index] = eval_f3(global_input, nullptr, 0, PS->emitter.initial_velocity_bc);
            //fflush(stdout);exit(0);
            //{random01()-0.5f, 1.0f+random01()*0.5f, random01()-0.5f};
            P->acceleration[index] = vec3{0.0f, -1.0f, 0.0f};

            float initial_life = E->life + random01();
            P->life_seconds[index] = initial_life;
            P->life_max[index] = initial_life;
            P->life_01[index] = 0.0f;
            P->size[index] = 0.01f;
            P->color[index] = vec3{1.0f, 1.0f, 1.0f};
            P->random[index] = vec4{random01(), random01(), random01(), random01()};

            num_to_emit--;
        }
        i++;
        search++;
        last_i = index;
    }
    E->last_index = last_i;
}

void simulate(Particle_System *PS, Emitter_Instance *E, Particles *P, float dt)
{
    E->particles_alive = 0;
    emit(PS, E, P, dt);
    float drag = PS->emitter.drag;

    float global_input[16];
    float *attributes[16];
    attributes[PS->attrib_life] = P->life_01;
    attributes[PS->attrib_velocity] = (float*)&P->velocity;
    attributes[PS->attrib_particle_random] = (float*)&P->random;

    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            global_input[PS->random_i] = random01();

            P->life_seconds[i] -= dt;
            if (P->life_seconds[i] < 0.0f) P->life_seconds[i] = 0.0f;
            P->life_01[i] = 1.0f - P->life_seconds[i] * (1.0f / P->life_max[i]);

            P->velocity[i] = (P->velocity[i] + P->acceleration[i] * dt) * drag;
            P->position[i] = P->position[i] + P->velocity[i] * dt;
            P->size[i] = eval_f1(global_input, attributes, i, PS->size);
            P->color[i] = eval_f3(global_input, attributes, i, PS->color);
            //printf("P color: %.2f %.2f %.2f\n", P->color[i].x, P->color[i].y, P->color[i].z);

            E->particles_alive++;
        }
    }
}

struct Camera
{
    vec3 position;
    vec3 rotation;
    float zoom;
};

void move_camera(Camera *camera, int delta_x, int delta_y)
{
    camera->rotation.y += (float)delta_x * 0.02f;
    camera->rotation.x += (float)delta_y * 0.02f;
}

void draw_particle(Particles *P, int i, vec3 right, vec3 up)
{
    float size = P->size[i] * 0.5f;

    vec3 w_pos = vec3{P->position[i].x, P->position[i].y, P->position[i].z};

    //vec3 h0 = vec3{1.0f, 0.0f, 0.0f} * size;
    //vec3 h1 = vec3{0.0f, 1.0f, 0.0f} * size;

    vec3 h0 = right * size;
    vec3 h1 = up * size;

    vec3 w_pos0 = w_pos - h0 - h1;
    vec3 w_pos1 = w_pos - h0 + h1;
    vec3 w_pos2 = w_pos + h0 + h1;
    vec3 w_pos3 = w_pos + h0 - h1;

    glColor3f(P->color[i].x, P->color[i].y, P->color[i].z);
    glVertex3fv(&w_pos0.x);
    glVertex3fv(&w_pos1.x);
    glVertex3fv(&w_pos2.x);
    glVertex3fv(&w_pos0.x);
    glVertex3fv(&w_pos2.x);
    glVertex3fv(&w_pos3.x);
}

mat4 camera_matrix(Camera camera)
{
    return
        translation(vec3{0, 0.0, -1.0f * camera.zoom}) *
        rotation_x(camera.rotation.x) *
        rotation_y(camera.rotation.y)
        ;
}

void draw(Camera camera, int width, int height, Particles *P)
{
    mat4 camera_proj = Perspective_lh(90.0f, (float)width / (float)height, 0.1f, 1000.0f);
    camera_proj = transpose(camera_proj);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_proj.m);

    //mat4 identity = mat4{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    mat4 view_mat = camera_matrix(camera);
    vec3 right = {view_mat[0], view_mat[1], view_mat[2]};
    vec3 up = {view_mat[4], view_mat[5], view_mat[6]};
    view_mat = transpose(view_mat);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view_mat.m);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    //glBlendEquationEXT();

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < Particles::MAX; i++)
    {
        if (P->life_seconds[i] > 0.0f)
        {
            draw_particle(P, i, right, up);
        }
    }
    glEnd();
}

#include "fxcomp.h"

void report_compile_error(const char *err) { printf("Error: %s\n", err); }

FXVM_Bytecode compile_particle_expr(Particle_System *PS, const char *source)
{
    FXVM_Compiler compiler = { };
    compiler.report_error = report_compile_error;

    PS->random_i = register_global_input_variable(&compiler, "random01", FXTYP_F1);

    PS->attrib_life = register_attribute(&compiler, "particle_life", FXTYP_F1);
    PS->attrib_velocity = register_attribute(&compiler, "particle_velocity", FXTYP_F3);
    PS->attrib_particle_random = register_attribute(&compiler, "particle_random", FXTYP_F1);

    compile(&compiler, source, source + strlen(source));
    return { compiler.codegen.buffer_len, compiler.codegen.buffer };
}

FXVM_Bytecode compile_emitter_expr(Particle_System *PS, const char *source)
{
    FXVM_Compiler compiler = { };
    compiler.report_error = report_compile_error;

    PS->emitter.random_i = register_global_input_variable(&compiler, "random01", FXTYP_F1);

    compile(&compiler, source, source + strlen(source));
    return { compiler.codegen.buffer_len, compiler.codegen.buffer };
}

#define SOURCE(x) #x

Particle_System load_psys()
{
    Particle_System result = { };
    result.emitter.rate = 40.0f;
    result.emitter.initial_velocity = vec3{0.0f, 1.0f, 0.0f};
    result.emitter.drag = 0.97;
    result.emitter.initial_velocity_bc = compile_emitter_expr(&result, SOURCE(
        normalize(vec3(random01-0.5, 1, sin(random01*3.2)-0.5));
    ));
    result.size = compile_particle_expr(&result, SOURCE(
        0.01 + 0.01 * particle_random - 0.02 * particle_life + random01 * 0.018;
    ));
    result.color = compile_particle_expr(&result, SOURCE(
        c0 = lerp(vec3(1.0, 1.0, 0.5), vec3(1.0, 0.5, 0.0), clamp01(particle_life * 2.0));
        lerp(c0, vec3(0.5, 0.0, 0.0), clamp01(particle_life * 2.0 - 1.0));
    ));
    return result;
}

#include <intrin.h>

void mouse_wheel(float wheel, Camera *camera)
{
    printf("wheel %f\n", wheel); fflush(stdout);
    camera->zoom += wheel * 0.05f;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    Window window = { };
    create_window(1000, 800, &window);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    Particles P = { };
    Emitter_Instance E = { };
    E.rate = 30.0f;
    E.life = 1.0f;

    Particle_System psys = load_psys();
    //Emitter_Instance E = new_emitter(psys);

    Camera camera = { };
    camera.zoom = 1.0f;

    window.wheel_user_ptr = &camera;
    window.mouse_wheel = (void (*)(float, void*))mouse_wheel;

    float time_accum = 0.0f;

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    float sim_dt = 0.01666f;
    uint64_t sim_ticks = 0;
    float sim_ticks_smooth = 0.0f;

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

        LARGE_INTEGER last_counter = counter;
        QueryPerformanceCounter(&counter);
        LONGLONG tick_count = counter.QuadPart - last_counter.QuadPart;
        float dt = (float)tick_count / (float)freq.QuadPart;
        time_accum += dt;

        if (time_accum > 0.1f) time_accum = 0.1f;

        while (time_accum >= sim_dt)
        {
            uint64_t start_cycles = __rdtsc();
            simulate(&psys, &E, &P, sim_dt);
            //simulate(&E, &P, sim_dt);
            uint64_t end_cycles = __rdtsc();

            sim_ticks = end_cycles - start_cycles;
            sim_ticks_smooth = sim_ticks_smooth * 0.98f + sim_ticks * 0.02f;
            time_accum -= sim_dt;
        }
        draw(camera, window.width, window.height, &P);

        float fps = 1.0f / dt;

        char buf[64];
        snprintf(buf, 64, "FPS %.3f; SIM %lld cycles (%.0f), particles alive %d",
                fps, sim_ticks, sim_ticks_smooth, E.particles_alive);
        set_window_title(window, buf);

        SwapBuffers((HDC)window.hdc);
    }
    wglMakeCurrent((HDC)window.hdc, nullptr);
    wglDeleteContext((HGLRC)window.glrc);
    DestroyWindow((HWND)window.hwnd);
    return 0;
}

