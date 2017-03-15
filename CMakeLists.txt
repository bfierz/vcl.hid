CMAKE_MINIMUM_REQUIRED(VERSION 3.6)

PROJECT(vcl.hid)

# Configure the VCL compilation environment
INCLUDE(externals/vcl/src/cmake/VCLConfig.cmake)

# External libraries
SUBDIRS(externals/vcl/src/externals/gsl)
SET(GSL_TEST OFF CACHE BOOL "" FORCE)
SUBDIRS(externals/vcl/src/externals/fmtlib)
SUBDIRS(externals/vcl/src/externals/json)
SET(BuildTests OFF CACHE BOOL "" FORCE)
SUBDIRS(externals/vcl/src/libs/vcl.core)

SET(VCL_HID_WINDOWS_INC
	src/vcl/hid/windows/hid.h
)
SET(VCL_HID_WINDOWS_SRC
	src/vcl/hid/windows/hid.cpp
)

SET(VCL_HID_INC
	src/vcl/hid/device.h
	src/vcl/hid/gamepad.h
	src/vcl/hid/joystick.h
)
SET(VCL_HID_SRC
	src/vcl/hid/device.cpp
	src/vcl/hid/gamepad.cpp
	src/vcl/hid/joystick.cpp
)

SET(SOURCE
	${VCL_HID_WINDOWS_INC}
	${VCL_HID_WINDOWS_SRC}
	${VCL_HID_INC}
	${VCL_HID_SRC}
)

SOURCE_GROUP("windows" FILES ${VCL_HID_WINDOWS_SRC} ${VCL_HID_WINDOWS_INC})
SOURCE_GROUP("" FILES ${VCL_HID_SRC} ${VCL_HID_INC})

# Generate library
ADD_LIBRARY(vcl.hid STATIC ${SOURCE})
SET_TARGET_PROPERTIES(vcl.hid PROPERTIES FOLDER libs)
SET_TARGET_PROPERTIES(vcl.hid PROPERTIES DEBUG_POSTFIX _d)

TARGET_INCLUDE_DIRECTORIES(vcl.hid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

TARGET_LINK_LIBRARIES(vcl.hid
	vcl_core
	hid
)

# Build examples
OPTION(VCL_HID_BUILD_EXAMPLES "Build the examples" OFF)
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
