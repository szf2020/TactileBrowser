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

# Run CMake with flags for static library only
subprocess.run([
    "cmake", LEXBOR_DIR,
    "-DLEXBOR_BUILD_STATIC=ON",
    "-DLEXBOR_BUILD_SHARED=OFF",
    "-DLEXBOR_BUILD_TESTS=OFF",
    "-DLEXBOR_BUILD_EXAMPLES=OFF",
    "-DLEXBOR_INSTALL_HEADERS=OFF",
    "-DCMAKE_BUILD_TYPE=Release"
], check=True)

# Build
subprocess.run(["make"], check=True)

# Copy static library to lexbor dir
shutil.copy("liblexbor_static.a", LEXBOR_DIR)

# Clean up build directory
os.chdir(PROJECT_DIR)
shutil.rmtree(BUILD_DIR)

print("Lexbor static library built using CMake and build folder cleaned.")