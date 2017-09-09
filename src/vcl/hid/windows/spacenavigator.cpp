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
#include "spacenavigator.h"

// Debug tracing
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_WM_INPUT_PERIOD 0
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_3DINPUT_PERIOD 0
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA 0
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_TYPE 0
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_RIDI_DEVICENAME 0
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_RIDI_DEVICEINFO 0
#define VCL_DEVICE_SPACENAVIGATOR_TRACE_KEYUP 0

// Configuration
#define VCL_DEVICE_SPACENAVIGATOR_CONSTANT_INPUT_PERIOD 0

namespace Vcl { namespace HID { namespace Windows
{
	SpaceNavigatorHID::SpaceNavigatorHID
	(
		std::unique_ptr<GenericHID> dev,
		bool poll_3d_mouse
	)
	: AbstractHID{std::move(dev)}
	, SpaceNavigator()
	, _poll3DMouse(poll_3d_mouse)
	{
		const auto names = device()->readDeviceName();
		setVendorName(names.first);
		setDeviceName(names.second);
		
		setNrAxes(static_cast<uint32_t>(device()->axes().size()));
		setNrButtons(static_cast<uint32_t>(device()->buttons().size()));

		// Initialize axis data
		_deviceData.axes.fill(0.0f);
				
		setAxisState(0, _deviceData.axes[0]);
		setAxisState(1, _deviceData.axes[1]);
		setAxisState(2, _deviceData.axes[2]);
				
		setAxisState(4, _deviceData.axes[3]);
		setAxisState(3, _deviceData.axes[4]);
		setAxisState(5, _deviceData.axes[5]);
	}
	
	void SpaceNavigatorHID::onActivateApp(BOOL active, DWORD)
	{
		if (!_poll3DMouse)
		{
			if (!active)
			{
				_last3DMouseInputTime = 0;
			}
		}
		
		// Initialize axis data
		_deviceData.axes.fill(0.0f);
				
		setAxisState(0, _deviceData.axes[0]);
		setAxisState(1, _deviceData.axes[1]);
		setAxisState(2, _deviceData.axes[2]);
				
		setAxisState(4, _deviceData.axes[3]);
		setAxisState(3, _deviceData.axes[4]);
		setAxisState(5, _deviceData.axes[5]);
	}

	bool SpaceNavigatorHID::processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input)
	{
		// We are not interested in keyboard or mouse data received via raw input
		if (raw_input->header.dwType != RIM_TYPEHID)
			return false;

		// Flag if we have new 6dof data and need to invoke the on3DMouseInput handler
		bool have_new_input = false;

		// Process the input data
		have_new_input = translateRawInputData(input_code, raw_input);

		// If we have mouse input data for the application then tell the application about it
		if (have_new_input)
		{
			// If we are polling and the timer is not running then start it
			if (_poll3DMouse)
			{
				startTimer(window_handle);
			}
			else
			{
				// Process the motion data
				on3DMouseInput();
			}
		}

		return true;
	}
	
	bool SpaceNavigatorHID::translateRawInputData(UINT input_code, PRAWINPUT raw_input)
	{
		bool is_foreground = (input_code == RIM_INPUT) || !_only_foreground;

#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_TYPE
		wprintf(L"Rawinput.header.dwType=0x%x\n", pRawInput->header.dwType);
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_TYPE

#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RIDI_DEVICENAME
		UINT dwSize=0;
		if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, NULL, &dwSize) == 0)
		{
			wchar_t* pszDeviceName = new wchar_t[dwSize+1];
			if (pszDeviceName.get())
			{
				if (::GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, pszDeviceName, &dwSize) > 0)
				{
					wprintf(L"Device Name = %s\nDevice handle = 0x%x\n", pszDeviceName, pRawInput->header.hDevice);
				}
			}
			delete[] pszDeviceName;
		}
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RIDI_DEVICENAME

