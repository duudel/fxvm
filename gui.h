
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_opengl2.h"

#include <windows.h>

void gui_init(HWND hwnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    //io.BackendFlags |= ImGuiBackendFlags_HasSet;
    io.BackendPlatformName = "imgui_win32";

    io.ImeWindowHandle = hwnd;

    ImGui_ImplOpenGL2_Init();
}

void gui_deinit()
{
    ImGui_ImplOpenGL2_Shutdown();
    ImGui::DestroyContext();
}

bool ImGui_ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

void ImGui_ImplWin32_UpdateMousePos(HWND hwnd)
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        ::ClientToScreen(hwnd, &pos);
        ::SetCursorPos(pos.x, pos.y);
    }

    // Set mouse position
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    POINT pos;
    if (HWND active_window = ::GetForegroundWindow())
        if (active_window == hwnd || ::IsChild(active_window, hwnd))
            if (::GetCursorPos(&pos) && ::ScreenToClient(hwnd, &pos))
                io.MousePos = ImVec2((float)pos.x, (float)pos.y);
}

void update_mouse_pos_and_cursor(HWND hwnd)
{
    ImGui_ImplWin32_UpdateMouseCursor();
    ImGui_ImplWin32_UpdateMousePos(hwnd);
}

void gui_new_frame(HWND hwnd, int win_width, int win_height)
{
    ImGui_ImplOpenGL2_NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    RECT rect;
    ::GetClientRect(hwnd, &rect);

    (void)win_width;
    (void)win_height;
    //float w = win_width;
    //float h = win_height;
    float disp_width = rect.right - rect.left;
    float disp_height = rect.bottom - rect.top;
    io.DisplaySize = ImVec2(disp_width, disp_height);
    //io.DisplayFramebufferScale = ImVec2(w > 0.0f ? (disp_width / w) : 0.0f, h > 0.0f ? (disp_height / h) : 0.0f);
    io.DeltaTime = 1.0f / 144.0f;

    update_mouse_pos_and_cursor(hwnd);

    ImGui::NewFrame();
}

void gui_mouse_down(int button, bool down)
{
    ImGuiIO &io = ImGui::GetIO();
    io.MouseDown[button] = down;
}

bool gui_want_capture_mouse()
{
    ImGuiIO &io = ImGui::GetIO();
    return io.WantCaptureMouse;
}
