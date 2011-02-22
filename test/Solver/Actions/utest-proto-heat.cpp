// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "Test module for heat-conduction related proto operations"

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "Solver/Actions/Proto/CProtoElementsAction.hpp"
#include "Solver/Actions/Proto/CProtoNodesAction.hpp"
#include "Solver/Actions/Proto/ElementLooper.hpp"
#include "Solver/Actions/Proto/Functions.hpp"
#include "Solver/Actions/Proto/NodeLooper.hpp"
#include "Solver/Actions/Proto/Terminals.hpp"

#include "Common/Core.hpp"
#include "Common/CRoot.hpp"
#include "Common/Log.hpp"

#include "Mesh/CMesh.hpp"
#include "Mesh/CRegion.hpp"
#include "Mesh/CElements.hpp"
#include "Mesh/CField.hpp"
#include "Mesh/CMeshReader.hpp"
#include "Mesh/ElementData.hpp"

#include "Mesh/Integrators/Gauss.hpp"
#include "Mesh/SF/Types.hpp"

#include "Solver/CEigenLSS.hpp"

#include "Tools/MeshGeneration/MeshGeneration.hpp"

using namespace CF;
using namespace CF::Solver;
using namespace CF::Solver::Actions;
using namespace CF::Solver::Actions::Proto;
using namespace CF::Common;
using namespace CF::Math::MathConsts;
using namespace CF::Mesh;

using namespace boost;


typedef std::vector<std::string> StringsT;
typedef std::vector<Uint> SizesT;

/// Check close, for testing purposes
inline void 
check_close(const Real a, const Real b, const Real threshold)
{
  BOOST_CHECK_CLOSE(a, b, threshold);
}

static boost::proto::terminal< void(*)(Real, Real, Real) >::type const _check_close = {&check_close};

struct ProtoHeatFixture
{
  ProtoHeatFixture() :
    root( Core::instance().root() )
  {
  }
  
  ~ProtoHeatFixture()
  {
    root->remove_component("mesh");
    root->remove_component("LSS");
  }
  
  CRoot::Ptr root;
  
};

BOOST_FIXTURE_TEST_SUITE( ProtoHeatSuite, ProtoHeatFixture )

//////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( Laplacian1D )
{
  const Uint nb_segments = 5;
  
  CMesh::Ptr mesh = root->create_component<CMesh>("mesh");
  Tools::MeshGeneration::create_line(*mesh, 5., nb_segments);
  
  // Linear system
  CEigenLSS& lss = *root->create_component<CEigenLSS>("LSS");
  
  // Create output field
  lss.resize(mesh->create_scalar_field("Temperature", "T", CField2::Basis::POINT_BASED).data().size());
  
  MeshTerm<0, Field<Real> > temperature("Temperature", "T");
  
  for_each_element< boost::mpl::vector<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    _cout << "elem result:\n" << integral<1>(laplacian(temperature) * jacobian_determinant) << "\n"
  );
  
  for_each_element< boost::mpl::vector<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    system_matrix(lss, temperature) +=integral<1>(transpose( gradient(temperature) ) * gradient(temperature) * jacobian_determinant)
  );
  
  lss.print_matrix();
}

BOOST_AUTO_TEST_CASE( Heat1D )
{
  Real length     =     5.;
  Real temp_start =   100.;
  Real temp_stop  =   500.;

  const Uint nb_segments = 20;
  
  CMesh::Ptr mesh = root->create_component<CMesh>("mesh");
  Tools::MeshGeneration::create_line(*mesh, length, nb_segments);
  
  // Linear system
  CEigenLSS& lss = *root->create_component<CEigenLSS>("LSS");
  
  // Create output field
  lss.resize(mesh->create_scalar_field("Temperature", "T", CField2::Basis::POINT_BASED).data().size());
  
  MeshTerm<0, Field<Real> > temperature("Temperature", "T");
  
  for_each_element< boost::mpl::vector<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    system_matrix(lss, temperature) += integral<1>( laplacian(temperature) * jacobian_determinant )
  );
  
  // Left boundary at temp_start
  for_each_node
  (
    find_component_recursively_with_name<CRegion>(*mesh, "xneg"),
    dirichlet(lss, temperature) = temp_start
  );

  // Right boundary at temp_stop
  for_each_node
  (
    find_component_recursively_with_name<CRegion>(*mesh, "xpos"),
    dirichlet(lss, temperature) = temp_stop
  );

  // Solve the system!
  lss.solve();
  increment_solution(lss.solution(), StringsT(1, "Temperature"), StringsT(1, "T"), SizesT(1, 1), *mesh);
  
  // Check solution
  for_each_node
  (
    mesh->topology(),
    _check_close(temperature, temp_start + (coordinates(0,0) / length) * (temp_stop - temp_start), 1e-6)
  );
}



// Heat conduction with Neumann BC
BOOST_AUTO_TEST_CASE( Heat1DNeumannBC )
{
  const Real length     =     5.;
  const Real temp_start =   100.;
  const Real temp_stop  =   500.;
  const Real k = 1.;
  const Real q = k * (temp_stop - temp_start) / length;

  const Uint nb_segments = 5;

  CMesh::Ptr mesh = root->create_component<CMesh>("mesh");
  Tools::MeshGeneration::create_line(*mesh, length, nb_segments);
  
  // Linear system
  CEigenLSS& lss = *root->create_component<CEigenLSS>("LSS");
  
  // Create output field
  lss.resize(mesh->create_scalar_field("Temperature", "T", CField2::Basis::POINT_BASED).data().size());
  
  MeshTerm<0, Field<Real> > temperature("Temperature", "T");
  
  for_each_element< boost::mpl::vector<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    system_matrix(lss, temperature) += k * integral<1>( laplacian(temperature) * jacobian_determinant )
  );
  
  // Right boundary at constant heat flux
  for_each_node
  (
    find_component_recursively_with_name<CRegion>(*mesh, "xpos"),
    neumann(lss, temperature) = q
  );
  
  // Left boundary at temp_start
  for_each_node
  (
    find_component_recursively_with_name<CRegion>(*mesh, "xneg"),
    dirichlet(lss, temperature) = temp_start
  );
  
  // Solve the system!
  lss.solve();
  increment_solution(lss.solution(), StringsT(1, "Temperature"), StringsT(1, "T"), SizesT(1, 1), *mesh);
  
  // Check solution
  for_each_node
  (
    mesh->topology(),
    _check_close(temp_start + q * coordinates(0,0), temperature, 1e-6)
  );
}



