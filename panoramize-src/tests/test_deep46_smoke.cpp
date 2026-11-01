#include "DeepInverse46.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
int main(){
 const int sr=8000;std::vector<float> x(sr/2);
 for(int i=0;i<int(x.size());i++)x[size_t(i)]=float(.1*std::sin(2*3.14159265*220*i/sr));
 auto a=panorama::deep46::analyze(x,sr,256);
 assert(a.error.empty()&&!a.frames.empty());
 auto family=panorama::gen37::Family::String1D;
 panorama::gen37::WaveRecipe r;r.family=family;
 auto y=panorama::gen37::renderWave(r,22050,2048);
 auto fit=panorama::deep46::fitWave(y,y,22050,family,r.pickup,4);
 assert(fit.error.empty()&&fit.evaluations==4);
 x[0]=std::numeric_limits<float>::quiet_NaN();
 assert(!panorama::deep46::analyze(x,sr).error.empty());
 std::cout<<"DEEP46_SMOKE_PASS\n";
}
