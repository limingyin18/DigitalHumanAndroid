project(CraneGraphicsAndroid)

add_subdirectory(third_party)
add_subdirectory(AssetLoad)
add_subdirectory(Physics)
add_subdirectory(Render)

#add_library(native_app_glue STATIC
#        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Add source to this project's executable.
file(GLOB SRC "*.hpp" "*.cpp")
add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PUBLIC
                      Render
                      Physics
                      assetlib
                      imgui
                      VkFFT
        #native_app_glue
        PirateNdkEngine)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(${PROJECT_NAME} PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue)
# TODO: Add tests and install targets if needed.

add_subdirectory(Module/OceanFFT)