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
#include "hid.h"

// C++ standard library
#include <iostream>

// Windows
#include <dinput.h>
#include <dinputd.h>

// VCL
#include <vcl/core/contract.h>
#include <vcl/math/ceil.h>
#include <vcl/hid/windows/spacenavigator.h>
#include <vcl/hid/gamepad.h>
#include <vcl/hid/joystick.h>
#include <vcl/hid/multiaxiscontroller.h>
#include <vcl/util/scopeguard.h>

// Missing typedef from Windows API
typedef unsigned __int64 QWORD;

namespace
{
	//! Workaround for incorrect alignment of the RAWINPUT structure on x64 os
	//! when running as Wow64.
	UINT getRawInputBuffer(HWND hWnd, PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader)
	{
#ifdef VCL_ABI_WIN64
		return ::GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
#else
		BOOL is_wow_64 = FALSE;
		::IsWow64Process(GetCurrentProcess(), &is_wow_64);
		if (!is_wow_64 || pData == NULL)
			return ::GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
		else
		{
			UINT cbDataSize=0;
			UINT nCount=0;
			PRAWINPUT pri = pData;

			MSG msg;
			while (PeekMessage(&msg, hWnd, WM_INPUT, WM_INPUT, PM_NOREMOVE))
			{
				HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(msg.lParam);
				UINT cbSize = *pcbSize - cbDataSize;
				if (::GetRawInputData(hRawInput, RID_INPUT, pri, &cbSize, cbSizeHeader) == static_cast<UINT>(-1))
				{
					if (nCount==0)
						return static_cast<UINT>(-1);
					else
						break;
				}
				++nCount;

				// Remove the message for the data just read
				PeekMessage(&msg, hWnd, WM_INPUT, WM_INPUT, PM_REMOVE);

				pri = NEXTRAWINPUTBLOCK(pri);
				cbDataSize = reinterpret_cast<ULONG_PTR>(pri) - reinterpret_cast<ULONG_PTR>(pData);
				if (cbDataSize >= *pcbSize)
				{
					cbDataSize = *pcbSize;
					break;
				}
			}
			return nCount;
		}
#endif
	}
}

namespace Vcl { namespace HID { namespace Windows
{
	// Direct-input button mapping
	struct DirectInputButtonMapping
	{
		WORD    usagePage;
		WORD    usage;
		wchar_t name[32];
	};

	// Direct-input axis mapping and calibration
	struct DirectInputAxisMapping
	{
		WORD                 usagePage;
		WORD                 usage;
		bool                 isCalibrated;
		DIOBJECTCALIBRATION  calibration;
		wchar_t              name[32];
	};

	GenericHID::GenericHID(HANDLE raw_handle)
	: _rawInputHandle(raw_handle)
	{
		// Access the object path of the device name
		wchar_t path_buffer[260 + 4];
		auto path_length = UINT(sizeof path_buffer / sizeof path_buffer[0]);
		auto bytes_copied = GetRawInputDeviceInfoW(_rawInputHandle, RIDI_DEVICENAME, path_buffer, &path_length);
		if (bytes_copied == UINT(-1))
		{
			return;
		}

		// Windows XP can return a '?' as second character: http://stackoverflow.com/q/10798798
		path_buffer[1] = L'\\';

		_fileHandle = CreateFileW(
			path_buffer, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			0u, nullptr
		);
		if (_fileHandle == INVALID_HANDLE_VALUE)
		{
			return;
		}
		
		// Read the device info in order to determine the exact device type
		RID_DEVICE_INFO dev_info = {};
		dev_info.cbSize = sizeof(RID_DEVICE_INFO);

		UINT dev_info_size = sizeof(RID_DEVICE_INFO);
		bytes_copied = GetRawInputDeviceInfoW(_rawInputHandle, RIDI_DEVICEINFO, &dev_info, &dev_info_size);
		if (bytes_copied == sizeof(RID_DEVICE_INFO))
		{
			_vendorId = dev_info.hid.dwVendorId;
			_productId = dev_info.hid.dwProductId;
		}

		auto caps = readDeviceCaps();

		DirectInputButtonMapping di_button_mapping[128] = {};
		DirectInputAxisMapping di_axis_mapping[7] = {};

		calibrateButtons(std::get<0>(caps), di_button_mapping);
		calibrateAxes(   std::get<1>(caps), di_axis_mapping);

		storeButtons(std::move(std::get<0>(caps)), di_button_mapping);
		storeAxes(   std::move(std::get<1>(caps)), di_axis_mapping);
	}

