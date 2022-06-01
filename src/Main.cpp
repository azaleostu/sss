#include "Application.h"

#include <cstdint>
#include <glad/glad.h>

#if INTPTR_MAX != INT64_MAX
#error "build target must be 64 bits"
#endif

class SSSApp : public sss::Application {
public:
  bool init() override {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    return true;
  }

  void beginFrame() override { glClear(GL_COLOR_BUFFER_BIT); }
};

int main(int, char**) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start("sss", 1600, 900);
  return 0;
}
