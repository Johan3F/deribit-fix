cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(${CMAKE_MODULE_PATH}/AddSubdirectories.cmake)
include(${CMAKE_MODULE_PATH}/GroupSources.cmake)

project(Deribit)

file(GLOB PROJECT_SOURCES "*.cpp")
file(GLOB PROJECT_HEADERS "*.h")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Linking quickfix
find_package(Quickfix REQUIRED)

target_link_libraries(${PROJECT_NAME} ${quickfix_LIBRARY})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${quickfix_INCLUDE_DIRS})

# Linking boost
if (WIN32)
	set(BOOST_ROOT C:\\libraries\\boost\\boost_1_68_0)
	add_definitions(-DBOOST_ALL_NO_LIB)
	set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost REQUIRED COMPONENTS date_time)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Boost_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

# Linking openssl
find_package(OpenSSL REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})

# Add code that it's inside any subfolder
AddSubdirectories(.)

# Group code so that it appears properly in Visual studio
if (WIN32)
	GroupSources(.)
endif()