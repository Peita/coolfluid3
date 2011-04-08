// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "Common/RegistLibrary.hpp"
#include "Common/CRoot.hpp"

#include "SFDM/src/Common/LibCommon.hpp"

namespace CF {
namespace SFDM {
namespace Common {

CF::Common::RegistLibrary<LibCommon> LibCommon;

////////////////////////////////////////////////////////////////////////////////

void LibCommon::initiate_impl()
{
}

void LibCommon::terminate_impl()
{
}

////////////////////////////////////////////////////////////////////////////////

} // Common
} // SFDM
} // CF
