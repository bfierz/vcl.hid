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

// C++ Standard library
#include <memory>
#include <vector>

// GSL
#include <gsl/gsl>

// VCL
#include <vcl/hid/windows/hid.h>
#include <vcl/hid/spacenavigator.h>

namespace Vcl { namespace HID { namespace Windows
{
	class SpaceNavigatorHID : public AbstractHID, public SpaceNavigator
	{
	private:
		struct InputData 
		{
			//! Current time to live for telling
			//! if the device was unplugged while sending data
			int timeToLive;

			//! Indicate if the data is dirty
			bool isDirty;

			//! Axis data
			std::array<float,6> axes;

			//! Check if the data is zero
			bool isZero()
			{
				return (0 == axes[0] && 0 == axes[1] && 0 == axes[2] &&
						0 == axes[3] && 0 == axes[4] && 0 == axes[5]    );
			}

			//! Maximum time to live
			static const int MaxTimeToLive = 5;
		};

	public:
		SpaceNavigatorHID(std::unique_ptr<GenericHID> dev, bool poll_3d_mouse = false);
		
		//! Reset device when activating the program
		void onActivateApp(BOOL active, DWORD dwThreadID);

		//! Handle device input
		bool processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input) override;
		
	private:
		
		/*
		 *	on3DMouseInput() does all the preprocessing of the rawinput device data before
		 *	finally calling the move3D method.
		 *
		 *	If polling is enabled (Poll3DMouse==true) on3DMouseInput() is called from 
		 *	the windows timer message handler (onTimer())
		 *
		 *	If polling is NOT enabled (Poll3DMouse==false) on3DMouseInput() is called 
		 *	directly from the WM_INPUT handler (onRawInput())
		 */
		void on3DMouseInput();
		
		//////////////////////////////////////////////////////////////////////////////////
		// onSpaceMouseMove is invoked when new 3d mouse data is available.
		// Override this method to process 3d mouse 6dof data in the application
		// Input:
		//    HANDLE hDevice - the raw input device handle 
		//                     this can be used to identify the device the data is coming
		//                     from or simply ignored.
		//
		//    ARRAY_NS::array<float, 6> aMotionData - contains the displacement data, using
		//                     a right-handed coordinate system with z down
		//                     see 'Programing for the 3dmouse' document available at
		//                     www.3dconnexion.com.
		//                     aMotionData[0], aMotionData[1], aMotionData[2] is the
		//                     incremental pan zoom displacement vector (x,y,z).
		//                     aMotionData[3], aMotionData[4], aMotionData[5] is the
		//                     incremental rotation vector (NOT Euler angles)
		void onSpaceMouseMove(std::array<float, 6> motion_data);

		//////////////////////////////////////////////////////////////////////////////////
		// onSpaceMouseKeyDown processes the standard 3d mouse key presses
		// This method can be overwritten
		// Input:
		//    HANDLE hDevice - the raw input device handle 
		//                     this can be used to identify the device the data is coming
		//                     from or simply ignored.
		//   UINT nVirtualKey - Standard 3d mouse key code 
		void onSpaceMouseKeyDown(UINT virtual_key);

		//////////////////////////////////////////////////////////////////////////////////
		// onSpaceMouseKeyUp processes the standard 3d mouse key releases
		// This method can be overwritten
		// Input:
		//    HANDLE hDevice - the raw input device handle 
		//                     this can be used to identify the device the data is coming
		//                     from or simply ignored.
		//   UINT nVirtualKey - Standard 3d mouse key code 
		void onSpaceMouseKeyUp(UINT virtual_key);

	private:
		//! Process a raw input message
		bool translateRawInputData(UINT input_code, PRAWINPUT raw_input);

		//! Axis input data
		InputData _deviceData;

		//! Button input data
		uint32_t _keystate{ 0 };
		
		//! Last time the data was updated.
		//! Use to calculate distance traveled since last event
		DWORD _last3DMouseInputTime{ 0 };

	private: // Polling support methods

		//! Start the timer
		void startTimer(HWND hwnd);

		//! Timer callback
		void onTimer(UINT_PTR event_id);

		//! Kill the currently running timer
		void killPollingTimer();
		
	private: // Polling support
		//! 3D mouse is in polling mode
		bool _poll3DMouse{ false };
		
		//! Polling period. Default is 50 Hz)
		UINT _pollingPeriod3DMouse{ 20 };

		//! 3DMouse data polling timer
		//! Only used if _poll3DMouse == true
		UINT_PTR _timer3DMouse{ 0 };
	};
}}}
