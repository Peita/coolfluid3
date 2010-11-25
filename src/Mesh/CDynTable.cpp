// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "Common/CBuilder.hpp"
#include "Common/StreamHelpers.hpp"

#include "Mesh/LibMesh.hpp"
#include "Mesh/CDynTable.hpp"

namespace CF {
namespace Mesh {

using namespace Common;

Common::ComponentBuilder < CDynTable<bool>, Component, LibMesh > CDynTable_bool_Builder;

Common::ComponentBuilder < CDynTable<Uint>, Component, LibMesh > CDynTable_Uint_Builder;

Common::ComponentBuilder < CDynTable<int>, Component, LibMesh >  CDynTable_int_Builder;

Common::ComponentBuilder < CDynTable<Real>, Component, LibMesh > CDynTable_Real_Builder;

Common::ComponentBuilder < CDynTable<std::string>, Component, LibMesh > CDynTable_string_Builder;

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const CDynTable<bool>::ConstRow row)
{
  print_vector(os, row);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CDynTable<Uint>::ConstRow row)
{
  print_vector(os, row);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CDynTable<int>::ConstRow row)
{
  print_vector(os, row);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CDynTable<Real>::ConstRow row)
{
  print_vector(os, row);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CDynTable<std::string>::ConstRow row)
{
  print_vector(os, row);
  return os;
}

////////////////////////////////////////////////////////////////////////////////

} // Mesh
} // CF