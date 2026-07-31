# The null Metal renderer

League 16.15 is Metal-only ([AUDIT-16.15.md](AUDIT-16.15.md)): no OpenGL symbols, no
`NSOpenGLContext`, no Cg. Its entire statically visible Metal surface is **8 C symbols and
8 ObjC classes** — `MTLCreateSystemDefaultDevice`, the seven `MTL*Descriptor` classes and
`CAMetalLayer`. Everything else the engine uses (device, queue, command buffer, encoders,
buffers, textures, libraries, pipeline states, drawables) is a *protocol* method reached
through `objc_msgSend`, which means it is invisible to any static analysis and can only be
found by running the game.

The null renderer is the object graph behind those 8+8 symbols, implemented to be
semantically correct about everything except pixels. It draws nothing. It exists so that
16.15 boots, runs its frame loop and reaches `GAMESTATE_GAMELOOP` headless — the same gate
every other version is held to — and so that the set of methods it demands becomes a
written-down spec for a future real Metal backend.

Source: [`stub/src/metal/`](../stub/src/metal/).

## The `MACHOLO_METAL` contract

`MTLCreateSystemDefaultDevice` is the whole switch, and it is **opt-in**:

| `MACHOLO_METAL` | `MTLCreateSystemDefaultDevice` |
|---|---|
| unset, empty, or `0` | `NULL` |
| `null` | the null renderer's device |
| anything else | `NULL`, with a warning on stderr |

Read once, at first call.

Opt-in is a regression rule, not a preference. **16.3 weak-links Metal and takes its
OpenGL path precisely because this function returns `NULL` today.** A device that appeared
unconditionally would silently move a working, verified-on-screen version onto a renderer
that draws nothing, and turn its gate green while showing a black window. So the default
must stay byte-for-byte the old behaviour.

The 8 weak `MTL*`/`CAMetalLayer` class references, by contrast, *are* resolved
unconditionally now: 16.3 never instantiates any of them while the device is `NULL`, so
the classes existing changes nothing for it. Verified — `tools/gate.sh 16.3` and
`tools/gate.sh 14.8` both pass with the default environment.

## Semantic rules

Drawing nothing is easy; not deadlocking is the part that takes care. Five rules carry the
whole design:

1. **Buffers are real memory.** `newBufferWith*` `calloc`s, `-contents` returns that
   pointer and `-length` is exact. The engine writes vertex and constant data straight
   through `-contents` and reads it back; a fake pointer is an immediate crash. `calloc`
   rather than `malloc` because engines routinely read regions they never wrote.
2. **Textures keep their shape, not their pixels.** A texture stores its descriptor's
   dimensions, format, mip count and usage so the engine's own bookkeeping stays
   consistent. `-replaceRegion:…` drops the upload; `-getBytes:…` writes zeros, so
   readback is deterministic black rather than uninitialised caller memory.
3. **`-commit` completes synchronously.** This is the load-bearing rule. A Metal engine
   paces itself with a `dispatch_semaphore` taken before encoding a frame and signalled
   from `-addCompletedHandler:`; libdispatch here is the real one, so a command buffer
   that never completes hangs the process on the second or third frame. `-commit` walks
   straight to `MTLCommandBufferStatusCompleted` and fires every scheduled and completed
   handler inline before returning. A handler registered *after* completion runs
   immediately. `-waitUntilScheduled`/`-waitUntilCompleted` return at once.
4. **Encoders accept everything and do nothing.** Render, blit, compute and parallel
   encoders are no-ops down to `-endEncoding`.
5. **`-nextDrawable` never blocks and never fails.** On a real device it waits for the
   display pipeline; here it always returns a fresh drawable carrying a null texture sized
   from the layer's `drawableSize`. `-present`/`-presentedTime` record a timestamp and
   nothing else; `-presentDrawable:` on the command buffer is a no-op.

