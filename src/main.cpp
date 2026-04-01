#include "app/application.h"

#include <cstdint>
#include <exception>

int32_t main()
{
  try
  {
    app::application instance;
    return instance.run();
  }
  catch (const std::exception& exception)
  {
    (void)exception;
    return 1;
  }
}
