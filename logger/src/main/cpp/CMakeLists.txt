
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)


configure_file(Config.h.in Config.h)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Installation directory" FORCE)
message(STATUS "CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")

# for zstd
#ui options menu
option(ZSTD_BUILD_STATIC "BUILD STATIC LIBRARIES" ON)
option(ZSTD_BUILD_SHARED "BUILD SHARED LIBRARIES" OFF)

#new variables
set(ZSTD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zstd")
set(LIBRARY_DIR ${ZSTD_SOURCE_DIR}/lib)


#for reuse like c++ include command
include(GNUInstallDirs)


#header search path
#include_directories ([AFTER|BEFORE] [SYSTEM] dir1 [dir2 ...])
#将指定目录添加到编译器的头文件搜索路径之下，指定的目录被解释成当前源码路径的相对路径。
include_directories("include")

#将子目录构建并将二进制保存到zstd目录
#add_subdirectory (source_dir [binary_dir] [EXCLUDE_FROM_ALL])
#添加一个子目录并构建该子目录保存到当前目录的zstd目录，如果不指定则保存到source目录
add_subdirectory(zstd/cmakebuild/cmake/lib zstd)



set(SELF_LIBS_OUT ${CMAKE_SYSTEM_NAME}.out)


# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("phoenix")

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

file(GLOB allCpp *c *h *cpp *hpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${allCpp})

#将zstd路径加入链接搜索库中，这样在target_link_libraries 可以直接添加
#LINK_DIRECTORIES 命令来指定第三方库所在路径，比如，你的动态库在/home/myproject/libs这个路径下，则通过命令：LINK_DIRECTORIES(/home/myproject/libs)，把该路径添加到第三方库搜索路径中，这样就可以使用相对路径了，使用TARGET_LINK_LIBRARIES的时候，只需要给出动态链接库的名字就行了。
#也可以set(LINK_DIR /Users/haoran/Downloads/wfdb/lib)，LINK_DIRECTORIES({LINK_DIR})
#官方不建议使用该命令，取而代之的为find_package()  find_library()
find_package(zstd)




# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        libzstd_static
        log)
