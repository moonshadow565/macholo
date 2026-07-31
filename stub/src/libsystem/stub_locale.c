#define _GNU_SOURCE  // strtod_l
#include <common.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

// Track 3 (docs/surface-audit/VERDICTS.md): LC_* categories are permuted between
// Darwin (ALL=0 COLLATE=1 CTYPE=2 MONETARY=3 NUMERIC=4 TIME=5 MESSAGES=6) and
// glibc (CTYPE=0 NUMERIC=1 TIME=2 COLLATE=3 MONETARY=4 MESSAGES=5 ALL=6), so
// setlocale(LC_ALL, "") was only setting LC_CTYPE. newlocale masks are
// (1 << category) on both sides, permuted the same way.

static int lc_category_to_gnu(int c) {
  switch (c) {
    case 0: return LC_ALL;
    case 1: return LC_COLLATE;
    case 2: return LC_CTYPE;
    case 3: return LC_MONETARY;
    case 4: return LC_NUMERIC;
    case 5: return LC_TIME;
    case 6: return LC_MESSAGES;
    default: return -1;
  }
}

char *$darwin$setlocale(int category, const char *locale) {
  int l = lc_category_to_gnu(category);
  if (l < 0) return NULL;
  return setlocale(l, locale);
}

void *$darwin$newlocale(int mask, const char *locale, void *base) {
  int lmask = 0;
  for (int c = 0; c <= 6; c++)
    if (mask & (1 << c)) lmask |= 1 << lc_category_to_gnu(c);
  return newlocale(lmask, locale, base);
}

// Darwin normalises loc == NULL to the current locale; glibc dereferences it.
// A locale_t the guest holds came from the newlocale shim above, so it is a
// valid glibc handle. (Sibling precedent: $darwin$vsnprintf_l.)
double $darwin$strtod_l(const char *s, char **end, void *loc) {
  if (!loc) return strtod(s, end);
  return strtod_l(s, end, (locale_t)loc);
}

// struct lconv: the ten leading char* fields agree; the trailing char fields are
// permuted (Darwin groups int_p_cs/int_n_cs, glibc groups int_p_cs/int_p_sep).
// Field-by-field copy into a Darwin-layout struct sidesteps the ordering.
struct $darwin$lconv {
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;
  char p_cs_precedes;
  char p_sep_by_space;
  char n_cs_precedes;
  char n_sep_by_space;
  char p_sign_posn;
  char n_sign_posn;
  char int_p_cs_precedes;
  char int_n_cs_precedes;
  char int_p_sep_by_space;
  char int_n_sep_by_space;
  char int_p_sign_posn;
  char int_n_sign_posn;
};

struct $darwin$lconv *$darwin$localeconv(void) {
  static _Thread_local struct $darwin$lconv out __attribute__((tls_model("initial-exec")));
  struct lconv *in = localeconv();
  out.decimal_point = in->decimal_point;
  out.thousands_sep = in->thousands_sep;
  out.grouping = in->grouping;
  out.int_curr_symbol = in->int_curr_symbol;
  out.currency_symbol = in->currency_symbol;
  out.mon_decimal_point = in->mon_decimal_point;
  out.mon_thousands_sep = in->mon_thousands_sep;
  out.mon_grouping = in->mon_grouping;
  out.positive_sign = in->positive_sign;
  out.negative_sign = in->negative_sign;
  out.int_frac_digits = in->int_frac_digits;
  out.frac_digits = in->frac_digits;
  out.p_cs_precedes = in->p_cs_precedes;
  out.p_sep_by_space = in->p_sep_by_space;
  out.n_cs_precedes = in->n_cs_precedes;
  out.n_sep_by_space = in->n_sep_by_space;
  out.p_sign_posn = in->p_sign_posn;
  out.n_sign_posn = in->n_sign_posn;
  out.int_p_cs_precedes = in->int_p_cs_precedes;
  out.int_n_cs_precedes = in->int_n_cs_precedes;
  out.int_p_sep_by_space = in->int_p_sep_by_space;
  out.int_n_sep_by_space = in->int_n_sep_by_space;
  out.int_p_sign_posn = in->int_p_sign_posn;
  out.int_n_sign_posn = in->int_n_sign_posn;
  return &out;
}
