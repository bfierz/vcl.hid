CMAKE_MINIMUM_REQUIRED(VERSION 3.6)

PROJECT(vcl.hid)

# Determine whether this is a standalone project or included by other projects
SET(VCL_HID_STANDALONE_PROJECT OFF)
IF (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  SET(VCL_HID_STANDALONE_PROJECT ON)
ENDIF ()

# Load the VCL core library from externally
IF (VCL_HID_STANDALONE_PROJECT)

	INCLUDE(ExternalProject)

	SET(VCL_SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/vcl-src")
	SET(VCL_BINARY_DIR "${CMAKE_BINARY_DIR}/externals/vcl-build")
	
	FILE(MAKE_DIRECTORY ${VCL_SOURCE_DIR})
	EXECUTE_PROCESS(
		COMMAND git clone https://github.com/bfierz/vcl.git .
		COMMAND git submodule update --init
		WORKING_DIRECTORY ${VCL_SOURCE_DIR}
	)

	#EXTERNALPROJECT_ADD(vcl
	#	GIT_REPOSITORY    https://github.com/bfierz/vcl.git
	#	GIT_TAG           master
	#	SOURCE_DIR        "${VCL_SOURCE_DIR}"
	#	BINARY_DIR        "${VCL_BINARY_DIR}"
	#	CONFIGURE_COMMAND ""
	#	BUILD_COMMAND     ""
	#	INSTALL_COMMAND   ""
	#	TEST_COMMAND      ""
	#)
		
	# Configure the VCL compilation environment
	SET(CMAKE_MODULE_PATH "${VCL_SOURCE_DIR}/src/cmake")
	INCLUDE(${VCL_SOURCE_DIR}/src/cmake/VCLConfig.cmake)
	
	# External libraries
	SET(EIGEN3_INCLUDE_DIR "${VCL_SOURCE_DIR}/src/externals/eigen")

	SUBDIRS(${VCL_SOURCE_DIR}/src/externals/gsl)
	SET(GSL_TEST OFF CACHE BOOL "" FORCE)
	SUBDIRS(${VCL_SOURCE_DIR}/src/externals/fmtlib)
	SUBDIRS(${VCL_SOURCE_DIR}/src/externals/json)
	SET(BuildTests OFF CACHE BOOL "" FORCE)

	SUBDIRS(${VCL_SOURCE_DIR}/src/libs/vcl.core)

ENDIF (VCL_HID_STANDALONE_PROJECT)

# Generate library
ADD_LIBRARY(vcl.hid STATIC "")

SET(VCL_HID_WINDOWS_INC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/hid.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/spacenavigator.h
)
SET(VCL_HID_WINDOWS_SRC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/hid.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/windows/spacenavigator.cpp
)

SET(VCL_HID_INC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/device.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/gamepad.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/joystick.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/multiaxiscontroller.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigator.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigatorhandler.h
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigatorvirtualkeys.h
)
SET(VCL_HID_SRC
	${PROJECT_SOURCE_DIR}/src/vcl/hid/device.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/gamepad.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/joystick.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/multiaxiscontroller.cpp
	${PROJECT_SOURCE_DIR}/src/vcl/hid/spacenavigator.cpp
)

SOURCE_GROUP("windows" FILES ${VCL_HID_WINDOWS_SRC} ${VCL_HID_WINDOWS_INC})
SOURCE_GROUP("" FILES ${VCL_HID_SRC} ${VCL_HID_INC})

TARGET_SOURCES(vcl.hid
	PRIVATE
		${VCL_HID_WINDOWS_SRC}
		${VCL_HID_SRC}
	PUBLIC
		${VCL_HID_WINDOWS_INC}
		${VCL_HID_INC}
)

TARGET_INCLUDE_DIRECTORIES(vcl.hid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

SET_TARGET_PROPERTIES(vcl.hid PROPERTIES FOLDER libs)
SET_TARGET_PROPERTIES(vcl.hid PROPERTIES DEBUG_POSTFIX _d)

TARGET_LINK_LIBRARIES(vcl.hid
	vcl_core
	hid
)

# Build examples
OPTION(VCL_HID_BUILD_EXAMPLES "Build the examples" ${VCL_HID_STANDALONE_PROJECT})
IF (VCL_HID_BUILD_EXAMPLES)

	# Enumeration example
	SET(VCL_HID_EXAMPLE_ENUM_SRC
		examples/enum/main.cpp
	)
	
	SOURCE_GROUP("" FILES ${VCL_HID_EXAMPLE_ENUM_SRC})

	ADD_EXECUTABLE(vcl.hid.enum
		${VCL_HID_EXAMPLE_ENUM_SRC}
	)
	
	SET_TARGET_PROPERTIES(vcl.hid.enum PROPERTIES FOLDER examples)
	TARGET_LINK_LIBRARIES(vcl.hid.enum
		vcl.hid
	)

	# Sample client example
	SET(VCL_HID_EXAMPLE_CLIENT_SRC
		examples/client/main.cpp
	)
	
	SOURCE_GROUP("" FILES ${VCL_HID_EXAMPLE_CLIENT_SRC})
	
	ADD_EXECUTABLE(vcl.hid.client
		${VCL_HID_EXAMPLE_CLIENT_SRC}
	)
	
	SET_TARGET_PROPERTIES(vcl.hid.client PROPERTIES FOLDER examples)
	TARGET_LINK_LIBRARIES(vcl.hid.client
		vcl.hid
	)

ENDIF (VCL_HID_BUILD_EXAMPLES)
