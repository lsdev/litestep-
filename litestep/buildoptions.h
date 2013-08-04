//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// This is a part of the Litestep Shell source code.
//
// Copyright (C) 1997-2013  LiteStep Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#if !defined(BUILDOPTIONS_H)
#define BUILDOPTIONS_H

//
// Build options
//
// Use this file sparingly. Do not edit this file unless you know what you are
// doing. Editing this file could make your LiteStep builds incompatible to the
// LSDev releases.
// The only reason this file exists is to toggle backwards compatibility hacks.
// New options should only be added if there is no other choice. Most of these
// hacks are scheduled to be taken out in the next LiteStep version.
//

// Enables Indie LS-like math evaluation in VarExpansion(Ex).
// 0.25.0 default: ENABLED
#define LS_COMPAT_MATH

// "Unoptimizes" BitmapToRegion to make it work the 0.24.6 way. It enables the
// clrTolerance parameter, which can be used to interpret e.g. FE00FE as Magic
// Pink.
// 0.25.0 default: ENABLED
#define LS_COMPAT_TRANSPTOL

// Adds basic logging functions to lsapi. LSLogFile and LSLogLevel are
// supported.
// 0.25.0 default: ENABLED
#define LS_COMPAT_LOGGING

// Adds support for IncludeFolder Directive (all .rc files in specified folder
// will be included - order not guaranteed).  This was added by the LDE(X) team.
// Thanks Phil!
// 0.25.0 default: ENABLED
#define LS_CUSTOM_INCLUDEFOLDER

// Makes lsapi use picoPNG instead of zlib+libpng
// 0.25.0 default: ENABLED
#define LS_USE_PICOPNG

// Makes LiteStep run Explorer's window procedure when set as shell. 
// 0.25.0 default: ENABLED
//#define LS_USE_EXPLORER_SERVICE

// Makes LCReadNextConfig conform to pre 0.24.7 API
// 0.25.0 default: DISABLED
//#define LS_COMPAT_LCREADNEXTCONFIG

// Uses *LSLoadModule instead of LoadModule
// 0.25.0 default: DISABLED
//#define LS_COMPAT_LSLOADMODULE

#endif // BUILDOPTIONS_H
