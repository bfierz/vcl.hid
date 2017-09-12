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
#include <vcl/hid/spacenavigator.h>

// C++ standard library
#include <algorithm>

namespace Vcl { namespace HID
{
	const unsigned int SpaceNavigator::LogitechVendorID = 0x46d;
	const float SpaceNavigator::AngularVelocity = 8.0e-6f;
	
	std::vector<SpaceNavigatorHandler*> SpaceNavigator::_handlers;

	void SpaceNavigator::registerHandler(SpaceNavigatorHandler* handler)
	{		
		if (std::find(_handlers.begin(), _handlers.end(), handler) == _handlers.end())
			_handlers.push_back(handler);
	}

	void SpaceNavigator::unregisterHandler(SpaceNavigatorHandler* handler)
	{
		auto it = std::find(_handlers.begin(), _handlers.end(), handler);
		if (it != _handlers.end())
			_handlers.erase(it);
	}
}}