And one rule about shaders: **libraries and functions are opaque success objects.**
`newLibraryWith{Data,File,URL,Source}:error:` accepts anything, `-newFunctionWithName:`
returns an object carrying the name, and pipeline creation *never fails* (`error` out-param
set to `nil`) — a pipeline failure is the one thing an engine treats as fatal. No AIR or
metallib byte is parsed; that is [PLAN.md](PLAN.md) phase 4's real work, and
[air-census.txt](air-census.txt) is the survey of what it would have to cover.

## How the surface is discovered

`MACHOLONullObject` is the base of every object handed to the game. Unknown selectors are
not errors: `+resolveInstanceMethod:` installs a generic implementation and logs the
selector once, as

```
[metal-surface] -[MTLSamplerDescriptor setMinFilter:]
```

The generic implementation is a property bag — `-setFoo:` stores a bare 64-bit word,
`-foo` returns the word stored for `foo`, anything else returns 0/nil — so descriptors
round-trip their properties without a line of code per property, and no-op encoder calls
cost nothing.

Two things the fallback cannot do, both of which mean "write the method by hand": a method
returning `float`/`double` reads a garbage SSE register, and a method returning a large
struct goes through `objc_msgSend_stret`, whose hidden return pointer the fallback does not
honour. That is why `clearColor`, `clearDepth`, `lodMinClamp`/`lodMaxClamp`, `drawableSize`
and `-maxThreadsPerThreadgroup` are declared explicitly while dozens of enum-valued
properties are not.

Grep a run's log for `[metal-surface]` to get the current frontier.

## The discovered method surface

This is the spec for a real Metal backend: everything 16.15 actually calls, from a full
replay run (boot → `GAMESTATE_GAMELOOP` → `GAMESTATE_ENDGAME`). **~60 methods are
implemented by hand and 63 more were resolved by the property-bag fallback**, for a demanded
surface of roughly 125 selectors across 17 protocol-ish types — against 8 statically visible
symbols.

### `MTLDevice`
`name`, `registryID`, `isLowPower`, `isHeadless`, `isRemovable`, `hasUnifiedMemory`,
`recommendedMaxWorkingSetSize`, `maxBufferLength`, `currentAllocatedSize`,
`maxThreadsPerThreadgroup`, `maxThreadgroupMemoryLength`, `supportsFeatureSet:`,
`supportsFamily:`, `supportsTextureSampleCount:`, `isDepth24Stencil8PixelFormatSupported`,
`newCommandQueue`, `newCommandQueueWithMaxCommandBufferCount:`,
`newLibraryWith{Data,File,URL,Source:options:}:error:`, `newDefaultLibrary`,
`newRenderPipelineStateWithDescriptor:error:` (+ `options:reflection:` form),
`newComputePipelineStateWithFunction:error:`, `newDepthStencilStateWithDescriptor:`,
`newSamplerStateWithDescriptor:`, `newBufferWithLength:options:`,
`newBufferWithBytes:length:options:`, `newBufferWithBytesNoCopy:length:options:deallocator:`,
`newTextureWithDescriptor:`, `newFence`, `newEvent`, `newSharedEvent`.

### `MTLCommandQueue` / `MTLCommandBuffer`
`commandBuffer`, `commandBufferWithUnretainedReferences`; `commit`, `enqueue`,
`waitUntilScheduled`, `waitUntilCompleted`, `status`, `error`, `addScheduledHandler:`,
`addCompletedHandler:`, `GPUStartTime`, `GPUEndTime`, `kernelStartTime`, `kernelEndTime`,
`renderCommandEncoderWithDescriptor:`, `parallelRenderCommandEncoderWithDescriptor:`,
`blitCommandEncoder`, `computeCommandEncoder`, `presentDrawable:` (+ `atTime:` /
`afterMinimumDuration:` forms), `pushDebugGroup:`, `popDebugGroup`.

### `MTLRenderCommandEncoder` (all reached via the fallback)
`setRenderPipelineState:`, `setDepthStencilState:`, `setCullMode:`,
`setFrontFacingWinding:`, `setScissorRect:`, `setDepthBias:slopeScale:clamp:`,
`setVertexBuffer:offset:atIndex:`, `setVertexBufferOffset:atIndex:`,
`setFragmentSamplerState:atIndex:`, `drawPrimitives:vertexStart:vertexCount:`,
`drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset:instanceCount:baseVertex:baseInstance:`,
`endEncoding`.

