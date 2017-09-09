/*  
 * This file is part of the Visual Computing Library (VCL) release under the
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
	//! Implementation of a 3Dconnexion space mouse
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
		
		/*!
		 * \brief Does all the preprocessing of the rawinput device data before
		 *        finally calling the move3D method.
		 *
		 * If polling is enabled (_poll3DMouse == true) this method is called
		 * from the windows timer message handler
		 * If polling is not enabled (_poll3DMouse == false) this method is
		 * called directly from the WM_INPUT handler
		 */
		void on3DMouseInput();
		
		/*!
		 * \brief onSpaceMouseMove is invoked when new 3d mouse data is
		 *        available.
		 * 
		 * \param motion_data Contains the displacement data, using a
		 *                    right-handed coordinate system with z down.
		 *                    See 'Programing for the 3dmouse' document
		 *                    available at www.3dconnexion.com.
		 *                    Entries 0, 1, 2 is the incremental pan zoom
		 *                    displacement vector (x,y,z).
		 *                    Entries 3, 4, 5 is the incremental rotation vector
		 *                    (NOT Euler angles).
		 */
		void onSpaceMouseMove(std::array<float, 6> motion_data);
		
		/*!
		 * \brief onSpaceMouseKeyDown processes the 3d mouse key presses
		 * 
		 * \param virtual_key 3d mouse key code 
		 */
		void onSpaceMouseKeyDown(UINT virtual_key);
		
		/*!
		 * \brief onSpaceMouseKeyUp processes the 3d mouse key releases
		 * 
		 * \param virtual_key 3d mouse key code 
		 */
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
