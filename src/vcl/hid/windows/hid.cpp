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
#include <vcl/util/scopeguard.h>

namespace Vcl { namespace HID { namespace Windows
{
	GenericHID::GenericHID(HANDLE raw_handle)
	: Device(DeviceType::Undefined)
	, _rawInputHandle(raw_handle)
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
			FILE_SHARE_READ | FILE_SHARE_WRITE, // wir können anderen Prozessen nicht verbieten, das HID zu benutzen
			nullptr,
			OPEN_EXISTING,
			0u, nullptr
		);
		if (_fileHandle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		const auto names = readDeviceName();
		setVendorName(names.first);
		setDeviceName(names.second);

		auto caps = readDeviceCaps();

		calibrateButtons(std::get<0>(caps));
		calibrateAxes(   std::get<1>(caps));

		storeButtons(std::move(std::get<0>(caps)));
		storeAxes(   std::move(std::get<1>(caps)));
	}

	bool GenericHID::processInput(PRAWINPUT raw_input)
	{
		return false;
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

	void GenericHID::calibrateAxes(std::vector<HIDP_VALUE_CAPS>& axes_caps)
	{
		struct MappingAndCalibration
		{
			WORD                 usagePage;
			WORD                 usage;
			bool                 isCalibrated;
			DIOBJECTCALIBRATION  calibration;
			wchar_t              name[32];
		} dInputAxisMapping[7] = {};

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
					dInputAxisMapping[index].usagePage = HID_USAGE_PAGE_GENERIC;
					dInputAxisMapping[index].usagePage = current_usage;
				}
			}
		}

		// In case there is no Z-Axis, the slider ist mapped to the empty position
		if (dInputAxisMapping[2].usagePage == 0)
		{
			dInputAxisMapping[2] = dInputAxisMapping[6];
			dInputAxisMapping[6].usagePage = 0;
			dInputAxisMapping[6].usage = 0;
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

			for (size_t i = 0; i < sizeof dInputAxisMapping / sizeof dInputAxisMapping[0]; ++i)
			{
				path[103] = wchar_t('0' + i);

				HKEY key = nullptr;
				if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &key) != 0)
				{
					// No calibratino data was found
					continue;
				}
				{
					DWORD valueType = REG_NONE;
					DWORD valueSize = 0;
					RegQueryValueExW(key, L"", nullptr, &valueType, nullptr, &valueSize);
					if (REG_SZ == valueType && sizeof dInputAxisMapping[i].name > valueSize)
					{
						RegQueryValueExW(key, L"", nullptr, &valueType, LPBYTE(dInputAxisMapping[i].name), &valueSize);
						// Der Name wurde bereits bei der Erzeugung genullt; kein Grund, die Null manuell anzufügen.
					}
				}

