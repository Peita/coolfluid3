// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef cf3_sdm_PrepareMesh_hpp
#define cf3_sdm_PrepareMesh_hpp

#include "solver/ActionDirector.hpp"

#include "sdm/LibSDM.hpp"

namespace cf3 {
namespace sdm {

class CellTerm;

/////////////////////////////////////////////////////////////////////////////////////

class sdm_API PrepareMesh : public cf3::solver::ActionDirector {

public: // typedefs

  
  

public: // functions
  /// Contructor
  /// @param name of the component
  PrepareMesh ( const std::string& name );

  /// Virtual destructor
  virtual ~PrepareMesh() {}

  /// Get the class name
  static std::string type_name () { return "PrepareMesh"; }

  /// execute the action
  virtual void execute ();

};

/////////////////////////////////////////////////////////////////////////////////////


} // sdm
} // cf3

#endif // cf3_sdm_PrepareMesh_hpp
