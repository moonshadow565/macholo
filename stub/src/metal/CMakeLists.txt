# Metal — the null renderer (docs/METAL-NULL.md). Its own module because this is the seam
# where a real 16.15 Metal backend grows; CAMetalLayer lives here too rather than in a
# one-class QuartzCore module, since it is only ever used as the Metal swapchain.
add_library(stub-metal OBJECT
    MetalTypes.h
    MACHOLONullObject.h
    MACHOLONullObject.m
    MTLDescriptors.h
    MTLDescriptors.m
    MACHOLONullResources.h
    MACHOLONullResources.m
    MACHOLONullCommand.h
    MACHOLONullCommand.m
    MACHOLONullDevice.h
    MACHOLONullDevice.m
    CAMetalLayer.h
    CAMetalLayer.m
    MetalStubs.c
)
target_include_directories(stub-metal PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-metal PRIVATE stub-flags GNUStep sdl2)