#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RIDI_DEVICEINFO
			
		RID_DEVICE_INFO sRidDeviceInfo;
		sRidDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
		UINT cbSize = sizeof(RID_DEVICE_INFO);

		switch (sRidDeviceInfo.dwType)
		{
		case RIM_TYPEMOUSE:
			wprintf(L"\tsRidDeviceInfo.dwType=RIM_TYPEMOUSE\n");
		break;

		case RIM_TYPEKEYBOARD:
			wprintf(L"\tsRidDeviceInfo.dwType=RIM_TYPEKEYBOARD\n");
		break;

		case RIM_TYPEHID:
			wprintf(L"\tsRidDeviceInfo.dwType=RIM_TYPEHID\n");
			wprintf(L"\tVendor=0x%x\n\tProduct=0x%x\n\tUsagePage=0x%x\n\tUsage=0x%x\n",
					sRidDeviceInfo.hid.dwVendorId, 
					sRidDeviceInfo.hid.dwProductId, 
					sRidDeviceInfo.hid.usUsagePage,
					sRidDeviceInfo.hid.usUsage);
		break;
		}
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RIDI_DEVICEINFO

		if (raw_input->data.hid.bRawData[0] == 0x01) // Translation vector
		{
			_deviceData.timeToLive = InputData::MaxTimeToLive;
			if (is_foreground)
			{
				short* pnRawData = reinterpret_cast<short*>(&raw_input->data.hid.bRawData[1]);
				// Cache the pan zoom data
				_deviceData.axes[0] = normalizeAxis(pnRawData[0], device()->axes()[0]);
				_deviceData.axes[1] = normalizeAxis(pnRawData[1], device()->axes()[1]);
				_deviceData.axes[2] = normalizeAxis(pnRawData[2], device()->axes()[2]);
					
				setAxisState(0, _deviceData.axes[0]);
				setAxisState(1, _deviceData.axes[1]);
				setAxisState(2, _deviceData.axes[2]);
				
#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
				wprintf(L"Pan/Zoom RI Data =\t%d,\t%d,\t%d\n",
						pnRawData[0],
						pnRawData[1],
						pnRawData[2]);
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
				if (raw_input->data.hid.dwSizeHid >= 13) // Highspeed package
				{
					// Cache the rotation data
					_deviceData.axes[3] = normalizeAxis(pnRawData[3], device()->axes()[3]);
					_deviceData.axes[4] = normalizeAxis(pnRawData[4], device()->axes()[4]);
					_deviceData.axes[5] = normalizeAxis(pnRawData[5], device()->axes()[5]);
					_deviceData.isDirty = true;
#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
					wprintf(L"Rotation RI Data =\t%d,\t%d,\t%d\n",
						pnRawData[3],
						pnRawData[4],
						pnRawData[5]);
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
				
					setAxisState(4, _deviceData.axes[3]);
					setAxisState(3, _deviceData.axes[4]);
					setAxisState(5, _deviceData.axes[5]);
					return true;
				}
			}
			else
			{
				// Zero out the data if the app is not in forground
				_deviceData.axes.fill(0.f);
				
				setAxisState(0, _deviceData.axes[0]);
				setAxisState(1, _deviceData.axes[1]);
				setAxisState(2, _deviceData.axes[2]);
				
				setAxisState(4, _deviceData.axes[3]);
				setAxisState(3, _deviceData.axes[4]);
				setAxisState(5, _deviceData.axes[5]);
			}
		}
		else if (raw_input->data.hid.bRawData[0] == 0x02)  // Rotation vector
		{
			// If we are not in foreground do nothing 
			// The rotation vector was zeroed out with the translation vector in the previous message
			if (is_foreground)
			{
				_deviceData.timeToLive = InputData::MaxTimeToLive;

				short* pnRawData = reinterpret_cast<short*>(&raw_input->data.hid.bRawData[1]);
				// Cache the rotation data
				_deviceData.axes[3] = normalizeAxis(pnRawData[0], device()->axes()[3]);
				_deviceData.axes[4] = normalizeAxis(pnRawData[1], device()->axes()[4]);
				_deviceData.axes[5] = normalizeAxis(pnRawData[2], device()->axes()[5]);
				_deviceData.isDirty = true;
				
				setAxisState(4, _deviceData.axes[3]);
				setAxisState(3, _deviceData.axes[4]);
				setAxisState(5, _deviceData.axes[5]);

#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
				wprintf(L"Rotation RI Data =\t%d,\t%d,\t%d\n",
						pnRawData[0],
						pnRawData[1],
						pnRawData[2]);
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
				return true;
			}
		}
		/////////////////////////////////////////////////////////////////////////////////////////////
		// this is a package that contains 3d mouse keystate information
		// bit0=key1, bit=key2 etc.
		else if (raw_input->data.hid.bRawData[0] == 0x03)  // Keystate change
		{
			unsigned long dwKeystate = *reinterpret_cast<unsigned long*>(&raw_input->data.hid.bRawData[1]); 
#if VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA
			wprintf(L"ButtonData =0x%x\n", dwKeystate);
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_RI_RAWDATA

			// Store the new keystate
			setButtonStates(dwKeystate);

			// Log the keystate changes
			unsigned long dwOldKeystate = _keystate;
			if (dwKeystate != 0)
				_keystate = dwKeystate;
			else
				_keystate = 0;

			//  Only call the keystate change handlers if the app is in foreground
			if (is_foreground)
			{
				unsigned long dwChange = dwKeystate ^ dwOldKeystate;

				for (unsigned short key = 1; key < 33; key++)
				{
					if (dwChange & 0x01)
					{
						unsigned int virtual_key_code = HidToVirtualKey(device()->productId(), key);
						if (virtual_key_code)
						{
							if (dwKeystate & 0x01)
								onSpaceMouseKeyDown(virtual_key_code);
							else
								onSpaceMouseKeyUp(virtual_key_code);
						}
					}
					dwChange >>=1;
					dwKeystate >>=1;
				}
			}

			// Don't signal further input processing as buttons were handled above
		}
		return false;
	}
	
	void SpaceNavigatorHID::on3DMouseInput()
	{
		// Don't do any data processing in background
		bool is_foreground = (::GetActiveWindow() != NULL) || !_only_foreground;
		if (!is_foreground)
		{
			// Set all cached data to zero so that a zero event is seen 
			// and the cached data deleted
			_deviceData.axes.fill(0.0f);
			_deviceData.isDirty = true;
		}

		DWORD dwNow = ::GetTickCount();           // Current time;
		DWORD dwElapsedTime;                      // Elapsed time since we were last here

#if VCL_DEVICE_SPACENAVIGATOR_CONSTANT_INPUT_PERIOD
		if (t_bPoll3dmouse)
			dwElapsedTime = m3DMousePollingPeriod;
		else
			dwElapsedTime = 16;
#else
		if (0 == _last3DMouseInputTime)
		{
			dwElapsedTime = 10;                    // System timer resolution
		}
		else 
		{
			dwElapsedTime = dwNow - _last3DMouseInputTime;
			if (_last3DMouseInputTime > dwNow)
				dwElapsedTime = ~dwElapsedTime+1;

			if (dwElapsedTime < 1)
				dwElapsedTime = 1;
			// Check for wild numbers because the device was removed while sending data
			else if (dwElapsedTime > 500)
				dwElapsedTime = 10;
		}
#endif // VCL_DEVICE_SPACENAVIGATOR_CONSTANT_INPUT_PERIOD

#if VCL_DEVICE_SPACENAVIGATOR_TRACE_3DINPUT_PERIOD
		wprintf(L"On3DmouseInput() period is %dms\n", dwElapsedTime);
#endif // VCL_DEVICE_SPACENAVIGATOR_TRACE_3DINPUT_PERIOD

		float fMouseData2Rotation = AngularVelocity;

		// v = w * r,  we don't know r yet so lets assume r=1.)
		float fMouseData2PanZoom = AngularVelocity;

		// Take a look at the users preferred speed setting and adjust the sensitivity accordingly
		Speed speed_setting = _speed;
		// See "Programming for the 3D Mouse", Section 5.1.3
		float fSpeed = (speed_setting == Speed::Low ? 0.25f : speed_setting == Speed::High ?  4.0f : 1.0f);

		// Multiplying by the following will convert the 3d mouse data to real world units
		fMouseData2PanZoom *= fSpeed;
		fMouseData2Rotation *= fSpeed;

		bool process_device_data = true;

		// If we have not received data for a while send a zero event 
		if ((--(_deviceData.timeToLive)) == 0)
		{
			_deviceData.axes.fill(0.0f);
		}
		// If we are not polling then only handle the data that was actually received
		else if (!_poll3DMouse && !_deviceData.isDirty)
		{
			process_device_data = false;
		}

		if (process_device_data)
		{
			_deviceData.isDirty = false;

			////////////////////////////////////////////////////////////////////////////////
			// get a copy of the motion vectors and apply the user filters
			std::array<float, 6> motionData = _deviceData.axes;

			////////////////////////////////////////////////////////////////////////////////
			// apply the user filters

			// Pan Zoom filter
			// See "Programming for the 3D Mouse", Section 5.1.2
			if (!_isPanZoom)
			{
				// Pan zoom is switched off so set the translation vector values to zero
				motionData[0] =  motionData[1] =  motionData[2] = 0.0f;
			}

			// Rotate filter
			// See "Programming for the 3D Mouse", Section 5.1.1
			if (!_isRotate)
			{
				// Rotate is switched off so set the rotation vector values to zero
				motionData[3] =  motionData[4] =  motionData[5] = 0.0f;
			}

			// Convert the translation vector into physical data
			for (size_t axis = 0; axis < 3; axis++)
				motionData[axis] *= fMouseData2PanZoom;

			// Convert the directed Rotate vector into physical data
			// See "Programming for the 3D Mouse", Section 7.2.2
			for (size_t axis = 3; axis < 6; axis++)
				motionData[axis] *= fMouseData2Rotation;

			////////////////////////////////////////////////////////////////////////////
			// Now that the data has had the filters and sensitivty settings applied
			// calculate the displacements since the last view update
			for (size_t axis = 0; axis < 6; axis++)
				motionData[axis] *= static_cast<float>(dwElapsedTime);

			// Pass the 3dmouse input to the view controller
			onSpaceMouseMove(motionData);
		}

		if (!_deviceData.isZero())
		{
			_last3DMouseInputTime = dwNow;
		}
		else
		{  
			_last3DMouseInputTime = 0;
			killPollingTimer();
		}
	}

	void SpaceNavigatorHID::onSpaceMouseMove(std::array<float, 6> motion_data)
	{
		for (auto handler : _handlers)
			handler->onSpaceMouseMove(this, motion_data);
	}

	void SpaceNavigatorHID::onSpaceMouseKeyDown(UINT virtual_key)
	{
		for (auto handler : _handlers)
			handler->onSpaceMouseKeyDown(this, virtual_key);
	}

	void SpaceNavigatorHID::onSpaceMouseKeyUp(UINT virtual_key)
	{
		for (auto handler : _handlers)
			handler->onSpaceMouseKeyUp(this, virtual_key);
	}
	
	void SpaceNavigatorHID::startTimer(HWND hwnd)
	{
		if (!_timer3DMouse)
		{
			// Create a new unused timer
			_timer3DMouse = ::SetTimer(0, 0, USER_TIMER_MAXIMUM, nullptr);

			// Now set the HWND to the derived window and correct the timeout
			_timer3DMouse = ::SetTimer(hwnd, _timer3DMouse, _pollingPeriod3DMouse, NULL);
		}
	}

	void SpaceNavigatorHID::onTimer(UINT_PTR event_id) 
	{
		// Only if polling is enabled
		if (_poll3DMouse)
		{
			if (event_id == _timer3DMouse)
				on3DMouseInput();
		}
	}

	void SpaceNavigatorHID::killPollingTimer()
	{
		if (_poll3DMouse)
		{
			UINT_PTR timer = _timer3DMouse;
			_timer3DMouse = 0;
			if (timer)
				::KillTimer(nullptr, timer);
		}
	}
}}}
