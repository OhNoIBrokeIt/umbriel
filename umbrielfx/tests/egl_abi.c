// wlroots' multi-GPU renderer reads umbrielfx's EGL objects directly. Exercise
// that shared layout through the linked library without opening a GPU. Missing
// BGRA support deliberately stops initialization after its EGL import check.
//
// The test replaces wlr_egl_make_current so no GPU is needed. A wlroots linked
// with -Bsymbolic-functions (Ubuntu's default) or built with LTO binds that
// call internally, and the real function fails on the fake display before the
// import check. That build cannot be probed this way, so the test skips.
#include "render/egl.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/render/gles2.h>
#include <wlr/util/log.h>

static bool import_rejected;
static bool bgra_rejected;
static bool make_current_bound_internally;

enum check_result {
  CHECK_PASS,
  CHECK_FAIL,
  CHECK_SKIP,
};

static void capture_log(enum wlr_log_importance importance, const char* format, va_list args) {
  if (importance != WLR_ERROR) {
    return;
  }
  char message[512];
  vsnprintf(message, sizeof(message), format, args);
  import_rejected |= strstr(message, "EGL_EXT_image_dma_buf_import not supported") != NULL;
  bgra_rejected |= strstr(message, "BGRA8888 format not supported by GLES2") != NULL;
  make_current_bound_internally |= strstr(message, "eglMakeCurrent failed") != NULL;
}

bool wlr_egl_make_current(struct wlr_egl* egl, struct wlr_egl_context* save_context) {
  (void)egl;
  (void)save_context;
  return true;
}

const GLubyte* glGetString(GLenum name) {
  (void)name;
  return (const GLubyte*)"";
}

static enum check_result check_import_flag(bool supported) {
  struct wlr_egl egl = {0};
  egl.exts.EXT_image_dma_buf_import = supported;
  import_rejected = false;
  bgra_rejected = false;
  make_current_bound_internally = false;
  struct wlr_renderer* renderer = wlr_gles2_renderer_create(&egl);
  if (renderer == NULL && make_current_bound_internally && !import_rejected && !bgra_rejected) {
    fprintf(
        stderr, "SKIP: linked wlroots calls its own wlr_egl_make_current, so the EGL layout cannot be probed\n"
    );
    return CHECK_SKIP;
  }
  if (renderer != NULL || import_rejected == supported || bgra_rejected != supported) {
    fprintf(
        stderr,
        "FAIL: linked wlroots misread EGL import flag %d "
        "(import rejected: %d, BGRA rejected: %d)\n",
        supported, import_rejected, bgra_rejected
    );
    return CHECK_FAIL;
  }
  return CHECK_PASS;
}

int main(void) {
  wlr_log_init(WLR_ERROR, capture_log);
  const bool flags[] = {false, true};
  for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
    switch (check_import_flag(flags[i])) {
    case CHECK_PASS:
      break;
    case CHECK_FAIL:
      return EXIT_FAILURE;
    case CHECK_SKIP:
      return 77;
    }
  }
  return EXIT_SUCCESS;
}
