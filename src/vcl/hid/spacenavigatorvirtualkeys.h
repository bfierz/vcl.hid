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

#define VCL_DEVICE_SPACENAVIGATOR_TRACE_VIRTUAL_KEYS 0

namespace Vcl { namespace HID
{
   enum e3dconnexion_pid : unsigned long
   {
      eSpacePilot = 0xc625,
      eSpaceNavigator = 0xc626,
      eSpaceExplorer = 0xc627,
      eSpaceNavigatorForNotebooks = 0xc628,
      eSpacePilotPRO = 0xc629
   };

   enum e3dmouse_virtual_key 
   {
      V3DK_INVALID=0
      , V3DK_MENU=1, V3DK_FIT
      , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT, V3DK_BOTTOM, V3DK_BACK
      , V3DK_CW, V3DK_CCW
      , V3DK_ISO1, V3DK_ISO2
      , V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6, V3DK_7, V3DK_8, V3DK_9, V3DK_10
      , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
      , V3DK_ROTATE, V3DK_PANZOOM, V3DK_DOMINANT
      , V3DK_PLUS, V3DK_MINUS
   };

#if VCL_DEVICE_SPACENAVIGATOR_TRACE_VIRTUAL_KEYS
   static const char* _3dmouse_virtual_key[] =
   {
      "V3DK_INVALID"
      , "V3DK_MENU", "V3DK_FIT"
      , "V3DK_TOP", "V3DK_LEFT", "V3DK_RIGHT", "V3DK_FRONT", "V3DK_BOTTOM", "V3DK_BACK"
      , "V3DK_CW", "V3DK_CCW"
      , "V3DK_ISO1", "V3DK_ISO2"
      , "V3DK_1", "V3DK_2", "V3DK_3", "V3DK_4", "V3DK_5", "V3DK_6", "V3DK_7", "V3DK_8", "V3DK_9", "V3DK_10"
      , "V3DK_ESC", "V3DK_ALT", "V3DK_SHIFT", "V3DK_CTRL"
      , "V3DK_ROTATE", "V3DK_PANZOOM", "V3DK_DOMINANT"
      , "V3DK_PLUS", "V3DK_MINUS"
   };
#endif /* VCL_DEVICE_SPACENAVIGATOR */

   struct tag_VirtualKeys
   {
      e3dconnexion_pid pid;
      size_t nKeys;
      e3dmouse_virtual_key *vkeys;
   };

   static const e3dmouse_virtual_key SpaceExplorerKeys [] = 
   {
      V3DK_INVALID     // there is no button 0
      , V3DK_1, V3DK_2
      , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT
      , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
      , V3DK_FIT, V3DK_MENU
      , V3DK_PLUS, V3DK_MINUS
      , V3DK_ROTATE
   };

   static const e3dmouse_virtual_key SpacePilotKeys [] = 
   {
      V3DK_INVALID 
      , V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6
      , V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT
      , V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL
      , V3DK_FIT, V3DK_MENU
      , V3DK_PLUS, V3DK_MINUS
      , V3DK_DOMINANT, V3DK_ROTATE
   };

   static const struct tag_VirtualKeys _3dmouseVirtualKeys[]= 
   {
      eSpacePilot
      , sizeof(SpacePilotKeys)/sizeof(SpacePilotKeys[0])
      , const_cast<e3dmouse_virtual_key *>(SpacePilotKeys),
      eSpaceExplorer
      , sizeof(SpaceExplorerKeys)/sizeof(SpaceExplorerKeys[0])
      , const_cast<e3dmouse_virtual_key *>(SpaceExplorerKeys)
   };

   /*-----------------------------------------------------------------------------
   *
   * unsigned short HidToVirtualKey(unsigned short pid, unsigned short hidKeyCode)
   *
   * Args:
   *    pid - USB Product ID (PID) of 3D mouse device 
   *    hidKeyCode - Hid keycode as retrieved from a Raw Input packet
   *
   * Return Value:
   *    Returns the standard 3d mouse virtual key (button identifier) or zero if an error occurs.
   *
   * Description:
   *    Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device
   *    to the standard 3d mouse virtual key definition.
   *
   *---------------------------------------------------------------------------*/
   inline unsigned short HidToVirtualKey(unsigned long pid, unsigned short hidKeyCode)
   {
      unsigned short virtualkey=hidKeyCode;
      for (size_t i=0; i<sizeof(_3dmouseVirtualKeys)/sizeof(_3dmouseVirtualKeys[0]); ++i)
      {
         if (pid == _3dmouseVirtualKeys[i].pid)
         {
            if (hidKeyCode < _3dmouseVirtualKeys[i].nKeys)
               virtualkey = _3dmouseVirtualKeys[i].vkeys[hidKeyCode];
            else
               virtualkey = V3DK_INVALID;
            break;
         }
      }
      // Remaining devices are unchanged
#if VCL_DEVICE_SPACENAVIGATOR_TRACE_VIRTUAL_KEYS
      TRACE("Converted %d to %s for pid 0x%x\n", hidKeyCode, _3dmouse_virtual_key[virtualkey], pid);
#endif /* VCL_DEVICE_SPACENAVIGATOR */
      return virtualkey;
   }
}}
