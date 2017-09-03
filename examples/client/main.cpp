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
#include <iostream>

// VCL
#include <vcl/hid/windows/hid.h>
#include <vcl/hid/gamepad.h>
#include <vcl/hid/joystick.h>
#include <vcl/hid/multiaxiscontroller.h>

Vcl::HID::Windows::DeviceManager manager;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	using namespace Vcl::HID;

	switch (message)
	{
	case WM_CREATE:
	{
	}
	break;
	case WM_INPUT:
	{
		if (manager.processInput(hWnd, message, wParam, lParam))
		{
			// Output buffer
			std::wstringstream output;
			output.precision(2);

			// Write current HID state to the console
			HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);

			SHORT curr_line = 2;
			for (auto dev : manager.devices())
			{
				output.str(std::wstring{});
				output.clear();

				SetConsoleCursorPosition(std_out, { 0, curr_line++ });
				switch (dev->type())
				{
				case DeviceType::Joystick:
				{
					output << L"Joystick ";

					auto joystick = dynamic_cast<const Joystick*>(dev);
					for (size_t i = 0; i < joystick->nrAxes(); i++)
					{
						auto state = joystick->axisState(i);
						output << state << L" ";
					}

					for (size_t i = 0; i < joystick->nrButtons(); i++)
					{
						auto pressed = joystick->buttonState(i);
						if (pressed)
						{
							output << L"1 ";
						}
						else
						{
							output << L"0 ";
						}
					}
					output << L"\n";

					break;
				}
				case DeviceType::Gamepad:
				{
					output << L"Gamepad ";

					auto gamepad = dynamic_cast<const Gamepad*>(dev);
					for (size_t i = 0; i < gamepad->nrAxes(); i++)
					{
						auto state = gamepad->axisState(i);
						output << state << L" ";
					}

					output << static_cast<uint32_t>(gamepad->hatState()) << " ";

					for (size_t i = 0; i < gamepad->nrButtons(); i++)
					{
						auto pressed = gamepad->buttonState(i);
						if (pressed)
						{
							output << L"1 ";
						}
						else
						{
							output << L"0 ";
						}
					}
					output << L"\n";

					break;
				}
				case DeviceType::MultiAxisController:
				{
					output << L"MultiAxisController ";

					auto gamepad = dynamic_cast<const MultiAxisController*>(dev);
					for (size_t i = 0; i < gamepad->nrAxes(); i++)
					{
						auto state = gamepad->axisState(i);
						output << state << L" ";
					}

					for (size_t i = 0; i < gamepad->nrButtons(); i++)
					{
						auto pressed = gamepad->buttonState(i);
						if (pressed)
						{
							output << L"1 ";
						}
						else
						{
							output << L"0 ";
						}
					}
					output << L"\n";

					break;
				}
				}

				DWORD written = 0;
				WriteConsoleW(std_out,
					output.str().c_str(), output.str().size(),
					&written, nullptr);
			}
			
			return 0;
		}
		else
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main(char** argv, int argc)
{
	using namespace Vcl::HID::Windows;

	// Create a hidden window (message only window)
	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = L"RawInputTest";
	wc.style = CS_OWNDC;

	if (!RegisterClassW(&wc))
		return 1;

	auto hWnd = CreateWindowExW(0, wc.lpszClassName, L"RawInputTest", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);

	manager.registerDevices(
		Vcl::HID::DeviceType::Joystick | 
		Vcl::HID::DeviceType::Gamepad |
		Vcl::HID::DeviceType::MultiAxisController,
		hWnd);

	// Write the header
	HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetCursorPos(0, 0);

	wchar_t* title= L"HID";
	DWORD written = 0;
	WriteConsoleW(std_out, title, wcslen(title), &written, nullptr);


	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Tear town the window
	CloseWindow(hWnd);
	return 0;
}
