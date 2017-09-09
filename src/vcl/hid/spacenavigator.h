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

// C++ standard libary
#include <array>
#include <limits>
#include <map>
#include <vector>

// VCL
#include <vcl/hid/multiaxiscontroller.h>
#include <vcl/hid/spacenavigatorhandler.h>

namespace Vcl { namespace HID
{
	/*!
	 *	Managing class for the 3Dconnexion SpaceNavigator.
	 *	\note The code is an adapted version of the SDK code stripped free of the 
	 *	ATL/WTL, MFC code; and adapted to this library
	 */
	class SpaceNavigator : public MultiAxisController
	{
	public:
		enum class Speed
		{
			Low = 0,
			Mid,
			High
		};

	public: // Constants
		//! 3Dconnexion SpaceNavigator uses the Logitech vendor ID
		static const unsigned int LogitechVendorID;
		
		//! Object angular velocity per mouse tick 0.008 milliradians per second per count
		static const float AngularVelocity;

	public: // Handler management
		void registerHandler(SpaceNavigatorHandler* handler);
		void unregisterHandler(SpaceNavigatorHandler* handler);

	public: // Configuration
		//! Set the speed configuration
		void setSpeed(Speed speed) { _speed = speed; }

	protected: // Handlers
		std::vector<SpaceNavigatorHandler*> _handlers;

	protected: // Configuration
		//! Only process values when application is in foreground
		bool _only_foreground{ false };

		//! Speed of the mouse motion
		Speed _speed{ Speed::Mid };

		//! Enable pan/zoom
		bool _isPanZoom{ true };

		//! Enable rotation
		bool _isRotate{ true };
	};
}}
