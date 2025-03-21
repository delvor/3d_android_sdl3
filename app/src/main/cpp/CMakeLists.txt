# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("testsdl")

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)



# Define GLM manually as a header-only library
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ./../../../libs/glm)

add_library(glm::glm ALIAS glm)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(maze3D SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        maze3D.cpp
        utils/utils.cpp
        utils/utils.h)
add_library(matrix SHARED
        matrix.cpp
        utils/utils.cpp
        utils/utils.h
)

# Find GLESv3 library explicitly (Android NDK provides this)
find_library(GLES3_LIBRARY NAMES GLESv3 libGLESv3
       )
if(NOT GLES3_LIBRARY)
    message(FATAL_ERROR "GLESv3 library not found. Ensure your NDK supports GLESv3.")
endif()


# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(maze3D PRIVATE
        # List libraries link to the target library
        android
        log SDL3::SDL3 SDL3_image::SDL3_image  ${GLES3_LIBRARY}  glm::glm)

target_link_libraries(matrix PRIVATE
        # List libraries link to the target library
        android
        log SDL3::SDL3 SDL3_ttf::SDL3_ttf)