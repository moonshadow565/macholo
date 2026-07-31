#include <common.h>
#include <math.h>

extern float exp10f (float f);
extern double exp10 (double f);

double $darwin$__exp10(double x) {
  return exp10(x);
}

float $darwin$__exp10f(float x) {
  // FIXME: is this correct?
  return exp10f(x);
}
