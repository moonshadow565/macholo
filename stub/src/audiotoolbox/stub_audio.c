#include <common.h>

// 'who?' -- CoreAudio's kAudioHardwareUnknownPropertyError. Reporting this for every
// property query is how a machine with no usable audio hardware answers, which is
// exactly our situation (SDL runs on the dummy driver). The client then stops asking
// instead of trapping.
#define kAudioHardwareUnknownPropertyError 0x77686F3F

int NewAUGraph() {
  // FIXME: implement NewAUGraph
  panic("not implemented NewAUGraph");
  return 0;
}

int DisposeAUGraph() {
  // FIXME: implement DisposeAUGraph
  panic("not implemented DisposeAUGraph");
  return 0;
}

int AudioObjectGetPropertyData(uint32_t objectID, void const *address, uint32_t qualifierDataSize,
                               void const *qualifierData, uint32_t *ioDataSize, void *outData) {
  if (ioDataSize) *ioDataSize = 0;
  return kAudioHardwareUnknownPropertyError;
}

int AudioObjectGetPropertyDataSize(uint32_t objectID, void const *address, uint32_t qualifierDataSize,
                                   void const *qualifierData, uint32_t *outDataSize) {
  if (outDataSize) *outDataSize = 0;
  return kAudioHardwareUnknownPropertyError;
}

int AudioObjectSetPropertyData(uint32_t objectID, void const *address, uint32_t qualifierDataSize,
                               void const *qualifierData, uint32_t dataSize, void const *data) {
  return kAudioHardwareUnknownPropertyError;
}

int AudioOutputUnitStop() {
  // FIXME: implement AudioOutputUnitStop
  panic("not implemented AudioOutputUnitStop");
  return 0;
}

int AudioUnitSetParameter() {
  // FIXME: implement AudioUnitSetParameter
  panic("not implemented AudioUnitSetParameter");
  return 0;
}

int AudioUnitSetProperty() {
  // FIXME: implement AudioUnitSetProperty
  panic("not implemented AudioUnitSetProperty");
  return 0;
}

int AudioUnitUninitialize() {
  // FIXME: implement AudioUnitUninitialize
  panic("not implemented AudioUnitUninitialize");
  return 0;
}

int AUGraphAddNode() {
  // FIXME: implement AUGraphAddNode
  panic("not implemented AUGraphAddNode");
  return 0;
}

int AUGraphClose() {
  // FIXME: implement AUGraphClose
  panic("not implemented AUGraphClose");
  return 0;
}

int AUGraphConnectNodeInput() {
  // FIXME: implement AUGraphConnectNodeInput
  panic("not implemented AUGraphConnectNodeInput");
  return 0;
}

int AUGraphInitialize() {
  // FIXME: implement AUGraphInitialize
  panic("not implemented AUGraphInitialize");
  return 0;
}

int AUGraphNodeInfo() {
  // FIXME: implement AUGraphNodeInfo
  panic("not implemented AUGraphNodeInfo");
  return 0;
}

int AUGraphOpen() {
  // FIXME: implement AUGraphOpen
  panic("not implemented AUGraphOpen");
  return 0;
}

int AUGraphSetNodeInputCallback() {
  // FIXME: implement AUGraphSetNodeInputCallback
  panic("not implemented AUGraphSetNodeInputCallback");
  return 0;
}

int AUGraphStart() {
  // FIXME: implement AUGraphStart
  panic("not implemented AUGraphStart");
  return 0;
}

int AUGraphStop() {
  // FIXME: implement AUGraphStop
  panic("not implemented AUGraphStop");
  return 0;
}
