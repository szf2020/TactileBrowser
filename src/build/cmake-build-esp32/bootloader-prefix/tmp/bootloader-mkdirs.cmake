# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/codespace/esp/esp-idf/components/bootloader/subproject"
  "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader"
  "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix"
  "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix/tmp"
  "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix/src/bootloader-stamp"
  "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix/src"
  "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/TactileBrowser/src/build/cmake-build-esp32/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
