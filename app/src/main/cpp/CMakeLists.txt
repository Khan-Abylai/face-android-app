# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.

cmake_minimum_required(VERSION 3.22.1)

project("face_det_mobapp")


set(OpenCV_STATIC on)
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/OpenCV-android-sdk/sdk/native/jni)
find_package(OpenCV REQUIRED)

set(ncnn_DIR ${CMAKE_SOURCE_DIR}/ncnn-20230816-android-vulkan/${ANDROID_ABI}/lib/cmake/ncnn)
find_package(ncnn REQUIRED)


add_library(${CMAKE_PROJECT_NAME} SHARED
        face.cpp
        face.h
        ndkcamera.cpp
        ndkcamera.h
        native-lib.cpp
        )

find_library(
        log-lib
        log)

find_library(jnigraphics-lib jnigraphics)

target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        ncnn
        ${OpenCV_LIBS}
        ${jnigraphics-lib}
        ${log-lib}
        camera2ndk mediandk)