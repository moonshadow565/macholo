// CFNotificationCenter: the game registers a handful of observers (e.g. workspace/app
// lifecycle) but nothing under macholo ever posts to these centers, so a single opaque
// non-NULL center plus no-op add/remove is enough to get past startup. Returning NULL
// here would make the game's CFNotificationCenterAddObserver dereference it and crash.
static char macholo_cf_local_center;
static char macholo_cf_distributed_center;

void *CFNotificationCenterGetLocalCenter(void) {
  return &macholo_cf_local_center;
}

void *CFNotificationCenterGetDistributedCenter(void) {
  return &macholo_cf_distributed_center;
}

void CFNotificationCenterAddObserver(void *center, const void *observer, void *callBack, const void *name,
                                     const void *object, long suspensionBehavior) {
  // NOTE: no center under macholo ever posts, so registration is a no-op.
}

void CFNotificationCenterRemoveObserver(void *center, const void *observer, const void *name, const void *object) {
  // NOTE: paired with the no-op AddObserver above.
}
