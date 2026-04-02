#include "application.h"

// clang-format off
#include "editor_shell.h"
#include "theme.h"

#include "GLFW/glfw3.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_opengl3_loader.h"
#include "imgui.h"
// clang-format on

#include <chrono>
#include <thread>

namespace
{
  constexpr float MIN_DPI_SCALE = 1.0f;
  constexpr float MAX_DPI_SCALE = 3.0f;
  constexpr double TARGET_FRAME_RATE = 15.0;
  const std::chrono::duration<double> TARGET_FRAME_INTERVAL(1.0 / TARGET_FRAME_RATE);

  void glfw_error_callback(int32_t error, const char* description)
  {
    (void) error;
    (void) description;
  }

  float sanitize_dpi_scale(float x_scale, float y_scale)
  {
    const float larger_scale = (x_scale > y_scale) ? x_scale : y_scale;
    if (larger_scale <= 0.0f)
    {
      return 1.0f;
    }

    if (larger_scale < MIN_DPI_SCALE)
    {
      return MIN_DPI_SCALE;
    }

    if (larger_scale > MAX_DPI_SCALE)
    {
      return MAX_DPI_SCALE;
    }

    return larger_scale;
  }
}

app::application::application()
  : m_glfw_initialized(false)
  , m_dpi_scale(1.0f)
  , m_window(nullptr)
  , m_editor_shell(nullptr)
  , m_glsl_version("#version 330")
{
}

app::application::~application()
{
  shutdown();
}

int32_t app::application::run()
{
  if (!init_window())
  {
    return 1;
  }

  if (!init_imgui())
  {
    return 1;
  }

  std::chrono::steady_clock::time_point next_frame_time = std::chrono::steady_clock::now();
  while (!glfwWindowShouldClose(m_window))
  {
    glfwPollEvents();
    render_frame();

    next_frame_time += std::chrono::duration_cast<std::chrono::steady_clock::duration>(TARGET_FRAME_INTERVAL);

    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (next_frame_time > now)
    {
      std::this_thread::sleep_until(next_frame_time);
    }
    else
    {
      next_frame_time = now;
    }
  }

  return 0;
}

bool app::application::init_window()
{
  glfwSetErrorCallback(glfw_error_callback);

  if (glfwInit() == GLFW_FALSE)
  {
    return false;
  }

  m_glfw_initialized = true;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

  m_window = glfwCreateWindow(1600, 980, "mIDE", nullptr, nullptr);
  if (m_window == nullptr)
  {
    glfwTerminate();
    m_glfw_initialized = false;
    return false;
  }

  glfwSetWindowUserPointer(m_window, this);
  glfwSetWindowContentScaleCallback(m_window, glfw_window_content_scale_callback);

  float window_x_scale = 1.0f;
  float window_y_scale = 1.0f;
  glfwGetWindowContentScale(m_window, &window_x_scale, &window_y_scale);
  m_dpi_scale = sanitize_dpi_scale(window_x_scale, window_y_scale);

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(1);
  return true;
}

bool app::application::init_imgui()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigDpiScaleFonts = true;
  io.ConfigDpiScaleViewports = true;
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  io.IniFilename = "imgui.ini";

  app::theme::configure();
  update_dpi_scale(m_dpi_scale, m_dpi_scale);

  if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
  {
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init(m_glsl_version))
  {
    return false;
  }

  m_editor_shell = new editor_shell();
  return true;
}

void app::application::glfw_window_content_scale_callback(GLFWwindow* window, float x_scale, float y_scale)
{
  app::application* instance = static_cast<app::application*>(glfwGetWindowUserPointer(window));
  if (instance != nullptr)
  {
    instance->update_dpi_scale(x_scale, y_scale);
  }
}

void app::application::update_dpi_scale(float x_scale, float y_scale)
{
  m_dpi_scale = sanitize_dpi_scale(x_scale, y_scale);

  if (ImGui::GetCurrentContext() == nullptr)
  {
    return;
  }

  app::theme::apply(m_dpi_scale);
}

void app::application::render_frame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  m_editor_shell->draw();

  ImGui::Render();

  int32_t framebuffer_width = 0;
  int32_t framebuffer_height = 0;
  glfwGetFramebufferSize(m_window, &framebuffer_width, &framebuffer_height);
  glViewport(0, 0, framebuffer_width, framebuffer_height);
  glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  ImGuiIO& io = ImGui::GetIO();
  if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
  {
    GLFWwindow* previous_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(previous_context);
  }

  glfwSwapBuffers(m_window);
}

void app::application::shutdown()
{
  if (m_editor_shell != nullptr)
  {
    delete m_editor_shell;
    m_editor_shell = nullptr;
  }

  if (ImGui::GetCurrentContext() != nullptr)
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  if (m_window != nullptr)
  {
    glfwSetWindowContentScaleCallback(m_window, nullptr);
    glfwSetWindowUserPointer(m_window, nullptr);
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }

  if (m_glfw_initialized)
  {
    glfwTerminate();
    m_glfw_initialized = false;
  }
}
