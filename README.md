# Moving Mesh methodology from 3D RTE data as presented in Gruenwald et al. 2021

Read the How_to_MovingMesh9.pdf for detailed explanation of each step to perform simulations in ANSYS Fluent software.

-ucd2stl.m is a code to transfer output of 3D RTE after using TomTec ImageArena to STL files.

-The project setup (with ANSYS 2022R1) can be found here: https://doi.org/10.5281/zenodo.8233365

-A tutorial video can be found here: https://youtu.be/C1O20YvkCJs


If there are questions:
Please contact Michael Neidlin - neidlin@ame.rwth-aachen.de

# Idea behind new feature:

Simulations of a ventricle including dynamics of the myocardium and the valves need dynamic remeshing of the cell zone to avoid highly skewed cells. Remeshing in general makes repartitioning necessary to avoid non-continious partitions and an unbalanced workload. Unfortunately, the initial implementation of the mesh motion does not allow repartitioning the computational domain as the distribution of the surface nodes among the used computing nodes needs to stay fixed throughout the simulation.

idea: use global node numbers by storing them initially in a text file and loading these into ram before starting the simulation. When looping through all face zones and all nodes, each computing node obtains the index of the current mesh node by comparing NODE_ID(v) to the global node IDs previously store in the text file. This enables to use both remeshing and dynamic repartitioning.

however: cell zone remeshing is still not allowed as this might change the topology of the surface mesh (define/dynamic-mesh/controls/remeshing-parameter/zone-remeshing no)