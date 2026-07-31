# Hand-written CMake build of the vendored gnustep-base (libs-base base-1_29_0).
#
# Replaces gnustep-make + autoconf.  Two rules govern this file:
#
#  1. The vendored sources under ../gnustep-base are NOT modified to suit CMake.
#     Anything configure used to generate lives frozen under
#     ../gnustep-base-config/<triple>/ and is put on the include path in front of
#     the vendored headers.
#  2. The source list, the per-subproject flag sets and the link line below are
#     transcribed verbatim from a `make messages=yes` run of deps/Containerfile's
#     step 3 (gnustep-make, ng-gnu-gnu, clang 22, Fedora 44).  Do not "tidy" them
#     without re-running that build and diffing.
#
# Deliberately NOT built (nothing in macholo links them; see docs/GNUSTEP-BUILD.md):
#   Tools/ (autogsdoc, defaults, gdnc, gdomap, pl*, xmlparse, HTMLLinker, ...),
#   Tools/make_strings, Tests/, Examples/, Documentation/, the ObjectiveC2
#   compatibility subproject (OBJC2RUNTIME=1 with libobjc2, so gnustep-make did
#   not build it either), and the win32 subproject.
# Everything that ended up inside libgnustep-base.so.1.29.0 is built here.

# Only the frozen configure output for this triple exists.  Fail loudly rather
# than compiling the wrong HAVE_*/SIZEOF_* answers: adding a platform means
# re-running deps/Containerfile there and committing a sibling config directory.
if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"))
    message(FATAL_ERROR
        "vendor/gnustep-base-config only has frozen configure output for "
        "x86_64-linux-gnu, not ${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME}. "
        "See docs/GNUSTEP-BUILD.md.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSBASE_XML REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules(GSBASE_TLS REQUIRED IMPORTED_TARGET gnutls)

set(GSBASE_SRC   "${CMAKE_CURRENT_SOURCE_DIR}/../gnustep-base")
set(GSBASE_CFG   "${CMAKE_CURRENT_SOURCE_DIR}/../gnustep-base-config/x86_64-linux-gnu")

# ---------------------------------------------------------------------------
# Source lists -- exactly the objects that went into libgnustep-base.so.1.29.0.
# ---------------------------------------------------------------------------
set(GSBASE_SOURCES
    GSLocale.m
    preface.m
    cifframe.m
    CXXException.m
    GSArray.m
    GSAttributedString.m
    GSBlocks.m
    GSConcreteValue.m
    GSCountedSet.m
    GSDictionary.m
    GSFTPURLHandle.m
    GSFormat.m
    GSHTTPAuthentication.m
    GSHTTPURLHandle.m
    GSICUString.m
    GSOrderedSet.m
    GSPrivateHash.m
    GSQuickSort.m
    GSRunLoopWatcher.m
    GSSet.m
    GSShellSort.m
    GSSocketStream.m
    GSStream.m
    GSString.m
    GSTimSort.m
    GSTLS.m
    GSValue.m
    GSSocksParser/GSSocksParser.m
    GSSocksParser/GSSocksParserPrivate.m
    GSSocksParser/GSSocks4Parser.m
    GSSocksParser/GSSocks5Parser.m
    NSAffineTransform.m
    NSAppleEventDescriptor.m
    NSAppleEventManager.m
    NSAppleScript.m
    NSArchiver.m
    NSArray.m
    NSAssertionHandler.m
    NSAttributedString.m
    NSAutoreleasePool.m
    NSBackgroundActivityScheduler.m
    NSBundle.m
    NSByteCountFormatter.m
    NSCache.m
    NSCachedURLResponse.m
    NSCalendar.m
    NSCalendarDate.m
    NSCallBacks.m
    NSCharacterSet.m
    NSClassDescription.m
    NSCoder.m
    NSCopyObject.m
    NSCountedSet.m
    NSConcreteHashTable.m
    NSConcreteMapTable.m
    NSConnection.m
    NSData.m
    NSDate.m
    NSDateComponentsFormatter.m
    NSDateFormatter.m
    NSDateInterval.m
    NSDateIntervalFormatter.m
    NSDebug.m
    NSDecimal.m
    NSDecimalNumber.m
    NSDictionary.m
    NSDistantObject.m
    NSDistributedLock.m
    NSDistributedNotificationCenter.m
    NSEnergyFormatter.m
    NSEnumerator.m
    NSError.m
    NSException.m
    NSExtensionContext.m
    NSExtensionItem.m
    NSFileCoordinator.m
    NSFileHandle.m
    NSFileManager.m
    NSFileVersion.m
    NSFileWrapper.m
    NSFormatter.m
    NSGarbageCollector.m
    NSGeometry.m
    NSHashTable.m
    NSHFSFileTypes.m
    NSHost.m
    NSHTTPCookie.m
    NSHTTPCookieStorage.m
    NSIndexPath.m
    NSIndexSet.m
    NSInvocation.m
    NSInvocationOperation.m
    NSISO8601DateFormatter.m
    NSItemProvider.m
    NSItemProviderReadingWriting.m
    NSJSONSerialization.m
    NSKeyedArchiver.m
    NSKeyedUnarchiver.m
    NSKeyValueCoding.m
    NSKeyValueObserving.m
    NSLengthFormatter.m
    NSLinguisticTagger.m
    NSLocale.m
    NSLock.m
    NSLog.m
    NSMapTable.m
    NSMassFormatter.m
    NSMeasurementFormatter.m
    NSMeasurement.m
    NSMetadata.m
    NSMetadataAttributes.m
    NSMethodSignature.m
    NSNotification.m
    NSNotificationCenter.m
    NSNotificationQueue.m
    NSNull.m
    NSNumber.m
    NSNumberFormatter.m
    NSObjCRuntime.m
    NSObject.m
    NSObjectScripting.m
    NSObject+NSComparisonMethods.m
    NSOperation.m
    NSOrderedSet.m
    NSOrthography.m
    NSPage.m
    NSPathUtilities.m
    NSPersonNameComponents.m
    NSPersonNameComponentsFormatter.m
    NSPipe.m
    NSPointerArray.m
    NSPointerFunctions.m
    NSConcretePointerFunctions.m
    NSPort.m
    NSPortCoder.m
    NSPortMessage.m
    NSPortNameServer.m
    NSPredicate.m
    NSProcessInfo.m
    NSProgress.m
    NSPropertyList.m
    NSProtocolChecker.m
    NSProxy.m
    NSRange.m
    NSRegularExpression.m
    NSRunLoop.m
    NSScanner.m
    NSScriptClassDescription.m
    NSScriptCoercionHandler.m
    NSScriptCommand.m
    NSScriptCommandDescription.m
    NSScriptExecutionContext.m
    NSScriptKeyValueCoding.m
    NSScriptObjectSpecifiers.m
    NSScriptStandardSuiteCommands.m
    NSScriptSuiteRegistry.m
    NSUnit.m
    NSUserActivity.m
    NSUserScriptTask.m
    NSSerializer.m
    NSSet.m
    NSSocketPort.m
    NSSocketPortNameServer.m
    NSSortDescriptor.m
    NSSpellServer.m
    NSString.m
    NSTask.m
    NSThread.m
    NSTimer.m
    NSTimeZone.m
    NSUbiquitousKeyValueStore.m
    NSUnarchiver.m
    NSUndoManager.m
    NSURL.m
    NSURLAuthenticationChallenge.m
    NSURLCache.m
    NSURLCredential.m
    NSURLConnection.m
    NSURLCredentialStorage.m
    NSURLDownload.m
    NSURLProtectionSpace.m
    NSURLProtocol.m
    NSURLRequest.m
    NSURLResponse.m
    NSTextCheckingResult.m
    NSURLHandle.m
    NSUserDefaults.m
    NSUserNotification.m
    NSUUID.m
    NSValue.m
    NSValueTransformer.m
    NSXMLDocument.m
    NSXMLDTD.m
    NSXMLDTDNode.m
    NSXMLElement.m
    NSXMLNode.m
    NSXMLParser.m
    NSXPCConnection.m
    NSZone.m
    externs.m
    objc-load.m
    GSFileHandle.m
    NSMessagePort.m
    NSMessagePortNameServer.m
    GSEasyHandle.m
    GSHTTPURLProtocol.m
    GSMultiHandle.m
    GSNativeProtocol.m
    GSTaskRegistry.m
    GSTimeoutSource.m
    GSTransferState.m
    GSURLSessionTaskBody.m
    GSURLSessionTaskBodySource.m
    NSURLSession.m
    GSFFIInvocation.m
)
list(TRANSFORM GSBASE_SOURCES PREPEND "${GSBASE_SRC}/Source/")

set(GSBASE_ADDITIONS
    GSTypeEncoding.c
    GSObjCRuntime.m
    GCObject.m
    GCArray.m
    GCDictionary.m
    GSLock.m
    GSMime.m
    GSXML.m
    GSFunctions.m
    GSInsensitiveDictionary.m
    NSArray+GNUstepBase.m
    NSAttributedString+GNUstepBase.m
    NSBundle+GNUstepBase.m
    NSCalendarDate+GNUstepBase.m
    NSData+GNUstepBase.m
    NSDebug+GNUstepBase.m
    NSError+GNUstepBase.m
    NSHashTable+GNUstepBase.m
    NSFileHandle+GNUstepBase.m
    NSLock+GNUstepBase.m
    NSMutableString+GNUstepBase.m
    NSNumber+GNUstepBase.m
    NSObject+GNUstepBase.m
    NSPropertyList+GNUstepBase.m
    NSProcessInfo+GNUstepBase.m
    NSStream+GNUstepBase.m
    NSString+GNUstepBase.m
    NSTask+GNUstepBase.m
    NSThread+GNUstepBase.m
    NSURL+GNUstepBase.m
    Unicode.m
)
list(TRANSFORM GSBASE_ADDITIONS PREPEND "${GSBASE_SRC}/Source/Additions/")

set(GSBASE_UNIX
    GSRunLoopCtxt.m
    NSStream.m
)
list(TRANSFORM GSBASE_UNIX PREPEND "${GSBASE_SRC}/Source/unix/")

# ---------------------------------------------------------------------------
# The library
# ---------------------------------------------------------------------------
add_library(gnustep-base-objs OBJECT ${GSBASE_SOURCES} ${GSBASE_ADDITIONS} ${GSBASE_UNIX})
set_target_properties(gnustep-base-objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Pin the language of every source explicitly.  Both C and OBJC claim the `.m`
# extension in CMake's extension->language map, and which one wins depends on the
# order the languages happen to be (re)initialised: a *second* `cmake -B <dir>`
# over an existing cache flips .m from OBJC to C, which silently drops every
# $<COMPILE_LANGUAGE:OBJC> flag below -- including -fobjc-runtime=gnustep-2.1.
# clang still compiles .m as Objective-C, so the damage shows up only as a
# fragile-ABI mismatch deep in a header. Pinning LANGUAGE makes it deterministic.
set_source_files_properties(${GSBASE_SOURCES} ${GSBASE_ADDITIONS} ${GSBASE_UNIX}
    PROPERTIES LANGUAGE OBJC)
set_source_files_properties("${GSBASE_SRC}/Source/Additions/GSTypeEncoding.c"
    PROPERTIES LANGUAGE C)

# Flags common to Source/, Source/Additions/ and Source/unix/ (from the
# gnustep-make command lines; ADDITIONAL_CFLAGS/OBJCFLAGS are the RELAX_CFLAGS
# set the Containerfile passes to work around clang 22 strictness).
target_compile_definitions(gnustep-base-objs PRIVATE
    GNUSTEP
    GNUSTEP_BASE_LIBRARY=1
    GNUSTEP_RUNTIME=1
    _NONFRAGILE_ABI=1
    _NATIVE_OBJC_EXCEPTIONS
    GSWARN
    GSDIAGNOSE
)
target_compile_options(gnustep-base-objs PRIVATE
    -Wall
    -Wdeclaration-after-statement
    -Wno-import
    -fno-strict-aliasing
    -fexceptions
    -pthread
    # gnustep-make builds gnustep-base at -g -O2 with NDEBUG *unset*; CMake's
    # RelWithDebInfo adds -DNDEBUG, so take it back off to keep assert() live
    # exactly as in the reference build.
    -UNDEBUG
    # clang 22 promoted a pile of long-standing C sloppiness to hard errors;
    # libs-base 1.29 still trips over all of these (deps/Containerfile RELAX_CFLAGS).
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion
    -Wno-error=implicit-function-declaration
    -Wno-error=implicit-int
    -Wno-error=incompatible-function-pointer-types
    # ObjC-only: the nonfragile GNUstep 2.1 ABI is the point of the exercise.
    $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-exceptions>
    $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-runtime=gnustep-2.1>
    $<$<COMPILE_LANGUAGE:OBJC>:-fblocks>
    $<$<COMPILE_LANGUAGE:OBJC>:-fconstant-string-class=NSConstantString>
)

# Source/ additionally gets the target triple defines and GNUSTEP_BASE_INTERNAL;
# the Additions and unix subprojects do not (they are compiled as subprojects).
# Values are the ones gnustep-make computed for x86_64/linux-gnu/ng-gnu-gnu.
set_property(SOURCE ${GSBASE_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS
    GNUSTEP_TARGET_DIR="."
    GNUSTEP_TARGET_CPU="x86_64"
    GNUSTEP_TARGET_OS="linux-gnu"
    GNUSTEP_IS_FLATTENED="yes"
    LIBRARY_COMBO="ng-gnu-gnu"
    GNUSTEP_BASE_INTERNAL=1
)
set_property(SOURCE ${GSBASE_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -Wcast-align)
# Source/Additions is also buildable standalone against Apple Foundation; the
# subproject makefile defines NO_GNUSTEP=1 for it.
set_property(SOURCE ${GSBASE_ADDITIONS} APPEND PROPERTY COMPILE_DEFINITIONS NO_GNUSTEP=1)

# Include order matters: the frozen configure output must win over anything in
# the vendored tree.  `private/` holds config.h + dynamic-load.h (quoted includes
# from Source/common.h and Source/objc-load.m that used to resolve to files
# config.status dropped into Source/); `include/` holds the public
# GNUstepBase/GSConfig.h that consumers need too.
target_include_directories(gnustep-base-objs PRIVATE
    "${GSBASE_CFG}/private"
    "${GSBASE_CFG}/include"
    "${GSBASE_SRC}/Headers"
    "${GSBASE_SRC}/Source"
    "${GSBASE_SRC}/Source/Additions"
    "${GSBASE_SRC}/Source/unix"
)
target_link_libraries(gnustep-base-objs PRIVATE objc PkgConfig::GSBASE_XML PkgConfig::GSBASE_TLS)

add_library(gnustep-base SHARED $<TARGET_OBJECTS:gnustep-base-objs>)
set_target_properties(gnustep-base PROPERTIES
    LINKER_LANGUAGE OBJC
    OUTPUT_NAME gnustep-base
    VERSION 1.29.0
    SOVERSION 1.29
)
# Transcribed from the reference link line (order-insensitive for a shared lib).
target_link_libraries(gnustep-base PRIVATE
    objc
    PkgConfig::GSBASE_XML
    PkgConfig::GSBASE_TLS
    -lpthread -lgmp -lxslt -lffi -lz -lrt -ldl -licui18n -ldispatch -lcurl -lm
)
target_link_options(gnustep-base PRIVATE -pthread -fexceptions -rdynamic)

# What consumers of -lgnustep-base need to see.
target_include_directories(gnustep-base INTERFACE
    "${GSBASE_CFG}/include"
    "${GSBASE_SRC}/Headers"
)
target_link_libraries(gnustep-base INTERFACE objc)