	auto GenericHID::readDeviceName() const -> std::pair<std::wstring, std::wstring>
	{
		wchar_t vendor_buffer[255];
		wchar_t device_buffer[255];
		if (HidD_GetManufacturerString(_fileHandle, vendor_buffer, 255) == FALSE)
		{
			wcscpy_s(vendor_buffer, L"(unknown)");
		}
		
		if (HidD_GetProductString(_fileHandle, device_buffer, 255) == FALSE)
		{
			wcscpy_s(device_buffer, L"(unknown)");
		}

		return std::make_pair(vendor_buffer, device_buffer);
	}

	auto GenericHID::readDeviceCaps() const
		-> std::tuple<std::vector<HIDP_BUTTON_CAPS>, std::vector<HIDP_VALUE_CAPS>>
	{
		PHIDP_PREPARSED_DATA preparsed_data;
		if (HidD_GetPreparsedData(_fileHandle, &preparsed_data) == FALSE)
		{
			return{};
		}

		// Free the allocated data structure at the end of the method
		VCL_SCOPE_EXIT { HidD_FreePreparsedData(preparsed_data); };

		HIDP_CAPS capabilities;
		if (HidP_GetCaps(preparsed_data, &capabilities) != HIDP_STATUS_SUCCESS)
		{
			return{};
		}

		std::vector<HIDP_BUTTON_CAPS> button_classes(capabilities.NumberInputButtonCaps);
		if (capabilities.NumberInputButtonCaps > 0 && 
			HidP_GetButtonCaps(
			HidP_Input, button_classes.data(), &capabilities.NumberInputButtonCaps, preparsed_data
		) != HIDP_STATUS_SUCCESS)
		{
			return{};
		}

		std::vector<HIDP_VALUE_CAPS> axis_classes(capabilities.NumberInputValueCaps);
		if (capabilities.NumberInputValueCaps > 0 &&
			HidP_GetValueCaps(
			HidP_Input, axis_classes.data(), &capabilities.NumberInputValueCaps, preparsed_data
		) != HIDP_STATUS_SUCCESS)
		{
			return{};
		}

		std::vector<HIDP_BUTTON_CAPS> output_button_classes(capabilities.NumberOutputButtonCaps);
		if (capabilities.NumberOutputButtonCaps > 0 &&
			HidP_GetButtonCaps(
			HidP_Output, output_button_classes.data(), &capabilities.NumberOutputButtonCaps, preparsed_data
		) != HIDP_STATUS_SUCCESS)
		{
			return{};
		}

		std::vector<HIDP_VALUE_CAPS> output_axis_classes(capabilities.NumberOutputValueCaps);
		if (capabilities.NumberOutputValueCaps > 0 && 
			HidP_GetValueCaps(
			HidP_Output, output_axis_classes.data(), &capabilities.NumberOutputValueCaps, preparsed_data
		) != HIDP_STATUS_SUCCESS)
		{
			return{};
		}

		// Initialize ranges for all buttons and axis'
		size_t nr_buttons = 0;
		for (auto& button : button_classes)
		{
			if (button.IsRange)
			{
				nr_buttons += button.Range.UsageMax - button.Range.UsageMin + 1u;
			}
			else
			{
				button.Range.UsageMin = button.Range.UsageMax = button.NotRange.Usage;
				button.Range.DataIndexMin = button.Range.DataIndexMax = button.NotRange.DataIndex;
				button.IsRange = TRUE;
				++nr_buttons;
			}
		}

		size_t nr_axes = 0;
		for (auto& axis : axis_classes)
		{
			if (axis.IsRange)
			{
				nr_axes += axis.Range.UsageMax - axis.Range.UsageMin + 1u;
			}
			else
			{
				axis.Range.UsageMin = axis.Range.UsageMax = axis.NotRange.Usage;
				axis.Range.DataIndexMin = axis.Range.DataIndexMax = axis.NotRange.DataIndex;
				axis.IsRange = TRUE;
				++nr_axes;
			}
		}

		return std::make_tuple(std::move(button_classes), std::move(axis_classes));
	}