				DIOBJECTATTRIBUTES mapping;
				DWORD              valueType = REG_NONE;
				DWORD              valueSize = 0;
				RegQueryValueExW(key, L"Attributes", nullptr, &valueType, nullptr, &valueSize);
				if (REG_BINARY == valueType && sizeof mapping == valueSize)
				{
					RegQueryValueExW(key, L"Attributes", nullptr, &valueType, LPBYTE(&mapping), &valueSize);
					if (0x15 > mapping.wUsagePage) { // Gültige Usage Page?
						dInputAxisMapping[i].usagePage = mapping.wUsagePage;
						dInputAxisMapping[i].usage = mapping.wUsage;
					}

					// Hier solltet ihr Debug-Informationen ausgeben um das Ergebnis zu kontrollieren …
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

			for (size_t i = 0; i < sizeof dInputAxisMapping / sizeof dInputAxisMapping[0]; ++i)
			{
				path[121] = wchar_t('0' + i);

				bool success = false;

				HKEY key = nullptr;
				if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, path, 0u, KEY_READ, &key))
				{
					auto & calibration = dInputAxisMapping[i].calibration;
					DWORD  valueType = REG_NONE;
					DWORD  valueSize = 0;
					RegQueryValueExW(key, L"Calibration", nullptr, &valueType, nullptr, &valueSize);
					if (REG_BINARY == valueType && sizeof calibration == valueSize) {

						if (0 == RegQueryValueExW(key, L"Calibration", nullptr, &valueType, LPBYTE(&calibration), &valueSize)) {
							dInputAxisMapping[i].isCalibrated = true;
						}

					}

					RegCloseKey(key);
				}

			}
		}
	}

	void GenericHID::calibrateButtons(std::vector<HIDP_BUTTON_CAPS>& button_caps)
	{
		struct Mapping {
			WORD    usagePage; // Null falls unbenutzt
			WORD    usage;
			wchar_t name[32];
		} dInputButtonMapping[128] = {}; // Nullinitialisierung

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
			for (size_t i = 0u; i < sizeof dInputButtonMapping / sizeof dInputButtonMapping[0]; ++i) {
				if (i >= 100) { // Dreistelliger Name?
					path[106] = wchar_t('0' + i / 100u);
					path[107] = wchar_t('0' + i % 100u / 10u);
					path[108] = wchar_t('0' + i % 10u);
					path[109] = wchar_t('\0');
				}
				else if (i >= 10u) { // Zweistelliger Name?
					path[106] = wchar_t('0' + i / 10u);
					path[107] = wchar_t('0' + i % 10u);
					path[108] = wchar_t('\0');
				}
				else { // Einstelliger Name?
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
					if (REG_SZ == valueType && sizeof dInputButtonMapping[i].name > valueSize) {
						RegQueryValueExW(key, L"", nullptr, &valueType, LPBYTE(dInputButtonMapping[i].name), &valueSize);
					}
				}

				DIOBJECTATTRIBUTES mapping;
				DWORD              valueType = REG_NONE;
				DWORD              valueSize = 0;
				RegQueryValueExW(key, L"Attributes", nullptr, &valueType, nullptr, &valueSize);
				if (REG_BINARY == valueType && sizeof mapping == valueSize) {

					RegQueryValueExW(key, L"Attributes", nullptr, &valueType, LPBYTE(&mapping), &valueSize);
					if (0x15 > mapping.wUsagePage) { // Gültige Usage Page?
						dInputButtonMapping[i].usagePage = mapping.wUsagePage;
						dInputButtonMapping[i].usage = mapping.wUsage;
					}

					// Nochmal Debug-Informationen!
				}

				RegCloseKey(key);
			} // for jeden Knopf

		}
	}

	void GenericHID::storeButtons(std::vector<HIDP_BUTTON_CAPS>&& button_caps)
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
				//for (auto & mapping : dInputButtonMapping) {
				//	if (currentClass.UsagePage == mapping.usagePage && currentUsage == mapping.usage) {
				//		toName = mapping.name;
				//
				//		mapping.usage = 0; // Optimierung: bei zukünfigen Durchläufen überspringen
				//		break;
				//	}
				//}

				Button button;
				button.usagePage = button_cap.UsagePage;
				button.usage = current_usage;
				button.index = current_index;
				button.name = di_name;

				_buttons.emplace_back(button);
			}
		}

		_buttonCaps = std::move(button_caps);
		_buttonStates.resize(_buttons.size(), 0);
	}

	void GenericHID::storeAxes(std::vector<HIDP_VALUE_CAPS>&& axes_caps)
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
				int32_t calibrated_minimum = 0;
				int32_t calibrated_maximum = 1 <<  axis_cap.BitSize;
				int32_t calibrated_center  = 1 << (axis_cap.BitSize - 1);
				wchar_t const * di_name = L"";
				
				// Wurden Kalibrierungsdaten oder Name überschrieben?
				//for (auto & mapping : dInputAxisMapping) {
				//	if (currentClass.UsagePage == mapping.usagePage && currentUsage == mapping.usage) {
				//		toName = mapping.name;
				//		isCalibrated = mapping.isCalibrated;
				//		if (mapping.isCalibrated) {
				//			calibratedMinimum = mapping.calibration.lMin;
				//			calibratedCenter = mapping.calibration.lCenter;
				//			calibratedMaximum = mapping.calibration.lMax;
				//		}
				//
				//		mapping.usage = 0; // Optimierung: bei zukünfigen Durchläufen überspringen
				//		break;
				//	}
				//}

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

	namespace
	{
		float normalizeAxis(ULONG value, const Axis& axis)
		{
			if (value < axis.logicalCalibratedCenter)
			{
				float range = axis.logicalCalibratedCenter - axis.logicalCalibratedMinimum;
				return ((LONG)value - axis.logicalCalibratedCenter) / range;
			}
			else
			{
				float range = axis.logicalCalibratedMaximum - axis.logicalCalibratedCenter;
				return ((LONG)value - axis.logicalCalibratedCenter) / range;
			}
		}
	}

	JoystickHID::JoystickHID(HANDLE raw_handle)
	: GenericHID(raw_handle)
	, Joystick()
	, Device(DeviceType::Joystick)
	{
		setNrAxes(axes().size());
		setNrButtons(buttons().size());
	}

	bool JoystickHID::processInput(PRAWINPUT raw_input)
	{
		PHIDP_PREPARSED_DATA preparsed_data;
		if (HidD_GetPreparsedData(fileHandle(), &preparsed_data) == FALSE)
		{
			return{};
		}

		// Free the allocated data structure at the end of the method
		VCL_SCOPE_EXIT{ HidD_FreePreparsedData(preparsed_data); };

		for (const auto& axis : axes())
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
					setAxisState(JoystickAxis::X, normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_Y:
					setAxisState(JoystickAxis::Y, normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_Z:
					setAxisState(JoystickAxis::Z, normalizeAxis(value, axis));
					break;

				case HID_USAGE_GENERIC_RX:
					setAxisState(JoystickAxis::RX, normalizeAxis(value, axis));
					break;
				case HID_USAGE_GENERIC_RY:
					setAxisState(JoystickAxis::RY, normalizeAxis(value, axis));
					break;
				case HID_USAGE_GENERIC_RZ:
					setAxisState(JoystickAxis::RZ, normalizeAxis(value, axis));
					break;
				}
			}
		}

		// Reset the states
		auto& states = buttonStates();
		states.assign(states.size(), 0);

		// Output set
		std::bitset<32> button_states;

		USAGE usages[128];
		ULONG nr_usages = 128;
		for (const auto& button_caps : buttonCaps())
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

	GamepadHID::GamepadHID(HANDLE raw_handle)
	: GenericHID(raw_handle)
	, Device(DeviceType::GamePad)
	{

	}

	bool GamepadHID::processInput(PRAWINPUT raw_input)
	{
		// We are not interested in keyboard or mouse data received via raw input
		if (raw_input->header.dwType != RIM_TYPEHID)
			return false;

		PHIDP_PREPARSED_DATA preparsed_data;
		if (HidD_GetPreparsedData(fileHandle(), &preparsed_data) == FALSE)
		{
			return{};
		}

		// Free the allocated data structure at the end of the method
		VCL_SCOPE_EXIT{ HidD_FreePreparsedData(preparsed_data); };

		for (const auto& axis : axes())
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
					std::cout << (LONG)value - axis.logicalCalibratedCenter << ", ";
					break;

				case HID_USAGE_GENERIC_Y:
					std::cout << (LONG)value - axis.logicalCalibratedCenter << ", ";
					break;

				case HID_USAGE_GENERIC_Z:
					std::cout << (LONG)value - axis.logicalCalibratedCenter << ", ";
					break;

				case HID_USAGE_GENERIC_RX:
					std::cout << (LONG)value - axis.logicalCalibratedCenter << ", ";
					break;
				case HID_USAGE_GENERIC_RY:
					std::cout << (LONG)value - axis.logicalCalibratedCenter << ", ";
					break;
				case HID_USAGE_GENERIC_HATSWITCH:
					std::cout << (LONG)value << ", ";
					break;
				}
			}
		}
		std::cout << "\t";

		// Reset the states
		auto& states = buttonStates();
		states.assign(states.size(), 0);

		USAGE usages[128];
		ULONG nr_usages = 128;
		for (const auto& button_caps : buttonCaps())
		{
			if (HidP_GetUsages(
				HidP_Input, button_caps.UsagePage, 0,
				usages, &nr_usages, preparsed_data,
				(PCHAR)raw_input->data.hid.bRawData, raw_input->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS)
			{
				for (ULONG i = 0; i < nr_usages; i++)
				{
					states[usages[i] - button_caps.Range.UsageMin] = TRUE;
				}
			}
		}

		for (const auto& state : states)
		{
			std::cout << state << ", ";
		}

		std::cout << std::endl;

		return true;
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
					switch (dev_info.hid.usUsage)
					{
					case 0x04:
						_devices.emplace_back(std::make_unique<JoystickHID>(desc.hDevice));
						break;

					case 0x05:
						_devices.emplace_back(std::make_unique<GamepadHID>(desc.hDevice));
						break;

					default:
						_devices.emplace_back(std::make_unique<GenericHID>(desc.hDevice));
					}
					// Store a link to the created device for external use
					_deviceLinks.emplace_back(static_cast<Device*>(_devices.back().get()));
				}
			}
		}
	}

	gsl::span<Device const* const> DeviceManager::devices() const
	{
		Device const* const* ptr = _deviceLinks.data();
		return gsl::make_span<Device const* const>(ptr, _deviceLinks.size());
	}

	void DeviceManager::registerDevices(Flags<DeviceType> device_types, HWND hWnd)
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
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x02, RIDEV_INPUTSINK | RIDEV_NOLEGACY, hWnd });
					break;
				case DeviceType::Keyboard:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x06, RIDEV_INPUTSINK | RIDEV_NOLEGACY, hWnd });
					break;
				case DeviceType::Joystick:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x04, RIDEV_INPUTSINK, hWnd });
					break;
				case DeviceType::GamePad:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x05, RIDEV_INPUTSINK, hWnd });
					break;
				case DeviceType::MultiAxisController:
					input_requests.emplace_back(RAWINPUTDEVICE{ 0x01, 0x08, RIDEV_INPUTSINK, hWnd });
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
			return 0;

		// Read the input data
		GetRawInputData(raw_input_handle, RID_INPUT, raw_input, &buffer_size, sizeof(RAWINPUTHEADER));

		// Pass the input data to the correct device
		auto dev_it = std::find_if(_devices.begin(), _devices.end(), [&raw_input](const auto& device)
		{
			return device->rawHandle() == raw_input->header.hDevice;
		});

		bool processed = false;
		if (dev_it != _devices.end())
		{
			processed = (*dev_it)->processInput(raw_input);
		}

		// Clean the buffer
		::DefRawInputProc(&raw_input, 1, sizeof(RAWINPUTHEADER));
		HeapFree(heap_handle, 0, raw_input);

		return processed;
	}
}}}
