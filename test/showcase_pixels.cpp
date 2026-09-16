#include <raylib.h>

#include <cmath>
#include <cstdio>

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  SetTraceLogLevel(LOG_NONE);
  Image actual = LoadImage(argv[1]);
  Image golden = LoadImage(argv[2]);
  if (!actual.data || !golden.data || actual.width < 1280 || actual.height < 720 ||
      actual.width * 9 != actual.height * 16 || golden.width * 9 != golden.height * 16)
    return 1;
  // WHY: macOS captures Retina pixels; Linux captures logical window pixels.
  ImageResize(&actual, 1280, 720);
  ImageResize(&golden, 1280, 720);
  Color* a = LoadImageColors(actual);
  Color* b = LoadImageColors(golden);
  const int pixels = actual.width * actual.height;
  int changed = 0;
  for (int i = 0; i < pixels; ++i) {
    if (std::abs(a[i].r - b[i].r) > 5 || std::abs(a[i].g - b[i].g) > 5 ||
        std::abs(a[i].b - b[i].b) > 5)
      ++changed;
  }
  const double fraction = static_cast<double>(changed) / pixels;
  std::fprintf(stderr, "pixels changed: %.3f%% (limit 2%%)\n", 100 * fraction);
  UnloadImageColors(a);
  UnloadImageColors(b);
  UnloadImage(actual);
  UnloadImage(golden);
  return fraction <= .02 ? 0 : 1;
}
