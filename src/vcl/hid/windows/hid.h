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
#include <string>
#include <tuple>
#include <vector>

// GSL
#include <gsl/gsl>

// Windows API
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
extern "C" {
#    include <hidsdi.h>
}

// VCL
#include <vcl/hid/device.h>

namespace Vcl { namespace HID { namespace Windows
{
	//! \note Structure definition taken from
	//! https://zfx.info/viewtopic.php?f=11&t=2977
	//! https://www.codeproject.com/Articles/297312/Minimal-Key-Logger-using-RAWINPUT
	//! https://www.codeproject.com/Articles/185522/Using-the-Raw-Input-API-to-Process-Joystick-Input
	struct Axis
	{
		//! Usage page as defined in the standard (e.g. "generic (0001)")
		USAGE usagePage;

		//! Usage of the axis as defined in the standard (e.g. "slider (0036)")
		USAGE usage;

		//! Index as defined through Hidp_GetData()
		USHORT index;
		
		//! Minimum value defined by the HID device
		int32_t                logicalMinimum;
		
		//! Maximum value defined by the HID device
		int32_t                logicalMaximum;

		//! Indicate wheter DirectInput calibration data was applied
		bool                   isCalibrated;
		
		//! Minimum value after calibration
		int32_t                logicalCalibratedMinimum;
		
		//! Maximum value after calibration
		int32_t                logicalCalibratedMaximum;

		//! Through calibration defined center value of the axis
		int32_t                logicalCalibratedCenter;

		//! Physical minimum value
		int32_t physicalMinimum;

		//! Physical maxiumum value
		int32_t physicalMaximum;

		//! Name as given by the driver
		std::wstring name;
	};

	struct Button
	{
		//! Usage page as defined in the standard (e.g. "buttons (0009)")
		USAGE usagePage;

		//! Usage of the axis as defined in the standard (e.g. "secondary (0002)")
		USAGE usage;

		//! Index as defined through Hidp_GetData()
		USHORT index;

		//! Name as given by the driver
		std::wstring name;
	};

	class GenericHID
	{
	public:
		GenericHID(HANDLE raw_handle);

		HANDLE rawHandle() const { return _rawInputHandle; }

		//! Access the windows internal handle
		//! \returns The file handle to the device
		HANDLE fileHandle() const { return _fileHandle; }

		//! Access the vendor ID
		//! \returns The vendor ID
		DWORD vendorId() const { return _vendorId; }

		//! Access the product ID
		//! \returns The product ID
		DWORD productId() const { return _productId; }

		//! Read the device name from the hardware
		//! \returns The vendor defined names (vendor, product)
		auto readDeviceName() const -> std::pair<std::wstring, std::wstring>;

		const std::vector<Axis>& axes() const { return _axes; }
		const std::vector<Button>& buttons() const { return _buttons; }

		const std::vector<HIDP_VALUE_CAPS>& axisCaps() const { return _axesCaps; }
		const std::vector<HIDP_BUTTON_CAPS>& buttonCaps() const { return _buttonCaps; }

	private:
		
		//! Read the device capabilities
		auto readDeviceCaps() const -> std::tuple<std::vector<HIDP_BUTTON_CAPS>, std::vector<HIDP_VALUE_CAPS>>;

		//! Map buttons and fetch calibration data
		//! @param button_caps
		//! @param mapping Direct Input related remapping of buttons
		void calibrateButtons(std::vector<HIDP_BUTTON_CAPS>& button_caps, gsl::span<struct DirectInputButtonMapping> mapping);

		//! Map axes and fetch calibration data
		//! @param axes_caps
		void calibrateAxes(std::vector<HIDP_VALUE_CAPS>& axes_caps, gsl::span<struct DirectInputAxisMapping> mapping);

		//! Convert and store the button caps
		//! @param button_caps
		//! @param mapping Direct Input related remapping of buttons
		void storeButtons(std::vector<HIDP_BUTTON_CAPS>&& button_caps, gsl::span<struct DirectInputButtonMapping> mapping);

		//! Convert and store the axes caps
		//! @param axes_caps
		void storeAxes(std::vector<HIDP_VALUE_CAPS>&& axes_caps, gsl::span<struct DirectInputAxisMapping> mapping);

	private:
		//! Handle provided by the raw input API
		HANDLE _rawInputHandle{ nullptr };

		//! Handle from the file API
		HANDLE _fileHandle{ nullptr };

		//! Vendor ID
		DWORD _vendorId;

		//! Product ID
		DWORD _productId;

		//! Buttons associated with the device
		std::vector<Button> _buttons;

		//! Axes associated with the device
		std::vector<Axis> _axes;

		//! HID button representation
		std::vector<HIDP_BUTTON_CAPS> _buttonCaps;

		//! HID axis representation
		std::vector<HIDP_VALUE_CAPS> _axesCaps;
	};
	
	class AbstractHID
	{
	public:
		AbstractHID(std::unique_ptr<GenericHID> device) : _device{ std::move(device) } {}

		const GenericHID* device() const { return _device.get(); }

		virtual bool processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input) = 0;

	private:
		//! Actual hardware device implementation
		std::unique_ptr<GenericHID> _device;
	};

	template<typename JoystickType>
	class JoystickHID : public AbstractHID, public JoystickType
	{
	public:
		JoystickHID(std::unique_ptr<GenericHID> device);

		bool processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input) override;
	};

	template<typename GamepadType>
	class GamepadHID : public AbstractHID, public GamepadType
	{
	public:
		GamepadHID(std::unique_ptr<GenericHID> device);

		bool processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input) override;
	};
	
	template<typename ControllerType>
	class MultiAxisControllerHID : public AbstractHID, public ControllerType
	{
	public:
		MultiAxisControllerHID(std::unique_ptr<GenericHID> device);

		bool processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input) override;
	};

	class DeviceManager
	{
	public:
		DeviceManager();
	
		gsl::span<Device const* const> devices() const;
		
		//! Register devices with a specific window
		//! \param device_types Types of devices for which input should
		//!                     be processed.
		//! \param window_handle Handle to the window to which devices should be
		//!                      registered.
		void registerDevices(Flags<DeviceType> device_types, HWND window_handle);
		
		//! Polls all the devices instead of processing only a single device
		//! \param window_handle Handle of the window calling this method
		//! \returns True, if any device was successfully polled.
		//! \note This method is implemented according to the documenation
		//!       on MSDN. However, it seems not possible to call it.
		bool poll(HWND window_handle, UINT input_code);

		//! Process the input of a specific device
		bool processInput(HWND window_handle, UINT message, WPARAM wide_param, LPARAM low_param);

	private:
		//! List of Windows HID
		std::vector<std::unique_ptr<AbstractHID>> _devices;

		//! List of device pointers (links to `_devices`)
		std::vector<Device*> _deviceLinks;
	};
}}}
