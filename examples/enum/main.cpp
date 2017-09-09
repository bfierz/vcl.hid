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
#include <iostream>

// VCL
#include <vcl/hid/windows/hid.h>
#include <vcl/hid/gamepad.h>
#include <vcl/hid/joystick.h>
#include <vcl/hid/multiaxiscontroller.h>

int main(char**, int)
{
	using namespace Vcl::HID::Windows;
	using Vcl::HID::DeviceType;
	using Vcl::HID::Joystick;
	using Vcl::HID::Gamepad;
	using Vcl::HID::MultiAxisController;

	Vcl::HID::Windows::DeviceManager manager;

	const auto devs = manager.devices();
	for (auto dev : devs)
	{
		std::wcout << dev->type() << L", " << dev->vendorName() << L", " << dev->deviceName() << L"\n";
		switch (dev->type())
		{
		case DeviceType::Joystick:

			std::wcout << L"Number of axes: "    << dynamic_cast<const Joystick*>(dev)->nrAxes() << L"\n";
			std::wcout << L"Number of buttons: " << dynamic_cast<const Joystick*>(dev)->nrButtons() << L"\n";
			break;

		case DeviceType::Gamepad:

			std::wcout << L"Number of axes: "    << dynamic_cast<const Gamepad*>(dev)->nrAxes() << L"\n";
			std::wcout << L"Number of buttons: " << dynamic_cast<const Gamepad*>(dev)->nrButtons() << L"\n";
			break;

		case DeviceType::MultiAxisController:

			std::wcout << L"Number of axes: "    << dynamic_cast<const MultiAxisController*>(dev)->nrAxes() << L"\n";
			std::wcout << L"Number of buttons: " << dynamic_cast<const MultiAxisController*>(dev)->nrButtons() << L"\n";
			break;
		}

		std::wcout << std::endl;
	}

	return 0;
}
