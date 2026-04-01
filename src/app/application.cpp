#include "application.h"

#include "editor_shell.h"
#include "theme.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_opengl3_loader.h"
#include "GLFW/glfw3.h"
#include "imgui.h"

namespace
{
  void glfw_error_callback(
    int32_t error,
    const char* description)
  {
    (void)error;
    (void)description;
  }
}

app::application::application()
  : m_glfw_initialized(false)
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

  while (!glfwWindowShouldClose(m_window))
  {
    glfwPollEvents();
    render_frame();
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

  m_window = glfwCreateWindow(1600, 980, "mIDE", nullptr, nullptr);
  if (m_window == nullptr)
  {
    glfwTerminate();
    m_glfw_initialized = false;
    return false;
  }

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
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  io.IniFilename = "imgui.ini";

  app::theme::configure();
  app::theme::apply();

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
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }

  if (m_glfw_initialized)
  {
    glfwTerminate();
    m_glfw_initialized = false;
  }
}
