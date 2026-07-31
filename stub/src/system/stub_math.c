#include <common.h>
#include <math.h>

extern float exp10f (float f);
extern double exp10 (double f);

struct float2 {
  float val[2];
};

double $darwin$__exp10(double x) {
  return exp10(x);
}

float $darwin$__exp10f(float x) {
  // FIXME: is this correct?
  return exp10f(x);
}

struct float2 $darwin$__sincosf_stret(float x) {
  // TODO: is this correct?
  return (struct float2 const){sinf(x), cosf(x)};
}

struct double2 {
  double val[2];
};

// Double-precision counterpart: returned in xmm0/xmm1, same as the float pair above.
struct double2 $darwin$__sincos_stret(double x) {
  return (struct double2 const){sin(x), cos(x)};
}