Notably absent from a full replay: no compute dispatch, no `setFragmentTexture:`/
`setVertexTexture:`, no `setFragmentBytes:`/`setVertexBytes:`. Both match the AIR census —
the corpus is vertex/fragment only, no compute.

### `MTLBlitCommandEncoder`
`copyFromBuffer:sourceOffset:toBuffer:destinationOffset:size:`,
`copyFromBuffer:sourceOffset:sourceBytesPerRow:sourceBytesPerImage:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:`,
`endEncoding`. That second one is the whole texture upload path: 16.15 stages pixels
through a buffer rather than `-replaceRegion:`.

### `MTLBuffer` / `MTLTexture`
Buffer: `contents`, `length`, `allocatedSize`, `didModifyRange:`, `setPurgeableState:`,
`addDebugMarker:range:`, `removeAllDebugMarkers`, `device`.
Texture: `textureType`, `pixelFormat`, `width`, `height`, `depth`, `mipmapLevelCount`,
`sampleCount`, `arrayLength`, `usage`, `storageMode`, `cpuCacheMode`, `isFramebufferOnly`,
`parentTexture`, `buffer`, `rootResource`, `replaceRegion:…` (both forms), `getBytes:…`
(both forms), `newTextureViewWithPixelFormat:`.

### `MTLLibrary` / `MTLFunction` / pipeline & state objects
`newFunctionWithName:` (+ `constantValues:error:`), `functionNames`; `name`,
`functionType`, `vertexAttributes`, `stageInputAttributes`; `maxTotalThreadsPerThreadgroup`,
`threadExecutionWidth`, `staticThreadgroupMemoryLength`, `device`, `label`.

### Descriptors
- **`MTLRenderPassDescriptor`** — `+renderPassDescriptor`, `colorAttachments`
  (indexed subscript), `depthAttachment`, `stencilAttachment`; per attachment `texture`,
  `resolveTexture`, `setLevel:`, `setLoadAction:`, `setStoreAction:`, `clearColor`,
  `clearDepth`, `clearStencil`.
- **`MTLRenderPipelineDescriptor`** — `vertexFunction`, `fragmentFunction`,
  `vertexDescriptor`, `colorAttachments`, `setDepthAttachmentPixelFormat:`,
  `setStencilAttachmentPixelFormat:`, `reset`; per colour attachment `pixelFormat`,
  `setBlendingEnabled:`, `setSourceRGBBlendFactor:`, `setDestinationRGBBlendFactor:`,
  `setRgbBlendOperation:`, `setSourceAlphaBlendFactor:`,
  `setDestinationAlphaBlendFactor:`, `setAlphaBlendOperation:`, `setWriteMask:`.
- **`MTLTextureDescriptor`** — `+texture2DDescriptorWithPixelFormat:width:height:mipmapped:`,
  `+textureCubeDescriptorWithPixelFormat:size:mipmapped:`,
  `+textureBufferDescriptorWithPixelFormat:width:resourceOptions:usage:`, and the full
  shape property set.
- **`MTLSamplerDescriptor`** — `setMinFilter:`, `setMagFilter:`, `setMipFilter:`,
  `setSAddressMode:`, `setTAddressMode:`, `setRAddressMode:`, `setMaxAnisotropy:`,
  `setCompareFunction:`, `setBorderColor:`, `lodMinClamp`, `lodMaxClamp`.
- **`MTLDepthStencilDescriptor`** — `setDepthCompareFunction:`, `setDepthWriteEnabled:`,
  `frontFaceStencil`, `backFaceStencil` (both default-constructed, never nil).
- **`MTLStencilDescriptor`** — `setStencilCompareFunction:`, `setStencilFailureOperation:`,
  `setDepthFailureOperation:`, `setDepthStencilPassOperation:`, `setReadMask:`,
  `setWriteMask:`.