BOOST_AUTO_TEST_CASE( Heat1DComponent )
{
  // Parameters
  Real length            = 5.;
  const Uint nb_segments = 5 ;
  
  BOOST_CHECK(true);

  CMesh::Ptr mesh = root->create_component<CMesh>("mesh");
  Tools::MeshGeneration::create_line(*mesh, length, nb_segments);
  
  // Linear system
  CEigenLSS& lss = *root->create_component<CEigenLSS>("LSS");
  
  BOOST_CHECK(true);
  
  // Variable holding the field
  MeshTerm<0, Field<Real> > temperature("Temperature", "T");
  
  BOOST_CHECK(true);

  // Create a CAction executing the expression
  CFieldAction::Ptr heat1d_action = build_elements_action( "Heat1D", *root, system_matrix(lss, temperature) += integral<1>(laplacian(temperature) * jacobian_determinant) );
  
  BOOST_CHECK(true);

  // Configure the CAction
  heat1d_action->configure_property("Region", mesh->topology().full_path());
  heat1d_action->create_fields();
  lss.resize( heat1d_action->nb_dofs() );
  
  // Run the expression
  heat1d_action->execute();
  
  BOOST_CHECK(true);

  // Left boundary condition
  MeshTerm<1, ConfigurableConstant<Real> > xneg_temp("XnegTemp", "Temperature at the start of the domain");
  CAction::Ptr xneg_action = build_nodes_action("xneg", *root, dirichlet(lss, temperature) = xneg_temp );  
  xneg_action->configure_property("Region", find_component_recursively_with_name<CRegion>(mesh->topology(), "xneg").full_path());
  xneg_action->configure_property("XnegTemp", 10.);
  xneg_action->execute();
  
  BOOST_CHECK(true);

  // Right boundary condition
  MeshTerm<2, ConfigurableConstant<Real> > xpos_temp("XposTemp", "Temperature at the end of the domain");
  CAction::Ptr xpos_action = build_nodes_action("xpos", *root, dirichlet(lss, temperature) = xpos_temp );  
  xpos_action->configure_property("Region", find_component_recursively_with_name<CRegion>(mesh->topology(), "xpos").full_path());
  xpos_action->configure_property("XposTemp", 35.);
  xpos_action->execute();
  
  // Solve system
  lss.solve();
  increment_solution(lss.solution(), heat1d_action->field_names(), heat1d_action->variable_names(), heat1d_action->variable_sizes(), *mesh);
  
  // Print solution field
  for_each_node
  (
    mesh->topology(),
    _cout << "T(" << coordinates(0,0) << ") = " << temperature << "\n"
  );
}

/// 1D heat conduction with a volume heat source
BOOST_AUTO_TEST_CASE( Heat1DVolumeTerm )
{
  Real length              = 5.;
  Real ambient_temp        = 293.;
  const Uint nb_segments   = 20;
  const Real k             = 100.; // thermal conductivity
  const Real q             = 100.; // Heat production per volume

  CMesh::Ptr mesh = root->create_component<CMesh>("mesh");
  Tools::MeshGeneration::create_line(*mesh, length, nb_segments);
  
  // Linear system
  CEigenLSS& lss = *root->create_component<CEigenLSS>("LSS");
  
  // Create output field
  lss.resize(mesh->create_scalar_field("Temperature", "T", CField2::Basis::POINT_BASED).data().size());
  
  // Variable holding the field
  MeshTerm<0, Field<Real> > temperature("Temperature", "T");
  
  for_each_element< boost::mpl::vector<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    system_matrix(lss, temperature) += integral<1>( k * laplacian(temperature) * jacobian_determinant )
  );
  
  MeshTerm<1, ConstField<Real> > heat("heat", q);
  
  for_each_element< boost::mpl::vector<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    system_rhs(lss, temperature) += integral<1>( jacobian_determinant * sf_outer_product(temperature) ) * heat
  );
  
  // Left boundary at ambient temperature
  for_each_node
  (
    find_component_recursively_with_name<CRegion>(mesh->topology(), "xneg"),
    dirichlet(lss, temperature) = ambient_temp
  );
  
  // Right boundary at ambient temperature
  for_each_node
  (
    find_component_recursively_with_name<CRegion>(mesh->topology(), "xpos"),
    dirichlet(lss, temperature) = ambient_temp
  );
  
  // Solve the system!
  lss.solve();
  increment_solution(lss.solution(), StringsT(1, "Temperature"), StringsT(1, "T"), SizesT(1, 1), *mesh);
  
  // Check solution
  for(int i = 0; i != lss.solution().rows(); ++i)
  {
    Real x = i * length / static_cast<Real>(nb_segments);
    BOOST_CHECK_CLOSE
    (
      lss.solution()[i],
      -q/(2.*k)*x*x + q*length/(2.*k)*x + ambient_temp, // analytical solution, see "A Heat transfer textbook, section 2.2
      1e-6
    );
  }
  CFinfo << CFendl;
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////
