cmake_minimum_required(VERSION 3.6)

project(vcl.hid)

# Determine whether this is a standalone project or included by other projects
set(VCL_HID_STANDALONE_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(VCL_HID_STANDALONE_PROJECT ON)
endif()

# Load the VCL libraries from externally
set(VCL_SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/vcl-src")
set(VCL_BINARY_DIR "${CMAKE_BINARY_DIR}/externals/vcl-build")

if (VCL_HID_STANDALONE_PROJECT)
	message(STATUS "Update VCL in '${VCL_SOURCE_DIR}'")
	if(NOT EXISTS "${VCL_SOURCE_DIR}")
		file(MAKE_DIRECTORY ${VCL_SOURCE_DIR})
		execute_process(
			COMMAND git clone https://github.com/bfierz/vcl.git .
			COMMAND git checkout master
			COMMAND git submodule update --init --recursive
			WORKING_DIRECTORY ${VCL_SOURCE_DIR}
		)
	else()
		execute_process(
			COMMAND git fetch --all --prune
			COMMAND git checkout "master"
			COMMAND git merge
			COMMAND git submodule sync
			COMMAND git submodule update --init --recursive
			WORKING_DIRECTORY ${VCL_SOURCE_DIR}
		)
	endif()
endif()

# VCL base libraries
if (VCL_HID_STANDALONE_PROJECT)
	option(VCL_BUILD_BENCHMARKS "" OFF)
	option(VCL_BUILD_EXAMPLES "" OFF)
	option(VCL_BUILD_TOOLS "" ON)
	option(VCL_BUILD_TESTS "" OFF)
	option(VCL_OPENGL_SUPPORT "" ON)
	add_subdirectory(${VCL_SOURCE_DIR}/src EXCLUDE_FROM_ALL)
endif()

# Generate library
add_library(vcl.hid STATIC "")

set(VCL_HID_WINDOWS_INC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/hid.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/spacenavigator.h
)
set(VCL_HID_WINDOWS_SRC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/hid.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/spacenavigator.cpp
)

set(VCL_HID_INC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/device.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/gamepad.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/joystick.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/multiaxiscontroller.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigator.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigatorhandler.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigatorvirtualkeys.h
)
set(VCL_HID_SRC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/device.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/gamepad.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/joystick.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/multiaxiscontroller.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigator.cpp
)

source_group("windows" FILES ${VCL_HID_WINDOWS_SRC} ${VCL_HID_WINDOWS_INC})
source_group("" FILES ${VCL_HID_SRC} ${VCL_HID_INC})

target_sources(vcl.hid
	PRIVATE
		${VCL_HID_WINDOWS_SRC}
		${VCL_HID_SRC}
	PUBLIC
		${VCL_HID_WINDOWS_INC}
		${VCL_HID_INC}
)

target_include_directories(vcl.hid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(vcl.hid PROPERTIES FOLDER libs)
set_target_properties(vcl.hid PROPERTIES DEBUG_POSTFIX _d)

target_link_libraries(vcl.hid
	vcl_core
	hid
)

# Build examples
option(VCL_HID_BUILD_EXAMPLES "Build the examples" ${VCL_HID_STANDALONE_PROJECT})
if(VCL_HID_BUILD_EXAMPLES)

	# Enumeration example
	set(VCL_HID_EXAMPLE_ENUM_SRC
		examples/enum/main.cpp
	)
	
	source_group("" FILES ${VCL_HID_EXAMPLE_ENUM_SRC})

	add_executable(vcl.hid.enum
		${VCL_HID_EXAMPLE_ENUM_SRC}
	)
	
	set_target_properties(vcl.hid.enum PROPERTIES FOLDER examples)
	target_link_libraries(vcl.hid.enum
		vcl.hid
	)

	# Sample client example
	set(VCL_HID_EXAMPLE_CLIENT_SRC
		examples/client/main.cpp
	)
	
	source_group("" FILES ${VCL_HID_EXAMPLE_CLIENT_SRC})
	
	add_executable(vcl.hid.client
		${VCL_HID_EXAMPLE_CLIENT_SRC}
	)
	
	set_target_properties(vcl.hid.client PROPERTIES FOLDER examples)
	target_link_libraries(vcl.hid.client
		vcl.hid
	)

endif(VCL_HID_BUILD_EXAMPLES)