- **`MTLVertexDescriptor`** — `+vertexDescriptor`, `attributes`, `layouts`, `reset`; per
  attribute `format`/`offset`/`bufferIndex` (read back as well as written), per layout
  `stride`/`stepFunction`/`stepRate` (likewise).

### `CAMetalLayer` / `CAMetalDrawable`
`+layer`, `device`, `pixelFormat`, `framebufferOnly`, `drawableSize`, `contentsScale`,
`frame`, `bounds`, `setMaximumDrawableCount:`, `setDisplaySyncEnabled:`, `nextDrawable`,
plus the CALayer odds and ends a view wiring path touches. Drawable: `texture`, `layer`,
`present`, `presentAtTime:`, `presentAfterMinimumDuration:`, `presentedTime`,
`addPresentedHandler:`, `drawableID`.

## 16.15 status

**Boots headless to `GAMESTATE_GAMELOOP` and plays the 44 KB replay stub out to
`GAMESTATE_ENDGAME`** — full state chain `GAMESTATE_PREGAME` → `GAMESTATE_SPAWN` →
`GAMESTATE_GAMELOOP` → `GAMESTATE_ENDGAME`, no crash, no hang. The engine logs
`Using X3D Platform: Metal` and `Metal Device GPU: macholo null`.

```
cd ~/Game/16.15/LeagueofLegends.app/Contents/MacOS && rm -f ../SOFT_REPAIR
DISPLAY=:9 MACHOLO_METAL=null <build>/macholo ./LeagueofLegends ~/Game/EUN1-3984273452.rofl
```

This was the first time 16.15 had ever been run. Three non-Metal gaps turned up on the way
and were fixed in their own modules, not here:

- `_availability_version_check` (`stub/src/libsystem/stub_xpc.c`) — clang's `@available()`
  backend, previously a `panic`. 16.15's Metal path is the first code in any audited
  version to reach one. Answers against the 10.15.6 that `stub_sys.c` already reports for
  `kern.osproductversion`.
- `dispatch_data_create` (`stub/src/libsystem/stub_dispatch.m`, was `.c`) — used only to
  wrap a metallib for `-newLibraryWithData:error:`. Returns an `NSData`, because on Darwin
  `dispatch_data_t` is an ObjC object and the caller's ARC releases it with `objc_release`.
- `-[NSView setWantsLayer:]`/`layer`/`setLayer:` (`stub/src/appkit/NSView.m`) — the view
  has to hold the `CAMetalLayer` and hand it back.

What is *not* covered: nothing renders, so there is no picture and no way to tell a
correct frame from a wrong one. Everything visual remains phase 4's real job.

### A build landmine this bring-up walked into

Midway through, every macholo binary started aborting before `main` with

```
Version 2 Objective-C ABI may not be mixed with earlier versions.
```

Nothing to do with Metal. libobjc2's `.m` files had flipped from language OBJC to language
**C** in CMake's extension→language map — the exact hazard
`vendor/gnustep-base-cmake/CMakeLists.txt` already warns about and pins against for its own
sources. Upstream libobjc2 applies `-fobjc-runtime=gnustep-2.0` through a
`$<COMPILE_LANGUAGE:OBJC>` generator expression, so when the language flips the flag
silently disappears and `associate.m`, `properties.m` and `fast_paths.m` compile against the
legacy GCC runtime. Their `__objc_exec_class` constructors set the runtime to `LegacyABI`
before libobjc2's own `__objc_load` constructor runs, and `__objc_load` aborts.

Diagnosis path, for next time: the message comes from `vendor/libobjc2/loader.c`, and
`nm -u` over `build/vendor/libobjc2/CMakeFiles/objc.dir/*.o | grep __objc_exec_class` names
the offending translation units directly.

`vendor/CMakeLists.txt` now pins `LANGUAGE OBJC`/`OBJCXX` on libobjc2's sources after
`add_subdirectory`, which is where it has to go — the vendored `CMakeLists.txt` must stay
byte-identical to upstream v2.3. The vendored build (`vendor/`, the default since
`924f95b7c`) is otherwise fine and is what everything above was built and verified with.
