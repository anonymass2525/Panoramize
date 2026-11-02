#pragma once
#include <cmath>
namespace panorama::detail {
// v1.1: high-accuracy, fixed-workload approximation of sin(2*pi*cycles).
// Reduce to the first quadrant, then use a degree-17 alternating Taylor series.
// For |x|<=pi/2 the omitted term is <=4.4e-14; no lookup table or allocations.
// Called only for continuously phased partials on the typed spectral bus.
inline double sineCycles(double cycles) noexcept {
  constexpr double halfPi=1.57079632679489661923132169163975144;
  const double unit=cycles-std::floor(cycles);
  double quarter=4.0*unit;
  double sign=1.0;
  if (quarter>=2.0) {quarter-=2.0;sign=-1.0;}
  if (quarter>1.0) quarter=2.0-quarter;
  const double x=halfPi*quarter;
  const double q=x*x;
  // Horner form: sin(x) = x*[1-x²/3!+x^4/5!- ... +x^16/17!]
  const double poly=1.0+q*(-1.0/6.0+q*(1.0/120.0+q*(-1.0/5040.0+q*(1.0/362880.0+q*(-1.0/39916800.0+q*(1.0/6227020800.0+q*(-1.0/1307674368000.0+q*(1.0/355687428096000.0))))))));
  return sign*x*poly;
}
}
