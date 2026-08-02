set(FOURKIT_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(FOURKIT_OUTPUT_DIR  "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>")
set(FOURKIT_CSPROJ      "${FOURKIT_PROJECT_DIR}/Minecraft.Server.FourKit.csproj")

# ---
# .NET 10 SDK check
# ---
# Fail configure with a clear message if .NET 10 SDK isn't installed. Otherwise
# the failure happens deep inside the dotnet publish step with a much less
# obvious error. The version pin in /global.json governs which 10.x SDK is
# selected at build time; this check just confirms one is installed at all.
find_program(DOTNET_EXECUTABLE NAMES dotnet
  HINTS "$ENV{ProgramFiles}/dotnet" "$ENV{ProgramW6432}/dotnet"
  DOC ".NET CLI executable")
if(NOT DOTNET_EXECUTABLE)
  message(FATAL_ERROR
    "FourKit requires the .NET 10 SDK but the 'dotnet' command was not found on PATH.\n"
    "Install it from https://dotnet.microsoft.com/download/dotnet/10.0 (x64 SDK)\n"
    "and reconfigure CMake.")
endif()

execute_process(
  COMMAND "${DOTNET_EXECUTABLE}" --list-sdks
  OUTPUT_VARIABLE FOURKIT_DOTNET_SDKS
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT FOURKIT_DOTNET_SDKS MATCHES "(^|\n)10\\.[0-9]+\\.[0-9]+")
  if(FOURKIT_DOTNET_SDKS STREQUAL "")
    set(_sdk_list "  (none installed)")
  else()
    set(_sdk_list "${FOURKIT_DOTNET_SDKS}")
  endif()
  message(FATAL_ERROR
    "FourKit requires a .NET 10 SDK but none was detected.\n"
    "Install it from https://dotnet.microsoft.com/download/dotnet/10.0 (x64 SDK)\n"
    "and reconfigure CMake.\n"
    "\n"
    "Currently installed .NET SDKs:\n"
    "${_sdk_list}")
endif()
message(STATUS "FourKit: .NET CLI at ${DOTNET_EXECUTABLE}")

file(GLOB_RECURSE FOURKIT_SOURCES RELATIVE "${FOURKIT_PROJECT_DIR}" "${FOURKIT_PROJECT_DIR}/*.cs")
list(FILTER FOURKIT_SOURCES EXCLUDE REGEX "([/\\](obj|bin)[/\\])|^(obj|bin)[/\\]")

set(DOTNET_CONFIG "$<IF:$<CONFIG:Debug>,Debug,Release>")

foreach(src_file IN LISTS FOURKIT_SOURCES)
    get_filename_component(src_path "${src_file}" PATH)
    if(src_path)
        string(REPLACE "/" "\\" group_path "${src_path}")
        source_group("${group_path}" FILES "${FOURKIT_PROJECT_DIR}/${src_file}")
    endif()
endforeach()

list(TRANSFORM FOURKIT_SOURCES PREPEND "${FOURKIT_PROJECT_DIR}/")

# Self-contained publish: bundles .NET 10 runtime so end users need nothing pre-installed.
# The publish AND the staging copy both live on this target so they re-run together
# whenever any FourKit C# source changes, even on incremental builds where the
# Minecraft.Server.FourKit C++ exe does not re-link.
#
# Target name note: the CMake target is "Minecraft.Server.FourKit.Managed" so it
# does not collide with the C++ executable target named "Minecraft.Server.FourKit".
# The .csproj file name, the assembly name, and the C# namespace all stay
# Minecraft.Server.FourKit because those are the donor's identifiers.
add_custom_target(Minecraft.Server.FourKit.Managed ALL
  COMMAND dotnet publish "${FOURKIT_CSPROJ}"
    --configuration "${DOTNET_CONFIG}"
    --runtime win-x64
    --self-contained true
    --output "${FOURKIT_OUTPUT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${FOURKIT_OUTPUT_DIR}"
    "$<TARGET_FILE_DIR:Minecraft.Server.FourKit>/runtime"
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:Minecraft.Server.FourKit>/plugins"
  WORKING_DIRECTORY "${FOURKIT_PROJECT_DIR}"
  SOURCES ${FOURKIT_SOURCES}
  COMMENT "dotnet publish Minecraft.Server.FourKit + stage runtime/plugins next to server"
  VERBATIM
)

set_target_properties(Minecraft.Server.FourKit.Managed PROPERTIES
  OUTPUT_NAME "Minecraft.Server.FourKit.Managed"
)

# ---------------------------------------------------------------------------
# FourKit-enabled C++ server executable.
#
# Built from the same shared source list as the vanilla Minecraft.Server,
# plus the seven FourKit native bridge files. The MINECRAFT_SERVER_FOURKIT_BUILD
# preprocessor define switches FourKitBridge.h from inline no-op stubs to its
# real declarations, and the FourKitRuntime.cpp / FourKitBridge.cpp /
# FourKitNatives.cpp / FourKitMappers.cpp impls link in.
#
# This target is defined in the Minecraft.Server.FourKit/ source dir (not
# Minecraft.Server/) so the Visual Studio generator emits its .vcxproj files
# into build/<dir>/Minecraft.Server.FourKit/, keeping the variant identity
# consistent across source dir, project file, runtime output dir, and Docker
# image base.
# ---------------------------------------------------------------------------
include("${CMAKE_SOURCE_DIR}/Minecraft.Server/cmake/sources/Common.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/CommonSources.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ServerTarget.cmake")

set(MINECRAFT_SERVER_FOURKIT_SOURCES
  ${MINECRAFT_SERVER_COMMON}
  ${SOURCES_COMMON}
  ${_MINECRAFT_SERVER_COMMON_SERVER_FOURKIT}
)

add_executable(Minecraft.Server.FourKit ${MINECRAFT_SERVER_FOURKIT_SOURCES})
configure_lce_server_target(Minecraft.Server.FourKit)
target_compile_definitions(Minecraft.Server.FourKit PRIVATE MINECRAFT_SERVER_FOURKIT_BUILD)
add_dependencies(Minecraft.Server.FourKit Minecraft.Server.FourKit.Managed)
