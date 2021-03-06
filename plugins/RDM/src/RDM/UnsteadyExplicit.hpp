// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef cf3_RDM_UnsteadyExplicit_hpp
#define cf3_RDM_UnsteadyExplicit_hpp

////////////////////////////////////////////////////////////////////////////////

#include "solver/Wizard.hpp"

#include "RDM/LibRDM.hpp"

namespace cf3 {

 namespace solver { class Model; }

namespace RDM {

////////////////////////////////////////////////////////////////////////////////

/// Wizard to setup a scalar advection simulation
/// @author Tiago Quintino
class RDM_API UnsteadyExplicit : public solver::Wizard {

public: // typedefs

  
  

public: // functions

  /// Contructor
  /// @param name of the component
  UnsteadyExplicit ( const std::string& name );

  /// Virtual destructor
  virtual ~UnsteadyExplicit();

  /// Get the class name
  static std::string type_name () { return "UnsteadyExplicit"; }

  // functions specific to the UnsteadyExplicit component

  cf3::solver::Model& create_model( const std::string& model_name,
                                    const std::string& physics_builder );

  /// @name SIGNALS
  //@{

  /// Signal to create a model
  void signal_create_model ( common::SignalArgs& node );

  void signature_create_model( common::SignalArgs& node);

  //@} END SIGNALS

};

////////////////////////////////////////////////////////////////////////////////

} // RDM
} // cf3

////////////////////////////////////////////////////////////////////////////////

#endif // cf3_RDM_UnsteadyExplicit_hpp
