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
#include <vcl/hid/joystick.h>

namespace Vcl { namespace HID { namespace Windows
{
	/// \note Structure definition taken from
	/// https://zfx.info/viewtopic.php?f=11&t=2977
	/// https://www.codeproject.com/Articles/297312/Minimal-Key-Logger-using-RAWINPUT
	/// https://www.codeproject.com/Articles/185522/Using-the-Raw-Input-API-to-Process-Joystick-Input
	struct Axis
	{
		/// Usage page as defined in the standard (e.g. "generic (0001)")
		USAGE usagePage;

		/// Usage of the axis as defined in the standard (e.g. "slider (0036)")
		USAGE usage;

		/// Index as defined through Hidp_GetData()
		USHORT index;



		// Umfang der logischen Werte dieser Achse. Liegt der tatsächliche Wert außerhalb dieses
		//  Bereichs, müssen wir annehmen, dass die Achse „nicht gesetzt“ ist. Das passiert z. B.
		//  bei Rundblickschaltern, wenn man sie nicht drückt.
		// Die HID API behandelt diese Werte durchgängig unsigned, aber die USB-HID-Spezifikation
		//  erinnert ausdrücklich, dass hier negative Werte auftreten können.
		int32_t                logicalMinimum;
		int32_t                logicalMaximum;
		// Ob wir DirectInput-Kalibrierungsdaten für diese Achse gefunden haben.
		bool                   isCalibrated;
		// Durch DirectInput-Kalibrierung bestimmter Bereich der Achse, falls sie kalibriert ist.
		int32_t                logicalCalibratedMinimum;
		int32_t                logicalCalibratedMaximum;
		// Durch DirectInput-Kalibrierung bestimmte Mittelstellung, falls die Achse kalibriert ist.
		int32_t                logicalCalibratedCenter;


		/// Physical minimum value
		int32_t physicalMinimum;

		/// Physical maxiumum value
		int32_t physicalMaximum;

		/// Name as given by the driver
		std::wstring name;
	};

	struct Button
	{
		/// Usage page as defined in the standard (e.g. "buttons (0009)")
		USAGE usagePage;

		/// Usage of the axis as defined in the standard (e.g. "secondary (0002)")
		USAGE usage;

		/// Index as defined through Hidp_GetData()
		USHORT index;

		/// Name as given by the driver
		std::wstring name;
	};

	class GenericHID : public virtual Device
	{
	public:
		GenericHID(HANDLE raw_handle);

		HANDLE rawHandle() const { return _rawInputHandle; }

		virtual bool processInput(PRAWINPUT raw_input);

	protected:
		HANDLE fileHandle() const { return _fileHandle; }

		const std::vector<Axis>& axes() const { return _axes; }
		const std::vector<Button>& buttons() const { return _buttons; }

		const std::vector<HIDP_VALUE_CAPS>& axisCaps() const { return _axesCaps; }
		const std::vector<HIDP_BUTTON_CAPS>& buttonCaps() const { return _buttonCaps; }

		std::vector<USAGE>& buttonStates() { return _buttonStates; }

	private:
		/// Read the device name from the hardware
		/// \returns the vendor defined name
		auto readDeviceName() const -> std::pair<std::wstring, std::wstring>;

		/// Read the device capabilities
		auto readDeviceCaps() const -> std::tuple<std::vector<HIDP_BUTTON_CAPS>, std::vector<HIDP_VALUE_CAPS>>;

		/// Map buttons and fetch calibration data
		/// @param button_caps
		void calibrateButtons(std::vector<HIDP_BUTTON_CAPS>& button_caps);

		/// Map axes and fetch calibration data
		/// @param axes_caps
		void calibrateAxes(std::vector<HIDP_VALUE_CAPS>& axes_caps);

		/// Convert and store the button caps
		/// @param button_caps
		void storeButtons(std::vector<HIDP_BUTTON_CAPS>&& button_caps);

		/// Convert and store the axes caps
		/// @param axes_caps
		void storeAxes(std::vector<HIDP_VALUE_CAPS>&& axes_caps);

	private:
		/// Handle provided by the raw input API
		HANDLE _rawInputHandle{ nullptr };

		/// Handle from the file API
		HANDLE _fileHandle{ nullptr };

		/// Buttons associated with the device
		std::vector<Button> _buttons;

		/// Axes associated with the device
		std::vector<Axis> _axes;

		/// HID button representation
		std::vector<HIDP_BUTTON_CAPS> _buttonCaps;

		/// HID axis representation
		std::vector<HIDP_VALUE_CAPS> _axesCaps;

		/// Button states
		std::vector<USAGE> _buttonStates;
	};

	class JoystickHID : public GenericHID, public Joystick
	{
	public:
		JoystickHID(HANDLE raw_handle);

		bool processInput(PRAWINPUT raw_input) override;
	};

	class GamepadHID : public GenericHID
	{
	public:
		GamepadHID(HANDLE raw_handle);


		bool processInput(PRAWINPUT raw_input) override;
	};

	class DeviceManager
	{
	public:
		DeviceManager();
	
		gsl::span<Device const* const> devices() const;

		void registerDevices(Flags<DeviceType> device_types, HWND hWnd);

		bool processInput(HWND window_handle, UINT message, WPARAM wide_param, LPARAM low_param);

	private:
		/// List of Windows HID
		std::vector<std::unique_ptr<GenericHID>> _devices;

		/// List of device pointers (links to `_devices`)
		std::vector<Device*> _deviceLinks;
	};
}}}
