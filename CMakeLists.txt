cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Two modes: 
#  - Debug   : Default
#  - Realease: -DCMAKE_BUILD_TYPE=RELEASE
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()


# Start the Cmake 
project(Deribit)

# Allow folder structure in Visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define the C++ version
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cmake scripts to use
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Specify flags for Windows
if(MSVC)
	# Specify flags for Visual studio 2017
	if (MSVC_VERSION GREATER_EQUAL "1900")
		include(CheckCXXCompilerFlag)
		CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
		if (_cpp_latest_flag_supported)
			add_compile_options("/std:c++latest")
		endif()
	endif()	
else()
	# Specify flags for linux
    # add_compile_options(-W -Wall)
endif()

add_subdirectory(src)
