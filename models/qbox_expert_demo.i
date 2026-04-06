# Qbox Expert template for GDIS
#
# Use this file from Tools > Computation > Qbox... > Expert tab:
#   Include command file (optional): models/qbox_expert_demo.i
#
# Notes:
# - The main GDIS-generated input can still provide cell/species/atom.
# - Keep only the commands you need.

# electronic controls
set scf_tol 1e-8
set nempty 8

# k-point sampling example
kpoint delete 0 0 0
kpoint add 0.0 0.0 0.0 1.0

# add a simple band path (weights 0 for non-SCF path points)
# kpoint add 0.0 0.0 0.0 0.0
# kpoint add 0.5 0.0 0.0 0.0
# kpoint add 0.333333 0.333333 0.0 0.0
# kpoint add 0.0 0.0 0.0 0.0

# if using geometry optimization:
# set atoms_dyn CG
# set dt 20

# extra run segments
run 0 40

# optional analysis/output commands:
# status
# list_atoms
# list_species
# compute_mlwf
# response
# spectrum
# plot -wf 9 HOMO.cube
