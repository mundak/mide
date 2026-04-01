#pragma once

#include <cstdint>

struct GLFWwindow;

namespace app
{
  class editor_shell;

  class application
  {
  public:
    application();
    ~application();

    application(const application&) = delete;
    application& operator=(const application&) = delete;

    int32_t run();

  private:
    bool init_window();
    bool init_imgui();
    void shutdown();
    void render_frame();

    bool m_glfw_initialized;
    GLFWwindow* m_window;
    editor_shell* m_editor_shell;
    const char* m_glsl_version;
  };
}
