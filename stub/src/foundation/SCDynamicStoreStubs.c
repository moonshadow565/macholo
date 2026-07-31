#include "CoreFoundation/CFDictionary.h"

// Neither CF nor NS surface (SystemConfiguration.framework's SC* API); kept in
// foundation/ per the Misc.m dissolution's leftover bucket. See docs/REFACTOR.md
// Stage 2b report.

CFDictionaryRef SCDynamicStoreCopyProxies() {
  // https://github.com/curl/curl/blob/1ccf1cd9936dfa382fe1f061b639de20170bc8ea/lib/macos.c#L47
  // FIXME: SCDynamicStoreCopyProxies
  // panic("SCDynamicStoreCopyProxies");
  return NULL;
}
