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
#include "gamepad.h"

// VCL
#include <vcl/core/contract.h>

namespace Vcl { namespace HID
{
	Gamepad::Gamepad()
	: Device(DeviceType::Gamepad)
	{
		_axes.assign(0);
	}

	uint32_t Gamepad::nrAxes() const
	{
		return _nrAxes;
	}
	void Gamepad::setNrAxes(uint32_t nr_axes)
	{
		_nrAxes = nr_axes;
	}

	uint32_t Gamepad::nrButtons() const
	{
		return _nrButtons;
	}
	void Gamepad::setNrButtons(uint32_t nr_buttons)
	{
		_nrButtons = nr_buttons;
	}

	float Gamepad::axisState(uint32_t axis) const
	{
		return _axes[axis];
	}

	void Gamepad::setAxisState(uint32_t axis, float state)
	{
		_axes[axis] = state;
	}

	bool Gamepad::buttonState(uint32_t idx) const
	{
		VclRequire(idx < std::min(32u, nrButtons()), "Button index is valid.");

		return _buttons[idx];
	}

	void Gamepad::setButtonStates(std::bitset<32>&& states)
	{
		_buttons = states;
	}

	void Gamepad::setHatState(uint32_t state)
	{
		VclRequire(state < 9, "Hat state is valid.");

		_hat = static_cast<GamepadHat>(state);
	}
}}
