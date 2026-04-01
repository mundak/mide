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
    static void glfw_window_content_scale_callback(GLFWwindow* window, float x_scale, float y_scale);

    bool init_window();
    bool init_imgui();
    void update_dpi_scale(float x_scale, float y_scale);
    void shutdown();
    void render_frame();

    bool m_glfw_initialized;
    float m_dpi_scale;
    GLFWwindow* m_window;
    editor_shell* m_editor_shell;
    const char* m_glsl_version;
  };
}
