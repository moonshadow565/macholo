#include <math.h>

struct float2 {
  float val[2];
};

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
