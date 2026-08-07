# Isolated MSDF/D3D font-rendering library.
#
# These sources are ported verbatim from upstream noname08662/awesome_wotlk and are written
# against upstream's client-abstraction headers (GameClient.h / Hooks.h / Types.h / Lua.h /
# Utils.h), which are vendored *in this directory* and differ from the fork's own headers of
# the same name. Compiling them as a separate static library with this directory on the
# include path keeps upstream's headers and the fork's headers in disjoint translation units,
# so the two never collide. Only primitive-typed entry points (see MSDFBridge.h) cross back
# into the fork.

add_library(AwesomeMSDF STATIC
    "D3D.cpp"
    "MSDF.cpp"
    "MSDFCache.cpp"
    "MSDFManager.cpp"
    "MSDFFont.cpp"
)

target_include_directories(AwesomeMSDF PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                 # upstream client headers + MSDF/D3D headers
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/deps/skia/include
    ${CMAKE_SOURCE_DIR}/deps/msdfgen
    ${CMAKE_BINARY_DIR}/deps/msdfgen
)

# MSDF and msdfgen use C++ exceptions and heavy STL; the DLL-wide flags drop /EHsc, so restore
# synchronous EH for just this target.
target_compile_options(AwesomeMSDF PRIVATE /EHsc)

# The DLL-wide flags pin _WIN32_WINNT=0x0601 (Win7) for the injector/Detours, but the MSDF
# manager calls Windows 10 APIs (VirtualAlloc2 / MapViewOfFile3 / UnmapViewOfFile2), guarded at
# runtime via GetProcAddress. Raise the SDK target for this library only so they are declared.
# These must be compile *options* (not definitions) so they land after the DLL-wide
# /D_WIN32_WINNT=0x0601 in the command line and win; /U first avoids a redefinition warning.
target_compile_options(AwesomeMSDF PRIVATE
    /U_WIN32_WINNT /UWINVER /UNTDDI_VERSION
    /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00 /DNTDDI_VERSION=0x0A000000
)

# PUBLIC so these propagate to the AwesomeWotlkLib DLL that links this static library.
target_link_libraries(AwesomeMSDF PUBLIC
    Detours
    msdfgen::msdfgen-core
    msdfgen::msdfgen-ext
    freetype
    ankerl::unordered_dense
    d3d9
    d3dcompiler
    "${CMAKE_SOURCE_DIR}/deps/skia/skia.lib"
)
