# CMakeList.txt : CMake project for CraneVision, include source and define
# project specific logic here.
#
project(Render)

# Add source to this project's executable.
file(GLOB SRC "*.hpp" "*.cpp")
add_library(${PROJECT_NAME} ${SRC})

if(ANDROID)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_ANDROID_KHR)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC
                      vulkan
        vma
        spdlog
                      spirv_reflect
                      eigen
                      )
target_link_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/../Lib)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(ANDROID)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ANDROID)
endif()
# TODO: Add tests and install targets if needed.
