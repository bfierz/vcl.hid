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

// VCL
#include <vcl/core/flags.h>

namespace Vcl { namespace HID
{
	VCL_DECLARE_FLAGS(DeviceType,
		Undefined,          // Usage: 0x0
		Pointer,            // Usage: 0x1
		Mouse,              // Usage: 0x2
		Joystick,           // Usage: 0x4
		Gamepad,            // Usage: 0x5
		Keyboard,           // Usage: 0x6
		Keypad,             // Usage: 0x7
		MultiAxisController // Usage: 0x8
	);

	/*!
	 *
	 */
	class Device
	{
	public:
		Device(DeviceType::Enum type);
		virtual ~Device() = default;

		DeviceType::Enum type() const { return _type; }

		const std::wstring& vendorName() const { return _vendor; }
		const std::wstring& deviceName() const { return _name; }

	protected:
		void setVendorName(const std::wstring& name) { _vendor = name; }
		void setDeviceName(const std::wstring& name) { _name = name; }

	private:
		/// Device type
		DeviceType::Enum _type;

		/// Vendor name
		std::wstring _vendor;

		/// Vendor defined device name
		std::wstring _name;
	};
}}
