#!/usr/bin/env python3
import os
import subprocess
import shutil

PROJECT_DIR = os.getcwd()
LEXBOR_DIR = os.path.join(PROJECT_DIR, "lib", "lexbor")
BUILD_DIR = os.path.join(PROJECT_DIR, ".pio", "lexbor_build")

# Create build directory
os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

# Get ESP32 toolchain from environment (set by PlatformIO)
toolchain_prefix = os.environ.get('CC', '').replace('-gcc', '')
if not toolchain_prefix:
    # Fallback to common ESP32 toolchain path
    toolchain_prefix = '/home/codespace/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-'

# Set environment for cross-compilation
env = os.environ.copy()
env['CC'] = toolchain_prefix + 'gcc'
env['CXX'] = toolchain_prefix + 'g++'
env['AR'] = toolchain_prefix + 'ar'
env['RANLIB'] = toolchain_prefix + 'ranlib'

# Run CMake with flags for static library only and cross-compilation
subprocess.run([
    "cmake", LEXBOR_DIR,
    "-DLEXBOR_BUILD_STATIC=ON",
    "-DLEXBOR_BUILD_SHARED=OFF",
    "-DLEXBOR_BUILD_TESTS=OFF",
    "-DLEXBOR_BUILD_EXAMPLES=OFF",
    "-DLEXBOR_INSTALL_HEADERS=OFF",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_SYSTEM_NAME=Generic",
    "-DCMAKE_C_COMPILER=" + env['CC'],
    "-DCMAKE_CXX_COMPILER=" + env['CXX'],
    "-DCMAKE_AR=" + env['AR'],
    "-DCMAKE_RANLIB=" + env['RANLIB'],
    "-DCMAKE_C_FLAGS=-mlongcalls",
    "-DCMAKE_CXX_FLAGS=-mlongcalls"
], env=env, check=True)

# Build
subprocess.run(["make"], env=env, check=True)

# Copy static library to lexbor dir
shutil.copy("liblexbor_static.a", LEXBOR_DIR)

# Clean up build directory
os.chdir(PROJECT_DIR)
shutil.rmtree(BUILD_DIR)

print("Lexbor static library built using ESP32 toolchain and build folder cleaned.")
