cmake_minimum_required(VERSION 3.14)

#project(ncmdump LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(ncmdump_wasm CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(WITH_ZLIB OFF)
set(BUILD_TESTING OFF)

# 添加 taglib 作为子目录。确保 taglib 本身可以使用 Emscripten 编译。
add_subdirectory(taglib)

# 核心逻辑的通用源文件和头文件
FILE(GLOB COMMON_HEADERS src/include/*.h)
FILE(GLOB COMMON_SOURCES src/ncmcrypt.cpp src/utils/*.cpp)
FILE(GLOB LIBRARY_HEADERS src/lib/libncmdump.h)
FILE(GLOB LIBRARY_SOURCES src/lib/*.cpp)

# Emscripten 绑定接口的源文件
set(WASM_INTERFACE_SOURCE src/wasm_interface.cpp)

# =================== WebAssembly 构建配置 ===================
if(EMSCRIPTEN)
  message(STATUS "Configuring WebAssembly build with Emscripten")

  # Emscripten 可执行目标
  add_executable(ncmdump_wasm
          ${COMMON_SOURCES}        # 包含核心 C++ 源文件
          ${LIBRARY_SOURCES}       # 包含库源文件
          ${WASM_INTERFACE_SOURCE} # 包含 embind 接口源文件
  )

  # 所有源文件的包含目录
  target_include_directories(ncmdump_wasm PRIVATE
          src/include
          taglib
          taglib/taglib
          taglib/taglib/toolkit
          taglib/taglib/mpeg/id3v2
  )

  # 链接 taglib
  target_link_libraries(ncmdump_wasm tag)

  # 设置输出名称 (将生成 ncmdump.js 和 ncmdump.wasm)
  set_target_properties(ncmdump_wasm PROPERTIES OUTPUT_NAME "ncmdump")

  # 编译选项: 仅 C++ 特定的编译标志 (每个 .cpp 文件编译时使用)
  target_compile_options(ncmdump_wasm PRIVATE
          -frtti        # 启用运行时类型信息
          -fexceptions  # 启用 C++ 异常
  )

  # 链接选项: Emscripten 特定的标志，用于模块生成和运行时 (链接整个模块时使用)
  target_link_options(ncmdump_wasm PRIVATE
          --bind                     # 关键：启用 embind
          -sMODULARIZE=1
          -sEXPORT_NAME="createNCMDumpModule"
          -sEXPORT_ES6=1
          -sUSE_ES6_IMPORT_META=1
          -sENVIRONMENT=web
          -sALLOW_MEMORY_GROWTH=1
          -sEXIT_RUNTIME=0
          -sASSERTIONS=1             # 调试时非常有用，会提供更多错误信息
          -sFORCE_FILESYSTEM=1
          -sEXPORTED_RUNTIME_METHODS=FS # 确保 FS 被导出
          -sNO_DISABLE_EXCEPTION_CATCHING
  )

  # 将 WASM 文件添加到安装目标 (可选，用于部署)
  install(TARGETS ncmdump_wasm
          RUNTIME DESTINATION wasm
          LIBRARY DESTINATION wasm
          ARCHIVE DESTINATION wasm
  )
else()
  # 桌面构建配置 (如果仍然需要)
  # 这里可以放置你的原始桌面版 add_executable 和 add_library 逻辑。
  # 例如:
  # add_executable(ncmdump_exec
  #   ${COMMON_HEADERS}
  #   ${COMMON_SOURCES}
  #   src/main.cpp # 你的原始桌面版 main.cpp
  # )
  # target_include_directories(ncmdump_exec PRIVATE src/include)
  # target_link_libraries(ncmdump_exec tag)
  # ... 等等桌面构建的配置
endif()
