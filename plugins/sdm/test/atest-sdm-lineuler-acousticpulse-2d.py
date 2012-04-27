import sys
sys.path.append('/Users/willem/workspace/coolfluid3/dev/builds/clang/release/dso/')

import coolfluid
import math

# The cf root component
root = coolfluid.Core.root()
env =  coolfluid.Core.environment()

### Logging configuration
env.options().set('assertion_backtrace', True)
env.options().set('exception_backtrace', True)
env.options().set('regist_signal_handlers', True)
env.options().set('exception_log_level', 10)
env.options().set('log_level', 3)
env.options().set('exception_outputs', True)

############################
# Create simulation
############################
model   = root.create_component('accousticpulse_2d','cf3.solver.Model');
solver  = model.create_solver('cf3.sdm.SDSolver')
physics = model.create_physics('cf3.physics.LinEuler.LinEuler2D')
domain  = model.create_domain()

### Load the mesh
mesh = domain.load_mesh(file = coolfluid.URI('../../../resources/circle-quad-p1-32.msh'), name = 'circle');

gmsh_writer = model.create_component('load_writer','cf3.mesh.gmsh.Writer')
gmsh_writer.options().set('mesh',mesh)
gmsh_writer.options().set('file',coolfluid.URI('file:load.msh'))
gmsh_writer.execute()

### Configure solver
solver.options().set('mesh',mesh)
solver.options().set('solution_vars','cf3.physics.LinEuler.Cons2D')
solver.options().set('solution_order',4)
solver.options().set('iterative_solver','cf3.sdm.RungeKuttaLowStorage2')

### Configure timestepping
solver.access_component('Time').options().set('time_step',1.);
solver.access_component('Time').options().set('end_time',0.3);
solver.access_component('TimeStepping').options().set('cfl','0.2');
solver.access_component('TimeStepping/IterativeSolver').options().set('nb_stages',3)

### Prepare the mesh for Spectral Difference (build faces and fields etc...)
solver.get_child('PrepareMesh').execute()

### Set the initial condition
gamma = 1.4
rho0 = 1.
p0 = 1.
c2 = gamma*p0/rho0
initial_condition = solver.get_child('InitialConditions').create_initial_condition( name = 'shocktube')
functions = [
 '0.001*exp( -( (x)^2 + (y)^2 )/(0.05)^2 )',
 '0',
 '0',
 str(c2)+' * 0.001*exp( -( (x)^2 + (y)^2 )/(0.05)^2 )'
]
initial_condition.options().set('functions',functions)
solver.get_child('InitialConditions').execute();

### Create convection term
convection = solver.get_child('DomainDiscretization').create_term(name = 'convection', type = 'cf3.sdm.lineuler.Convection2D')
convection.options().set('gamma',gamma)
convection.options().set('rho0',1.)
convection.options().set('U0',[0.,0.])
convection.options().set('p0',1.)

BCs = solver.access_component('TimeStepping/IterativeSolver/PreUpdate').create_component('BoundaryConditions','cf3.sdm.BoundaryConditions')
BCs.options().set('solver',solver)
BCs.options().set('mesh',mesh)
BCs.options().set('physical_model',physics)
non_refl_bc = BCs.create_boundary_condition(name='non_refl_bc',type='cf3.sdm.lineuler.BCSubsonicOutlet2D',regions=[mesh.access_component('topology/boundary').uri()]);
non_refl_bc.get_child('non_reflective_convection').options().set('gamma',gamma)
non_refl_bc.get_child('non_reflective_convection').options().set('rho0',1.)
non_refl_bc.get_child('non_reflective_convection').options().set('U0',[0.,0.])
non_refl_bc.get_child('non_reflective_convection').options().set('p0',1.)

#######################################
# SIMULATE
#######################################
model.simulate()

########################
# OUTPUT
########################

fields = [
mesh.access_component('solution_space/solution').uri(),
mesh.access_component('solution_space/wave_speed').uri(),
mesh.access_component('solution_space/residual').uri()
]

# tecplot
#########
tec_writer = model.get_child('tools').create_component('writer','cf3.mesh.tecplot.Writer')
tec_writer.options().set('mesh',mesh)
tec_writer.options().set('fields',fields)
tec_writer.options().set('cell_centred',False)
tec_writer.options().set('file',coolfluid.URI('file:sdm_output.plt'))
tec_writer.execute()

# gmsh
######
gmsh_writer = model.create_component('writer','cf3.mesh.gmsh.Writer')
gmsh_writer.options().set('mesh',mesh)
gmsh_writer.options().set('fields',fields)
gmsh_writer.options().set('file',coolfluid.URI('file:sdm_output.msh'))
gmsh_writer.execute()