	void GenericHID::calibrateAxes(std::vector<HIDP_VALUE_CAPS>& axes_caps, gsl::span<struct DirectInputAxisMapping> di_axis_mapping)
	{
		for (const auto& axis : axes_caps)
		{
			if (axis.UsagePage != HID_USAGE_PAGE_GENERIC)
			{
				continue;
			}

			const auto first_usage = axis.Range.UsageMin;
			const auto last_usage  = axis.Range.UsageMax;
			for (WORD current_usage = first_usage; current_usage <= last_usage; ++current_usage) {

				const auto index = unsigned(current_usage - HID_USAGE_GENERIC_X);
				if (index < 7)
				{
					di_axis_mapping[index].usagePage = HID_USAGE_PAGE_GENERIC;
					di_axis_mapping[index].usagePage = current_usage;
				}
			}
		}

		// In case there is no Z-Axis, the slider ist mapped to the empty position
		if (di_axis_mapping[2].usagePage == 0)
		{
			di_axis_mapping[2] = di_axis_mapping[6];
			di_axis_mapping[6].usagePage = 0;
			di_axis_mapping[6].usage = 0;
		}
		
		HIDD_ATTRIBUTES vendor_and_product_id;
		if (FALSE != HidD_GetAttributes(fileHandle(), &vendor_and_product_id))
		{
			wchar_t path[128] = L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_????&PID_????\\Axes\\?";
			path[84] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 12) & 0xF]);
			path[85] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 8) & 0xF]);
			path[86] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 4) & 0xF]);
			path[87] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 0) & 0xF]);
			path[93] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 12) & 0xF]);
			path[94] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 8) & 0xF]);
			path[95] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 4) & 0xF]);
			path[96] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 0) & 0xF]);

			for (size_t i = 0; i < sizeof di_axis_mapping / sizeof(di_axis_mapping[0]); ++i)
			{
				path[103] = wchar_t('0' + i);

				HKEY key = nullptr;
				if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &key) != 0)
				{
					// No calibration data was found
					continue;
				}
				{
					DWORD valueType = REG_NONE;
					DWORD valueSize = 0;
					RegQueryValueExW(key, L"", nullptr, &valueType, nullptr, &valueSize);
					if (REG_SZ == valueType && sizeof(di_axis_mapping[i].name) > valueSize)
					{
						RegQueryValueExW(key, L"", nullptr, &valueType, LPBYTE(di_axis_mapping[i].name), &valueSize);
					}
				}

				DIOBJECTATTRIBUTES mapping;
				DWORD              valueType = REG_NONE;
				DWORD              valueSize = 0;
				RegQueryValueExW(key, L"Attributes", nullptr, &valueType, nullptr, &valueSize);
				if (REG_BINARY == valueType && sizeof(mapping) == valueSize)
				{
					RegQueryValueExW(key, L"Attributes", nullptr, &valueType, LPBYTE(&mapping), &valueSize);
					if (0x15 > mapping.wUsagePage)
					{
						di_axis_mapping[i].usagePage = mapping.wUsagePage;
						di_axis_mapping[i].usage = mapping.wUsage;
					}
				}

				RegCloseKey(key);
			}

			wcscpy_s(path, L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput\\VID_????&PID_????\\Calibration\\0\\Type\\Axes\\?");
			path[83] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 12) & 0xF]);
			path[84] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >>  8) & 0xF]);
			path[85] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >>  4) & 0xF]);
			path[86] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >>  0) & 0xF]);
			path[92] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 12) & 0xF]);
			path[93] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >>  8) & 0xF]);
			path[94] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >>  4) & 0xF]);
			path[95] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >>  0) & 0xF]);

			for (size_t i = 0; i < sizeof(di_axis_mapping) / sizeof(di_axis_mapping[0]); ++i)
			{
				path[121] = wchar_t('0' + i);

				HKEY key = nullptr;
				if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, path, 0u, KEY_READ, &key))
				{
					auto & calibration = di_axis_mapping[i].calibration;
					DWORD  valueType = REG_NONE;
					DWORD  valueSize = 0;
					RegQueryValueExW(key, L"Calibration", nullptr, &valueType, nullptr, &valueSize);
					if (REG_BINARY == valueType && sizeof(calibration) == valueSize)
					{
						if (0 == RegQueryValueExW(key, L"Calibration", nullptr, &valueType, LPBYTE(&calibration), &valueSize))
						{
							di_axis_mapping[i].isCalibrated = true;
						}
					}

					RegCloseKey(key);
				}

			}
		}
	}

	void GenericHID::calibrateButtons(std::vector<HIDP_BUTTON_CAPS>& button_caps, gsl::span<struct DirectInputButtonMapping> di_button_mapping)
	{
		HIDD_ATTRIBUTES vendor_and_product_id;
		if (FALSE != HidD_GetAttributes(fileHandle(), &vendor_and_product_id))
		{
			wchar_t path[128] = L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_????&PID_????\\Axes\\?";
			path[84] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 12) & 0xF]);
			path[85] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 8) & 0xF]);
			path[86] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 4) & 0xF]);
			path[87] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.VendorID >> 0) & 0xF]);
			path[93] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 12) & 0xF]);
			path[94] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 8) & 0xF]);
			path[95] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 4) & 0xF]);
			path[96] = wchar_t("0123456789ABCDEF"[(vendor_and_product_id.ProductID >> 0) & 0xF]);

			wcscpy_s(path + 98, sizeof(path) / sizeof(path[0]) - 98, L"Buttons\\???");
			for (size_t i = 0u; i < sizeof di_button_mapping / sizeof(di_button_mapping[0]); ++i) {
				if (i >= 100) {
					path[106] = wchar_t('0' + i / 100u);
					path[107] = wchar_t('0' + i % 100u / 10u);
					path[108] = wchar_t('0' + i % 10u);
					path[109] = wchar_t('\0');
				}
				else if (i >= 10u) {
					path[106] = wchar_t('0' + i / 10u);
					path[107] = wchar_t('0' + i % 10u);
					path[108] = wchar_t('\0');
				}
				else {
					path[106] = wchar_t('0' + i);
					path[107] = wchar_t('\0');
				}

				HKEY key = nullptr;
				if (0 != RegOpenKeyExW(HKEY_CURRENT_USER, path, 0u, KEY_READ, &key)) {
					continue;
				}

				{
					DWORD valueType = REG_NONE;
					DWORD valueSize = 0;
					RegQueryValueExW(key, L"", nullptr, &valueType, nullptr, &valueSize);
					if (REG_SZ == valueType && sizeof(di_button_mapping[i].name) > valueSize)
					{
						RegQueryValueExW(key, L"", nullptr, &valueType, LPBYTE(di_button_mapping[i].name), &valueSize);
					}
				}

				DIOBJECTATTRIBUTES mapping;
				DWORD              valueType = REG_NONE;
				DWORD              valueSize = 0;
				RegQueryValueExW(key, L"Attributes", nullptr, &valueType, nullptr, &valueSize);
				if (REG_BINARY == valueType && sizeof(mapping) == valueSize) {

					RegQueryValueExW(key, L"Attributes", nullptr, &valueType, LPBYTE(&mapping), &valueSize);
					if (0x15 > mapping.wUsagePage) {
						di_button_mapping[i].usagePage = mapping.wUsagePage;
						di_button_mapping[i].usage = mapping.wUsage;
					}
				}

				RegCloseKey(key);
			}
		}
	}

	void GenericHID::storeButtons(std::vector<HIDP_BUTTON_CAPS>&& button_caps, gsl::span<struct DirectInputButtonMapping> di_button_mapping)
	{
		for (const auto& button_cap : button_caps)
		{
			const auto first_usage = button_cap.Range.UsageMin;
			const auto last_usage  = button_cap.Range.UsageMax;
			for (WORD current_usage = first_usage, current_index = button_cap.Range.DataIndexMin;
				current_usage <= last_usage;
				++current_usage, ++current_index
			)
			{
				// Check if the button name was overriden by direct input
				wchar_t const * di_name = L"";
				for (auto & mapping : di_button_mapping) {
					if (button_cap.UsagePage == mapping.usagePage && current_usage == mapping.usage) {
						di_name = mapping.name;
				
						// Skip this mapping in following runs
						mapping.usage = 0;
						break;
					}
				}

				Button button;
				button.usagePage = button_cap.UsagePage;
				button.usage = current_usage;
				button.index = current_index;
				button.name = di_name;

				_buttons.emplace_back(button);
			}
		}

		_buttonCaps = std::move(button_caps);
	}

	void GenericHID::storeAxes(std::vector<HIDP_VALUE_CAPS>&& axes_caps, gsl::span<struct DirectInputAxisMapping> di_axis_mapping)
	{
		for (const auto& axis_cap : axes_caps)
		{
			auto const firstUsage = axis_cap.Range.UsageMin;
			auto const lastUsage = axis_cap.Range.UsageMax;
			for (WORD current_usage = firstUsage, current_index = axis_cap.Range.DataIndexMin;
				current_usage <= lastUsage;
				++current_usage, ++current_index
				)
			{
				bool    is_calibrated = true;
				int32_t calibrated_minimum = axis_cap.LogicalMin;
				int32_t calibrated_maximum = axis_cap.LogicalMax;
				int32_t calibrated_center  = (axis_cap.LogicalMin + axis_cap.LogicalMax) / 2;
				wchar_t const * di_name = L"";
				
				for (auto & mapping : di_axis_mapping) {
					if (axis_cap.UsagePage == mapping.usagePage && current_usage == mapping.usage) {
						di_name = mapping.name;
						is_calibrated = mapping.isCalibrated;
						if (mapping.isCalibrated) {
							calibrated_minimum = mapping.calibration.lMin;
							calibrated_center = mapping.calibration.lCenter;
							calibrated_maximum = mapping.calibration.lMax;
						}
				
						// Skip this mapping in following runs
						mapping.usage = 0;
						break;
					}
				}

				Axis axis;
				axis.usagePage = axis_cap.UsagePage;
				axis.usage = current_usage;
				axis.index = current_index;
				axis.logicalMinimum = axis_cap.LogicalMin;
				axis.logicalMaximum = axis_cap.LogicalMax;
				axis.isCalibrated = is_calibrated;
				if (is_calibrated) {
					axis.logicalCalibratedMinimum = calibrated_minimum;
					axis.logicalCalibratedMaximum = calibrated_maximum;
					axis.logicalCalibratedCenter  = calibrated_center;
				}
				axis.physicalMinimum = axis_cap.PhysicalMin;
				axis.physicalMaximum = axis_cap.PhysicalMax;
				axis.name = di_name;

				_axes.push_back(axis);
			}
		}

		_axesCaps = std::move(axes_caps);
	}

	template<typename JoystickType>
	JoystickHID<JoystickType>::JoystickHID(std::unique_ptr<GenericHID> dev)
	: AbstractHID{std::move(dev)}
	, JoystickType()
	{
		static_assert(std::is_base_of<Joystick, JoystickType>::value, "JoystickType must be a joystick");
		
		const auto names = device()->readDeviceName();
		setVendorName(names.first);
		setDeviceName(names.second);

		setNrAxes(device()->axes().size());
		setNrButtons(device()->buttons().size());
	}

	template<typename JoystickType>
	bool JoystickHID<JoystickType>::processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input)
	{
		PHIDP_PREPARSED_DATA preparsed_data;
		if (HidD_GetPreparsedData(device()->fileHandle(), &preparsed_data) == FALSE)
		{
			return{};
		}

		// Free the allocated data structure at the end of the method
		VCL_SCOPE_EXIT{ HidD_FreePreparsedData(preparsed_data); };

		for (const auto& axis : device()->axes())
		{
			// Read the value of the axis
			ULONG value = 0;

			if (HidP_GetUsageValue(
				HidP_Input, axis.usagePage, 0,
				axis.usage, &value, preparsed_data,
				(PCHAR)raw_input->data.hid.bRawData, raw_input->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS)
			{
				switch (axis.usage)
				{
				case HID_USAGE_GENERIC_X:
					setAxisState(static_cast<uint32_t>(JoystickAxis::X), normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_Y:
					setAxisState(static_cast<uint32_t>(JoystickAxis::Y), normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_Z:
					setAxisState(static_cast<uint32_t>(JoystickAxis::Z), normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_RX:
					setAxisState(static_cast<uint32_t>(JoystickAxis::RX), normalizeAxis(value, axis));
					break;
				case HID_USAGE_GENERIC_RY:
					setAxisState(static_cast<uint32_t>(JoystickAxis::RY), normalizeAxis(value, axis));
					break;
				case HID_USAGE_GENERIC_RZ:
					setAxisState(static_cast<uint32_t>(JoystickAxis::RZ), normalizeAxis(value, axis));
					break;
				}
			}
		}

		// Output set
		std::bitset<32> button_states;

		USAGE usages[128];
		ULONG nr_usages = 128;
		for (const auto& button_caps : device()->buttonCaps())
		{
			if (HidP_GetUsages(
				HidP_Input, button_caps.UsagePage, 0,
				usages, &nr_usages, preparsed_data,
				(PCHAR)raw_input->data.hid.bRawData, raw_input->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS)
			{
				for (ULONG i = 0; i < std::min(nr_usages, 32ul); i++)
				{
					button_states[usages[i] - button_caps.Range.UsageMin] = true;
				}
			}
		}
		setButtonStates(std::move(button_states));

		return true;
	}

	template<typename GamepadType>
	GamepadHID<GamepadType>::GamepadHID(std::unique_ptr<GenericHID> dev)
	: AbstractHID{std::move(dev)}
	, GamepadType()
	{
		static_assert(std::is_base_of<Gamepad, GamepadType>::value, "GamepadType must be a gamepad");
		
		const auto names = device()->readDeviceName();
		setVendorName(names.first);
		setDeviceName(names.second);

		setNrAxes(device()->axes().size());
		setNrButtons(device()->buttons().size());
	}

	template<typename GamepadType>
	bool GamepadHID<GamepadType>::processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input)
	{
		// We are not interested in keyboard or mouse data received via raw input
		if (raw_input->header.dwType != RIM_TYPEHID)
			return false;

		PHIDP_PREPARSED_DATA preparsed_data;
		if (HidD_GetPreparsedData(device()->fileHandle(), &preparsed_data) == FALSE)
		{
			return{};
		}

		// Free the allocated data structure at the end of the method
		VCL_SCOPE_EXIT{ HidD_FreePreparsedData(preparsed_data); };

		for (const auto& axis : device()->axes())
		{
			// Read the value of the axis
			ULONG value = 0;

			if (HidP_GetUsageValue(
				HidP_Input, axis.usagePage, 0,
				axis.usage, &value, preparsed_data,
				(PCHAR)raw_input->data.hid.bRawData, raw_input->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS)
			{
				switch (axis.usage)
				{
				case HID_USAGE_GENERIC_X:
					setAxisState(static_cast<uint32_t>(GamepadAxis::X), normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_Y:
					setAxisState(static_cast<uint32_t>(GamepadAxis::Y), normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_Z:
					setAxisState(static_cast<uint32_t>(GamepadAxis::Z), normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_RX:
					setAxisState(static_cast<uint32_t>(GamepadAxis::RX), normalizeAxis(value, axis));
					break;
				case HID_USAGE_GENERIC_RY:
					setAxisState(static_cast<uint32_t>(GamepadAxis::RY), normalizeAxis(value, axis));
					break;
				case HID_USAGE_GENERIC_HATSWITCH:
					setHatState(value);
					break;
				default:
					VclDebugError("Not implemented");
				}
			}
		}

		// Output set
		std::bitset<32> button_states;

		USAGE usages[128];
		ULONG nr_usages = 128;
		for (const auto& button_caps : device()->buttonCaps())
		{
			if (HidP_GetUsages(
				HidP_Input, button_caps.UsagePage, 0,
				usages, &nr_usages, preparsed_data,
				(PCHAR)raw_input->data.hid.bRawData, raw_input->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS)
			{
				for (ULONG i = 0; i < std::min(nr_usages, 32ul); i++)
				{
					button_states[usages[i] - button_caps.Range.UsageMin] = true;
				}
			}
		}
		setButtonStates(std::move(button_states));

		return true;
	}
	
	template<typename ControllerType>
	MultiAxisControllerHID<ControllerType>::MultiAxisControllerHID(std::unique_ptr<GenericHID> dev)
	: AbstractHID{std::move(dev)}
	, ControllerType()
	{
		static_assert(std::is_base_of<MultiAxisController, ControllerType>::value, "ControllerType must be a multi-axis controller");
		
		const auto names = device()->readDeviceName();
		setVendorName(names.first);
		setDeviceName(names.second);

		setNrAxes(device()->axes().size());
		setNrButtons(device()->buttons().size());
	}
	
	template<typename ControllerType>
	bool MultiAxisControllerHID<ControllerType>::processInput(HWND window_handle, UINT input_code, PRAWINPUT raw_input)
	{
		// We are not interested in keyboard or mouse data received via raw input
		if (raw_input->header.dwType != RIM_TYPEHID)
			return false;

		return false;
	}
	
	DeviceManager::DeviceManager()
	{
		// Get a list of all devices provided by the raw input API
		UINT max_nr_HIDs = 0;
		if (GetRawInputDeviceList(nullptr, &max_nr_HIDs, sizeof(RAWINPUTDEVICELIST)) != 0)
		{
			return;
		}

		// Get a list of all HID descriptors
		std::vector<RAWINPUTDEVICELIST> descriptors(max_nr_HIDs);
		UINT nr_found_HIDs = GetRawInputDeviceList(descriptors.data(), &max_nr_HIDs, sizeof(RAWINPUTDEVICELIST));
		if (nr_found_HIDs == (UINT) -1)
		{
			return;
		}

		for (auto const& desc : descriptors)
		{
			// Read the device info in order to determine the exact device type
			RID_DEVICE_INFO dev_info = {};
			dev_info.cbSize = sizeof(RID_DEVICE_INFO);

			UINT dev_info_size = sizeof(RID_DEVICE_INFO);
			auto bytes_copied = GetRawInputDeviceInfoW(desc.hDevice, RIDI_DEVICEINFO, &dev_info, &dev_info_size);
			if (bytes_copied != sizeof(RID_DEVICE_INFO))
			{
				continue;
			}

			/*if (dev_info.dwType == RIM_TYPEMOUSE)
			{
				_devices.emplace_back(std::make_unique<GenericHID>(desc.hDevice));
			}
			else if (dev_info.dwType == RIM_TYPEKEYBOARD)
			{
				_devices.emplace_back(std::make_unique<GenericHID>(desc.hDevice));
			}
			else*/ if (dev_info.dwType == RIM_TYPEHID)
			{
				if (dev_info.hid.usUsagePage == 1)
				{
					// Instantiate the generic HID and pass it to the actual
					// implemenation.
					auto hid = std::make_unique<GenericHID>(desc.hDevice);

					switch (dev_info.hid.usUsage)
					{
					case 0x04:
					{
						_devices.emplace_back(std::make_unique<JoystickHID<Joystick>>(std::move(hid)));
						break;
					}
					case 0x05:
					{
						_devices.emplace_back(std::make_unique<GamepadHID<Gamepad>>(std::move(hid)));
						break;
					}
					case 0x08:
					{
						const auto names = hid->readDeviceName();
						if (dev_info.hid.dwVendorId == SpaceNavigator::LogitechVendorID &&
							names.first == L"3Dconnexion" && names.second == L"SpaceNavigator")
						{
							_devices.emplace_back(std::make_unique<SpaceNavigatorHID>(std::move(hid)));
						}
						else
						{
							_devices.emplace_back(std::make_unique<MultiAxisControllerHID<MultiAxisController>>(std::move(hid)));
						}
						break;
					}
					}
					// Store a link to the created device for external use
					_deviceLinks.emplace_back(dynamic_cast<Device*>(_devices.back().get()));
				}
			}
		}
	}

	gsl::span<Device const* const> DeviceManager::devices() const
	{
		Device const* const* ptr = _deviceLinks.data();
		return gsl::make_span<Device const* const>(ptr, _deviceLinks.size());
	}

	bool DeviceManager::poll(HWND window_handle, UINT input_code)
	{
		UINT input_buffer_size;
		if (GetRawInputBuffer(nullptr, &input_buffer_size, sizeof(RAWINPUTHEADER)) != 0)
			return false;

		// According to the MSDN documentation use '*pcbSize * 8'.
		// Add an additional 8 bytes for alignment
		input_buffer_size *= 9;

		auto raw_input_buffer = std::make_unique<uint8_t[]>(input_buffer_size);
		auto aligned_raw_input_buffer = Mathematics::ceil<8>(reinterpret_cast<uint64_t>(raw_input_buffer.get()));
		auto raw_input = reinterpret_cast<PRAWINPUT>(aligned_raw_input_buffer);
		UINT nr_buffers = GetRawInputBuffer(raw_input, &input_buffer_size, sizeof(RAWINPUTHEADER));
		if (nr_buffers == UINT_MAX)
			return false;

		for (UINT i = 0; i < nr_buffers; i++)
		{
			// Pass the input data to the correct device
			auto dev_it = std::find_if(_devices.begin(), _devices.end(), [&raw_input](const auto& device)
			{
				return device->device()->rawHandle() == raw_input->header.hDevice;
			});

			bool processed = false;
			if (dev_it != _devices.end())
				processed = (*dev_it)->processInput(window_handle, input_code, raw_input);
			
			// Clean the buffer
			if (!processed)
				::DefRawInputProc(&raw_input, 1, sizeof(RAWINPUTHEADER)); 

			raw_input = NEXTRAWINPUTBLOCK(raw_input);
		}

		return true;
	}
	
	void DeviceManager::registerDevices(Flags<DeviceType> device_types, HWND window_handle)
	{
		std::vector<RAWINPUTDEVICE> input_requests;
		input_requests.reserve(DeviceType::Count);

		for (size_t i = 0; i < DeviceType::Count; i++)
		{
			if (device_types.isSet((DeviceType::Enum) (1 << i)))
			{
				switch (1 << i)
				{
				case DeviceType::Mouse:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x02, RIDEV_INPUTSINK | RIDEV_NOLEGACY, window_handle });
					break;
				case DeviceType::Keyboard:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x06, RIDEV_INPUTSINK | RIDEV_NOLEGACY, window_handle });
					break;
				case DeviceType::Joystick:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x04, RIDEV_INPUTSINK, window_handle });
					break;
				case DeviceType::Gamepad:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x05, RIDEV_INPUTSINK, window_handle });
					break;
				case DeviceType::MultiAxisController:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x08, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window_handle });
					break;
				}
			}
		}

		if (RegisterRawInputDevices(input_requests.data(), input_requests.size(), sizeof(RAWINPUTDEVICE)) == FALSE)
		{
		}
	}

	bool DeviceManager::processInput(HWND window_handle, UINT message, WPARAM wide_param, LPARAM low_param)
	{
		VclRequire(message == WM_INPUT, "Is called while processing WM_INPUT");

		// Format the Windows message parameters
		UINT input_code = static_cast<UINT>(GET_RAWINPUT_CODE_WPARAM(wide_param)); // 0 - foreground, 1 - background
		HRAWINPUT raw_input_handle = reinterpret_cast<HRAWINPUT>(low_param);

		// Determine the necessary buffer size
		UINT buffer_size = 0;
		GetRawInputData(raw_input_handle, RID_INPUT, nullptr, &buffer_size, sizeof(RAWINPUTHEADER));

		// Allocate enough space to hold the input data
		HANDLE heap_handle = GetProcessHeap();
		PRAWINPUT raw_input = (PRAWINPUT)HeapAlloc(heap_handle, 0, buffer_size);
		if (!raw_input)
			return false;

		// Read the input data
		GetRawInputData(raw_input_handle, RID_INPUT, raw_input, &buffer_size, sizeof(RAWINPUTHEADER));

		// Pass the input data to the correct device
		auto dev_it = std::find_if(_devices.begin(), _devices.end(), [&raw_input](const auto& device)
		{
			return device->device()->rawHandle() == raw_input->header.hDevice;
		});

		bool processed = false;
		if (dev_it != _devices.end())
		{
			processed = (*dev_it)->processInput(window_handle, input_code, raw_input);
		}

		// Clean the buffer
		if (!processed)
			::DefRawInputProc(&raw_input, 1, sizeof(RAWINPUTHEADER));
		HeapFree(heap_handle, 0, raw_input);

		// Check if any other input messages are still in the pipeline
		poll(window_handle, input_code);

		return processed;
	}
}}}
