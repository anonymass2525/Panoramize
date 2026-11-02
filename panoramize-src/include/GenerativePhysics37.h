#pragma once
// Panoramize 3.7 - generic geometry descriptions and bounded 1D/2D wave prototypes,
// plus parametric tetrahedral construction for the coupled 3D six-string FEM36.
// Does not claim an exact physical plate, air column, or wood-material calibration.
#include "GeneralizedFEM36.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace panorama::gen37 {
enum class Family { String1D, Membrane2D, Plate2D, Tube1D, TetraResonator3D };
struct Shape {
 // Hexahedral lattice cells, subdivided into six tetrahedra each; bounded by FEM36.
 int xCells=2,yCells=1,zCells=1;
 double length=.2,width=.12,thickness=.08;
 double camber=0., taper=0.; // nontrivial surface warp; preserve admissible volumes
};
bool validShape(const Shape&) noexcept;
bool buildTetraMesh(const Shape&,fem36::Mesh&,std::string*error=nullptr);
std::string encodeShape(const Shape&);
bool decodeShape(const std::string&,Shape&);
struct WaveRecipe {
 Family family=Family::String1D;
 int nx=24,ny=16;
 double fundamentalHz=196.,damping=1.5,dispersion=.01;
 double exciter=.27,pickup=.64,amplitude=.2;
};
// Educational distributed parameter approximations: taut 1D string, 2D membrane,
// bending-regularized plate and 1D acoustic pressure/flow tube.
// 3D resonators use renderTetra() with fem36's actual FEM kernel.
std::string encodeWaveRecipe(const WaveRecipe&);
bool decodeWaveRecipe(const std::string&,WaveRecipe&);
std::vector<float> renderWave(const WaveRecipe&,int sampleRate,std::size_t frames);
std::vector<float> renderTetra(const fem36::Model&,int sampleRate,int midi,float velocity,std::size_t frames);
}
