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
#include <array>
#include <bitset>

// VCL
#include <vcl/hid/device.h>

namespace Vcl { namespace HID
{
	class MultiAxisController : public Device
	{
	public:
		MultiAxisController();

		uint32_t nrAxes() const;
		uint32_t nrButtons() const;

		float axisState(uint32_t axis) const;
		bool buttonState(uint32_t idx) const;

	protected:
		void setNrAxes(uint32_t nr_axes);
		void setNrButtons(uint32_t nr_buttons);
		void setAxisState(uint32_t axis, float state);
		void setButtonStates(std::bitset<32>&& states);

	private:
		/// Number of reported axes
		uint32_t _nrAxes{ 0 };

		/// Number of reported buttons
		uint32_t _nrButtons{ 0 };

		/// Axes states
		std::array<float, 8> _axes;

		/// Buttons states
		std::bitset<32> _buttons;
	};
}}
