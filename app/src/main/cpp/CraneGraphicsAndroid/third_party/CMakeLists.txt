project(third_party)

# vma
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE "VulkanMemoryAllocator/include")

# eigen
add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE "eigen")

# spdlog
add_subdirectory(spdlog)

# spirv-reflect
add_library(spirv_reflect "SPIRV-Reflect/spirv_reflect.c")
target_include_directories(spirv_reflect PUBLIC "SPIRV-Reflect")

#tracy
#add_subdirectory(tracy)

# json
add_subdirectory(json)

# lz4
set(LZ4_SOURCES
  "lz4/lib/lz4.c"
  "lz4/lib/lz4hc.c"
  "lz4/lib/lz4.h"
  "lz4/lib/lz4hc.h"
  "lz4/lib/lz4frame.c"
  "lz4/lib/lz4frame.h"
  "lz4/lib/xxhash.c")
add_library(lz4 STATIC ${LZ4_SOURCES})
target_include_directories(lz4 PUBLIC "lz4/lib")

# VkFFT
add_library(VkFFT INTERFACE)
target_include_directories(VkFFT INTERFACE "VkFFT/vkFFT")
target_include_directories(VkFFT INTERFACE "/usr/include/glslang/Include/")

# imgui
set(IMGUI_DIR "../third_party/imgui")
file(GLOB srcImgui "${IMGUI_DIR}/*.cpp")
add_library(imgui
        "${srcImgui}"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_android.cpp")
target_include_directories(imgui PUBLIC "${IMGUI_DIR}")
target_link_libraries(imgui vulkan)
#[[
#glslang
add_subdirectory(glslang)
]]