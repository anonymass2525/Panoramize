#include "../src/FastDSPMath.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
int main() {
 constexpr double pi=3.1415926535897932384626433832795;
 double worst=0.0;
 for (int i=-240000;i<=240000;i++) {
   const double cyc=static_cast<double>(i)/32000.0;
   const double actual=panorama::detail::sineCycles(cyc);
   const double exact=std::sin(2.0*pi*cyc);
   assert(std::isfinite(actual));
   worst=std::max(worst,std::abs(actual-exact));
 }
 for (double c : {-1.0,-0.75,-0.5,-0.25,0.0,.25,.5,.75,1.0}) {
   assert(std::abs(panorama::detail::sineCycles(c)-std::sin(2*pi*c))<2e-12);
 }
 std::cout<<"max abs error: "<<worst<<"\n";
 assert(worst < 2e-12);
}
