include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Common.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Durango.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/ORBIS.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/PS3.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/PSVita.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Windows.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Xbox360.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/CommonSources.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

# FourKit bridge sources
set(FOURKIT_BRIDGE_SOURCES
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitBridge.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitBridge.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitMappers.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitNatives.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitNatives.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitRuntime.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/FourKitRuntime.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogger.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogger.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerLogManager.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerProperties.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/ServerProperties.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/Access.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/Access.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/BanManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/BanManager.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/OpManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/OpManager.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/WhitelistManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Access/WhitelistManager.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/StringUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/StringUtils.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/FileUtils.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/FileUtils.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/NetworkUtils.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common/AccessStorageUtils.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/SecurityConfig.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/SecurityConfig.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/RateLimiter.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/RateLimiter.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/ConnectionCipher.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/ConnectionCipher.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/CipherHandshakeEnforcer.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/CipherHandshakeEnforcer.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/IdentityTokenManager.cpp"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Security/IdentityTokenManager.h"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.c"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.h"
)
source_group("FourKit" FILES ${FOURKIT_BRIDGE_SOURCES})

# Combine all source files into a single variable for the target
# We cant use CMAKE_CONFIGURE_PRESET here as VS doesn't set it, so just rely on the folder
set(MINECRAFT_CLIENT_SOURCES
  ${MINECRAFT_CLIENT_COMMON}
  $<$<STREQUAL:${PLATFORM_NAME},Durango>:${MINECRAFT_CLIENT_DURANGO}>
  $<$<STREQUAL:${PLATFORM_NAME},Orbis>:${MINECRAFT_CLIENT_ORBIS}>
  $<$<STREQUAL:${PLATFORM_NAME},PS3>:${MINECRAFT_CLIENT_PS3}>
  $<$<STREQUAL:${PLATFORM_NAME},PSVita>:${MINECRAFT_CLIENT_PSVITA}>
  $<$<STREQUAL:${PLATFORM_NAME},Windows64>:${MINECRAFT_CLIENT_WINDOWS}>
  $<$<STREQUAL:${PLATFORM_NAME},Xbox>:${MINECRAFT_CLIENT_XBOX360}>
  ${SOURCES_COMMON}
  ${FOURKIT_BRIDGE_SOURCES}
)

add_executable(Minecraft.Client ${MINECRAFT_CLIENT_SOURCES})

# Only define executable on windows
if(PLATFORM_NAME STREQUAL "Windows64")
  set_target_properties(Minecraft.Client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# FourKit bridge sources moved before MINECRAFT_CLIENT_SOURCES

target_include_directories(Minecraft.Client PRIVATE
  "${CMAKE_BINARY_DIR}/generated/" # This is for the generated BuildVer.h
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/Iggy/include"
  "${CMAKE_SOURCE_DIR}/include/"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/Common"
  "${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/nlohmann"
)
target_compile_definitions(Minecraft.Client PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
)
target_precompile_headers(Minecraft.Client PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:stdafx.h>")
set_source_files_properties(compat_shims.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON) # This redefines internal MSVC CRT symbols which will cause an issue with PCH
set_source_files_properties("${CMAKE_SOURCE_DIR}/Minecraft.Server/vendor/linenoise/linenoise.c" PROPERTIES SKIP_PRECOMPILE_HEADERS ON) # C file cannot use C++ PCH

configure_compiler_target(Minecraft.Client)

set_target_properties(Minecraft.Client PROPERTIES
  OUTPUT_NAME "minecraft.revhost"
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Client>"
)

target_link_libraries(Minecraft.Client PRIVATE
  Minecraft.World
  d3d11
  dxgi
  d3dcompiler
  XInput9_1_0
  wsock32
  legacy_stdio_definitions
  bcrypt
  shell32
  4JLibs.${PLATFORM_NAME}.Input
  4JLibs.${PLATFORM_NAME}.Profile
  4JLibs.${PLATFORM_NAME}.Storage
  4JLibs.${PLATFORM_NAME}.Render
)

# Iggy libs
foreach(lib IN LISTS IGGY_LIBS)
  target_link_libraries(Minecraft.Client PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/Iggy/lib/${lib}")
endforeach()

# ---
# Asset / redist copy
# ---
include("${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake")
set(ASSET_FOLDER_PAIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/music"                  "resources/music"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Media"           "resources/Common/Media"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/res"             "resources/Common/res"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}Media"  "resources/${PLATFORM_NAME}Media"
)
setup_asset_folder_copy(Minecraft.Client "${ASSET_FOLDER_PAIRS}")

# Copy redist files
add_copyredist_target(Minecraft.Client)

# Make sure Worlds exists on Windows
if(PLATFORM_NAME STREQUAL "Windows64")
  add_gamehdd_target(Minecraft.Client)
endif()

# ---
# FourKit managed (.NET 10) build step
# ---
set(FOURKIT_CSPROJ "${CMAKE_SOURCE_DIR}/Minecraft.Server.FourKit/Minecraft.Server.FourKit.csproj")

find_program(DOTNET_EXECUTABLE NAMES dotnet
  HINTS "$ENV{ProgramFiles}/dotnet" "$ENV{ProgramW6432}/dotnet"
  DOC ".NET CLI executable")

if(DOTNET_EXECUTABLE)
  set(DOTNET_CONFIG "$<IF:$<CONFIG:Debug>,Debug,Release>")

  add_custom_target(Minecraft.Server.FourKit.Managed ALL
    COMMAND "${DOTNET_EXECUTABLE}" publish "${FOURKIT_CSPROJ}"
      --configuration "${DOTNET_CONFIG}"
      --runtime win-x64
      --self-contained true
      --output "$<TARGET_FILE_DIR:Minecraft.Client>/runtime"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Minecraft.Client>/plugins"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Minecraft.Client>/plugin-dependencies"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Minecraft.Server.FourKit"
    COMMENT "Building FourKit managed plugin host (.NET 10)"
    VERBATIM
  )

  add_dependencies(Minecraft.Client Minecraft.Server.FourKit.Managed)
else()
  message(WARNING "dotnet not found - FourKit managed runtime will not be built. Install .NET 10 SDK.")
endif()
