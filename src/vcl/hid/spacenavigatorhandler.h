/*  
 * The VCL Jobs is released under the MIT
 * license.
 * 
 * Copyright (c) 2017 Basil Fierz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

// VCL configuration
#include <vcl/config/global.h>

// C++ standard library
#include <array>

// VCL
#include <vcl/hid/spacenavigatorvirtualkeys.h>

namespace Vcl { namespace HID
{
	/// Forward declaration
	class SpaceNavigator;

	class SpaceNavigatorHandler
	{
	public:
		//////////////////////////////////////////////////////////////////////////////////
		// onSpaceMouseMove is invoked when new 3d mouse data is available.
		// Override this method to process 3d mouse 6dof data in the application
		// Input:
		//    DeviceId device - the raw input device handle 
		//                     this can be used to identify the device the data is coming
		//                     from or simply ignored.
		//
		//    array<float, 6> motion_data - contains the displacement data, using
		//                     a right-handed coordinate system with z down
		//                     see 'Programing for the 3dmouse' document available at
		//                     www.3dconnexion.com.
		//                     aMotionData[0], aMotionData[1], aMotionData[2] is the
		//                     incremental pan zoom displacement vector (x,y,z).
		//                     aMotionData[3], aMotionData[4], aMotionData[5] is the
		//                     incremental rotation vector (NOT Euler angles)
		virtual void onSpaceMouseMove(const SpaceNavigator* device, std::array<float, 6> motion_data) = 0;

		//////////////////////////////////////////////////////////////////////////////////
		// onSpaceMouseKeyDown processes the standard 3d mouse key presses
		// This method can be overwritten
		// Input:
		//    DeviceId device - the raw input device handle 
		//                     this can be used to identify the device the data is coming
		//                     from or simply ignored.
		//   unsigned int virtual_key - Standard 3d mouse key code 
		virtual void onSpaceMouseKeyDown(const SpaceNavigator* device, unsigned int virtual_key) = 0;

		//////////////////////////////////////////////////////////////////////////////////
		// onSpaceMouseKeyUp processes the standard 3d mouse key releases
		// This method can be overwritten
		// Input:
		//    DeviceId device - the raw input device handle 
		//                     this can be used to identify the device the data is coming
		//                     from or simply ignored.
		//   unsigned int virtual_key - Standard 3d mouse key code 
		virtual void onSpaceMouseKeyUp(const SpaceNavigator* device, unsigned int virtual_key) = 0;
	};
}}
