#include "ReducedHamiltonian44.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>
#include <utility>
namespace panorama::reduced44 {
namespace {
constexpr double pi=3.14159265358979323846;
bool ok(double x,double a,double b){return std::isfinite(x)&&x>=a&&x<=b;}
bool chol(std::vector<double>&mat,int n){
 // input dense symmetric SPD; output packed-in-place lower triangle
 for(int i=0;i<n;i++)for(int j=0;j<=i;j++){
  double v=mat[size_t(i*n+j)];
  for(int k=0;k<j;k++)v-=mat[size_t(i*n+k)]*mat[size_t(j*n+k)];
  if(i==j){if(!(v>0)||!std::isfinite(v))return false;mat[size_t(i*n+j)]=std::sqrt(v);}
  else mat[size_t(i*n+j)]=v/mat[size_t(j*n+j)];
 }
 return true;
}
void triangular(const std::vector<double>&L,int n,const double*rhs,double*out) noexcept{
 for(int i=0;i<n;i++){
  double x=rhs[i];for(int j=0;j<i;j++)x-=L[size_t(i*n+j)]*out[j];
  out[i]=x/L[size_t(i*n+i)];
 }
 for(int i=n-1;i>=0;i--){
  double x=out[i];for(int j=i+1;j<n;j++)x-=L[size_t(j*n+i)]*out[j];
  out[i]=x/L[size_t(i*n+i)];
 }
}
}
Recipe example(){Recipe r;r.source=sparse43::example();r.modesPerRegion=6;r.multirate=true;r.rateFactor.assign(r.source.regions.size(),1);if(r.rateFactor.size()>2)r.rateFactor[2]=4;return r;}
bool valid(const Recipe&r,std::string*reason){
 auto fail=[&](const char*m){if(reason)*reason=m;return false;};
 if(!sparse43::valid(r.source,reason))return false;
 if(r.modesPerRegion<0||r.modesPerRegion>32||r.maxIterations<8||r.maxIterations>1000||!ok(r.relativeTolerance,1e-13,1e-3))return fail("invalid reduction and solver configuration");
 if(!r.rateFactor.empty()&&r.rateFactor.size()!=r.source.regions.size())return fail("multirate region count mismatch");
 for(int v:r.rateFactor)if(v!=1&&v!=2&&v!=4)return fail("subcycling factor must be 1, 2 or 4");
 if(reason)reason->clear();return true;
}
std::string serialize(const Recipe&r){
 std::ostringstream o;o<<std::setprecision(17)<<"PANORAMIZE-REDUCED44-v1\n";
 o<<"OPTIONS "<<r.modesPerRegion<<' '<<r.multirate<<' '<<r.maxIterations<<' '<<r.relativeTolerance<<' '<<r.rateFactor.size();
 for(int f:r.rateFactor)o<<' '<<f;
 o<<"\nSPARSE43_BEGIN\n"<<sparse43::serialize(r.source)<<"SPARSE43_END\n";
 return o.str();
}
bool deserialize(const std::string&txt,Recipe&result,std::string*reason){
 auto fail=[&](const char*m){if(reason)*reason=m;return false;};
 if(txt.size()>1950000)return fail("oversized reduced graph");
 std::istringstream in(txt);std::string line;Recipe r;
 if(!std::getline(in,line)||line!="PANORAMIZE-REDUCED44-v1")return fail("invalid reduced graph version");
 if(!std::getline(in,line))return fail("missing reduced options");
 std::istringstream options(line);std::string tag,extra;int mult=0,count=-1;
 if(!(options>>tag>>r.modesPerRegion>>mult>>r.maxIterations>>r.relativeTolerance>>count)||tag!="OPTIONS"||mult<0||mult>1||count<0||count>48)return fail("invalid options");
 r.multirate=bool(mult);
 for(int i=0;i<count;i++){int f=0;if(!(options>>f))return fail("missing rate factor");r.rateFactor.push_back(f);}
 if(options>>extra)return fail("trailing option");
 if(!std::getline(in,line)||line!="SPARSE43_BEGIN")return fail("missing source graph");
 std::string blob;bool closed=false;
 while(std::getline(in,line)){
  if(line=="SPARSE43_END"){closed=true;break;}
  blob+=line+'\n';if(blob.size()>1900000)return fail("oversized source payload");
 }
 if(!closed||std::getline(in,line))return fail("unclosed reduced source graph");
 if(!sparse43::deserialize(blob,r.source,reason)||!valid(r,reason))return false;
 result=std::move(r);if(reason)reason->clear();return true;
}
void Instrument::Matrix::apply(const std::vector<double>&x,std::vector<double>&y)const noexcept{
 for(int i=0;i<n;i++){
  double s=0;for(int k=ptr[size_t(i)];k<ptr[size_t(i+1)];k++)s+=val[size_t(k)]*x[size_t(index[size_t(k)])];
  y[size_t(i)]=s;
 }
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,double sr,std::string*reason){
 auto fail=[&](const char*m)->std::unique_ptr<Instrument>{if(reason)*reason=m;return {};};
 if(!valid(r,reason)||!ok(sr,22050,192000))return fail("invalid recipe or rate");
 auto orig=sparse43::Instrument::prepare(r.source,sr,reason);if(!orig)return {};
 auto full=orig->exportAssembled();
 auto x=std::unique_ptr<Instrument>(new Instrument());x->recipe_=r;x->h_=1./sr;x->fullDofs_=full.n;
 const int n=full.n,regions=int(full.regionOffset.size());
 std::vector<int> regionOf(size_t(n),-1);
 for(int region=0;region<regions;region++)for(int i=full.regionOffset[size_t(region)];i<full.regionOffset[size_t(region)]+full.regionSize[size_t(region)];i++)regionOf[size_t(i)]=region;
 // Exact identity lift at EVERY DOF appearing in a port (no boundary truncation).
 std::vector<char> boundary(size_t(n),false);
 for(const auto&p:full.ports)for(auto&term:p.weights)boundary[size_t(term.first)]=true;
 for(int region=0;region<regions;region++){
  const int off=full.regionOffset[size_t(region)],sz=full.regionSize[size_t(region)];
  Block block;block.begin=int(x->basis_.size());block.factor=r.rateFactor.empty()?1:r.rateFactor[size_t(region)];
  for(int i=off;i<off+sz;i++)if(boundary[size_t(i)]){
   std::vector<double>col(size_t(n),0.);col[size_t(i)]=1.;x->basis_.push_back(std::move(col));x->boundaryDofs_++;
  }
  // Craig-Bampton-style boundary constraint modes and approximately lowest
  // fixed-boundary eigenmodes. Offline dense interior solves; runtime is reduced.
  std::vector<int>interior,interface;
  for(int i=off;i<off+sz;i++)(boundary[size_t(i)]?interface:interior).push_back(i);
  const int nI=int(interior.size());
  if(nI){
   std::vector<double>Kii(size_t(nI*nI),0.),Kib(size_t(nI*interface.size()),0.);
   for(int i=0;i<nI;i++)for(int k=full.ptr[size_t(interior[size_t(i)])];k<full.ptr[size_t(interior[size_t(i)]+1)];k++){
    int global=full.index[size_t(k)];double val=full.K[size_t(k)];
    auto jt=std::lower_bound(interior.begin(),interior.end(),global);
    if(jt!=interior.end()&&*jt==global)Kii[size_t(i*nI+(jt-interior.begin()))]+=val;
    auto ib=std::lower_bound(interface.begin(),interface.end(),global);
    if(ib!=interface.end()&&*ib==global)Kib[size_t(i*interface.size()+(ib-interface.begin()))]+=val;
   }
   // Shift near-rigid free modes to allow stable static solve; this is an
   // approximation of the zero-frequency constraint shapes, not exact FEM statics.
   for(int i=0;i<nI;i++)Kii[size_t(i*nI+i)]+=(2*pi*1.)*(2*pi*1.)*full.mass[size_t(interior[size_t(i)])];
   auto factor=Kii;
   if(!chol(factor,nI))return fail("fixed-interface stiffness not SPD");
   std::vector<double>rhs(size_t(nI),0.),v(size_t(nI),0.);
   for(size_t bIndex=0;bIndex<interface.size();bIndex++){
    for(int i=0;i<nI;i++)rhs[size_t(i)]=-Kib[size_t(i*interface.size()+bIndex)];
    triangular(factor,nI,rhs.data(),v.data());
    auto&col=x->basis_[size_t(block.begin+int(bIndex))];
    for(int i=0;i<nI;i++)col[size_t(interior[size_t(i)])]=v[size_t(i)];
   }
   std::vector<std::vector<double>>previous;
   for(int k=0;k<r.modesPerRegion;k++){
    std::vector<double>mode(size_t(nI),0.);
    for(int i=0;i<nI;i++)mode[size_t(i)]=std::sin(pi*(k+1.)*(i+1.)/(nI+1.));
    auto orthogonal=[&](std::vector<double>&vec){
     for(const auto&prev:previous){
      double proj=0;for(int i=0;i<nI;i++)proj+=full.mass[size_t(interior[size_t(i)])]*vec[size_t(i)]*prev[size_t(i)];
      for(int i=0;i<nI;i++)vec[size_t(i)]-=proj*prev[size_t(i)];
     }
     double norm=0;for(int i=0;i<nI;i++)norm+=full.mass[size_t(interior[size_t(i)])]*vec[size_t(i)]*vec[size_t(i)];
     if(!(norm>1e-19))return false;
     const double scale=1./std::sqrt(norm);for(double&z:vec)z*=scale;
     return true;
    };
    if(!orthogonal(mode))continue;
    // Fixed-interface inverse iteration targets the lowest-frequency modes.
    for(int iteration=0;iteration<8;iteration++){
     for(int i=0;i<nI;i++)rhs[size_t(i)]=full.mass[size_t(interior[size_t(i)])]*mode[size_t(i)];
     triangular(factor,nI,rhs.data(),v.data());
     if(!orthogonal(v))break;
     mode.swap(v);
    }
    if(!orthogonal(mode))continue;
    previous.push_back(mode);
    std::vector<double>col(size_t(n),0.);
    for(int i=0;i<nI;i++)col[size_t(interior[size_t(i)])]=mode[size_t(i)];
    x->basis_.push_back(std::move(col));
   }
  }
  block.count=int(x->basis_.size())-block.begin;
  if(block.count==0)return fail("region has neither interface nor interior basis");
  x->blocks_.push_back(std::move(block));
 }
 const int m=int(x->basis_.size());if(m>1000)return fail("too many reduced coordinates");
 x->mass_.assign(size_t(m),0.);
 x->Mdense_.assign(size_t(m*m),0.);
 for(int i=0;i<m;i++)for(int j=0;j<m;j++){
  double val=0;for(int k=0;k<n;k++)val+=full.mass[size_t(k)]*x->basis_[size_t(i)][size_t(k)]*x->basis_[size_t(j)][size_t(k)];
  x->Mdense_[size_t(i*m+j)]=val;
 }
 for(int i=0;i<m;i++)x->mass_[size_t(i)]=x->Mdense_[size_t(i*m+i)];
 for(double mass:x->mass_)if(!(mass>1e-15)||!std::isfinite(mass))return fail("degenerate reduced mass");
 // Galerkin congruence of positive semidefinite K and R: symmetric and passive.
 const auto project=[&](const std::vector<double>&values){
  std::vector<double>ret(size_t(m*m),0.),tmp(size_t(n),0.);
  for(int j=0;j<m;j++){
   std::fill(tmp.begin(),tmp.end(),0.);
   for(int row=0;row<n;row++)for(int k=full.ptr[size_t(row)];k<full.ptr[size_t(row+1)];k++)
    tmp[size_t(row)]+=values[size_t(k)]*x->basis_[size_t(j)][size_t(full.index[size_t(k)])];
   for(int i=0;i<m;i++)for(int row=0;row<n;row++)ret[size_t(i*m+j)]+=x->basis_[size_t(i)][size_t(row)]*tmp[size_t(row)];
  }
  for(int i=0;i<m;i++)for(int j=0;j<i;j++){
   double s=.5*(ret[size_t(i*m+j)]+ret[size_t(j*m+i)]);
   ret[size_t(i*m+j)]=ret[size_t(j*m+i)]=s;
  }
  return ret;
 };
 x->directK_=project(full.K);x->directR_=project(full.R);
 auto reducedPorts=std::vector<std::vector<std::pair<int,double>>>{};
 for(const auto&p:full.ports){
  std::vector<std::pair<int,double>>terms;
  for(int j=0;j<m;j++){
   double weight=0;for(auto&term:p.weights)weight+=term.second*x->basis_[size_t(j)][size_t(term.first)];
   if(std::abs(weight)>1e-15)terms.push_back({j,weight});
  }
  reducedPorts.push_back(std::move(terms));
 }
 x->ports_=std::move(reducedPorts);
 std::vector<double>lk(size_t(m*m),0.),ld(size_t(m*m),0.);
 for(const auto&l:full.links){
  std::vector<double>c(size_t(m),0.);
  for(int j=0;j<m;j++)for(auto&term:l.row)c[size_t(j)]+=term.second*x->basis_[size_t(j)][size_t(term.first)];
  Coupler copy;copy.stiffness=l.k;copy.damping=l.d;
  for(int i=0;i<m;i++)if(std::abs(c[size_t(i)])>1e-18)copy.row.push_back({i,c[size_t(i)]});
  x->couplers_.push_back(std::move(copy));
  for(int i=0;i<m;i++)for(int j=0;j<m;j++){
   const double outer=c[size_t(i)]*c[size_t(j)];lk[size_t(i*m+j)]+=l.k*outer;ld[size_t(i*m+j)]+=l.d*outer;
  }
 }
 auto makeMatrix=[m](const std::vector<double>&dense){
  Matrix mat;mat.n=m;mat.ptr.resize(size_t(m+1));
  for(int i=0;i<m;i++){
   for(int j=0;j<m;j++)if(std::abs(dense[size_t(i*m+j)])>1e-16){mat.index.push_back(j);mat.val.push_back(dense[size_t(i*m+j)]);}
   mat.ptr[size_t(i+1)]=int(mat.index.size());
  }return mat;
 };
 x->K_=makeMatrix(x->directK_);x->R_=makeMatrix(x->directR_);
 auto midpoint=[&](const std::vector<double>&k,const std::vector<double>&d,double h){
  std::vector<double>a(size_t(m*m),0.);
  for(size_t i=0;i<a.size();i++)a[i]=.5*h*k[i]+d[i];
  for(int i=0;i<m;i++)for(int j=0;j<m;j++)a[size_t(i*m+j)]+=2*x->Mdense_[size_t(i*m+j)]/h;
  return a;
 };
 const auto A=midpoint(x->directK_,x->directR_,x->h_);
 x->A_=makeMatrix(A);
 x->diagInverse_.resize(size_t(m));
 for(int i=0;i<m;i++)x->diagInverse_[size_t(i)]=1./A[size_t(i*m+i)];
 x->blockFactor_.reserve(size_t(regions));
 for(auto&b:x->blocks_){
  const int sz=b.count,begin=b.begin;
  std::vector<double>factor(size_t(sz*sz));
  for(int i=0;i<sz;i++)for(int j=0;j<sz;j++)factor[size_t(i*sz+j)]=A[size_t((begin+i)*m+begin+j)];
  if(!chol(factor,sz))return fail("non-SPD block Jacobi factor");
  x->blockFactor_.push_back(std::move(factor));
  b.internalK.resize(size_t(sz*sz));b.internalR.resize(size_t(sz*sz));b.internalM.resize(size_t(sz*sz));
  for(int i=0;i<sz;i++)for(int j=0;j<sz;j++){
   b.internalM[size_t(i*sz+j)]=x->Mdense_[size_t((begin+i)*m+begin+j)];
   b.internalK[size_t(i*sz+j)]=x->directK_[size_t((begin+i)*m+begin+j)]-lk[size_t((begin+i)*m+begin+j)];
   b.internalR[size_t(i*sz+j)]=x->directR_[size_t((begin+i)*m+begin+j)]-ld[size_t((begin+i)*m+begin+j)];
  }
  // Prepare local midpoint for h / rateFactor (fast regions are subcycled).
  const double dt=x->h_/b.factor;
  b.L.resize(size_t(sz*sz));
  for(int i=0;i<sz;i++)for(int j=0;j<sz;j++)b.L[size_t(i*sz+j)]=b.internalR[size_t(i*sz+j)]+.5*dt*b.internalK[size_t(i*sz+j)];
  for(int i=0;i<sz;i++)for(int j=0;j<sz;j++)b.L[size_t(i*sz+j)]+=2*b.internalM[size_t(i*sz+j)]/dt;
  if(!chol(b.L,sz))return fail("non-SPD local subcycle operator");
 }
 x->q_.assign(size_t(m),0.);x->v_=x->q_;x->rhs_=x->q_;x->vmid_=x->q_;
 x->res_=x->q_;x->z_=x->q_;x->dir_=x->q_;x->Ap_=x->q_;
 x->scratchK_=x->q_;x->scratchR_=x->q_;x->force_=x->q_;
 // Precompute affine domain propagators for local 1/2/4 subcycles.
 // Each map exactly represents repeated implicit midpoint under constant boundary force.
 for(auto&b:x->blocks_){
  const int z=b.count,off=b.begin;const double dt=x->h_/b.factor;
  b.transitionQ.assign(size_t(z*2*z),0.);b.transitionV=b.transitionQ;
  b.responseQ.assign(size_t(z*z),0.);b.responseV=b.responseQ;
  std::vector<double>q(size_t(z),0.),v(size_t(z),0.),f(size_t(z),0.),rhs(size_t(z),0.),mid(size_t(z),0.);
  std::vector<double>midResponse(size_t(b.factor*z*3*z),0.);
  for(int column=0;column<3*z;column++){
   std::fill(q.begin(),q.end(),0.);std::fill(v.begin(),v.end(),0.);std::fill(f.begin(),f.end(),0.);
   if(column<z)q[size_t(column)]=1.;
   else if(column<2*z)v[size_t(column-z)]=1.;
   else f[size_t(column-2*z)]=1.;
   for(int step=0;step<b.factor;step++){
    for(int i=0;i<z;i++){
     double momentum=0,kq=0;
     for(int j=0;j<z;j++){
      momentum+=b.internalM[size_t(i*z+j)]*v[size_t(j)];
      kq+=b.internalK[size_t(i*z+j)]*q[size_t(j)];
     }
     rhs[size_t(i)]=2*momentum/dt-kq+f[size_t(i)];
    }
    triangular(b.L,z,rhs.data(),mid.data());
    for(int i=0;i<z;i++)midResponse[size_t((step*z+i)*(3*z)+column)]=mid[size_t(i)];
    for(int i=0;i<z;i++){
     q[size_t(i)]+=dt*mid[size_t(i)];v[size_t(i)]=2*mid[size_t(i)]-v[size_t(i)];
    }
   }
   for(int i=0;i<z;i++){
    if(column<2*z){b.transitionQ[size_t(i*2*z+column)]=q[size_t(i)];b.transitionV[size_t(i*2*z+column)]=v[size_t(i)];}
    else {b.responseQ[size_t(i*z+column-2*z)]=q[size_t(i)];b.responseV[size_t(i*z+column-2*z)]=v[size_t(i)];}
   }
  }
  // Independent positive-semidefinite loss integral for local subcycling:
  // integral v_mid^T R v_mid dt = input^T W input.
  b.dissipKernel.assign(size_t(3*z*3*z),0.);
  for(int step=0;step<b.factor;step++)for(int i=0;i<z;i++)for(int j=0;j<z;j++){
   const double r=b.internalR[size_t(i*z+j)]*dt;
   if(r==0)continue;
   for(int a=0;a<3*z;a++)for(int c=0;c<3*z;c++)
    b.dissipKernel[size_t(a*3*z+c)]+=r*midResponse[size_t((step*z+i)*3*z+a)]*midResponse[size_t((step*z+j)*3*z+c)];
  }
 }
 // Boundary-only Schur complement. F_link = C^T lambda where lambda is
 // the discrete-gradient passive interface effort:
 // lambda_l = -k_l C_l(q0+q1)/2 - (d_l/h) C_l(q1-q0).
 // This retains exact port power accounting even with different local rates.
 const int links=int(x->couplers_.size());
 if(links>128)return fail("too many interfaces for prepared Schur complement");
 auto dotResponse=[&](const Coupler&a,const Coupler&b){
  double acc=0;
  for(const auto&t:a.row)for(const auto&u:b.row){
   const int i=t.first,j=u.first;
   for(const auto&blk:x->blocks_)if(i>=blk.begin&&i<blk.begin+blk.count && j>=blk.begin&&j<blk.begin+blk.count){
    acc+=t.second*u.second*blk.responseQ[size_t((i-blk.begin)*blk.count+j-blk.begin)];break;
   }
  }return acc;
 };
 std::vector<double>matrix(size_t(links*links),0.);
 for(int i=0;i<links;i++)for(int j=0;j<links;j++){
  double gain=.5*x->couplers_[size_t(i)].stiffness+x->couplers_[size_t(i)].damping/x->h_;
  matrix[size_t(i*links+j)]=(i==j?1.:0.)+gain*dotResponse(x->couplers_[size_t(i)],x->couplers_[size_t(j)]);
 }
 // Invert small SPD-equivalent interface operator with off-thread pivoted LU.
 x->schurInverse_.assign(size_t(links*links),0.);
 for(int i=0;i<links;i++)x->schurInverse_[size_t(i*links+i)]=1.;
 for(int i=0;i<links;i++){
  int best=i;double mag=std::abs(matrix[size_t(i*links+i)]);
  for(int j=i+1;j<links;j++)if(std::abs(matrix[size_t(j*links+i)])>mag){best=j;mag=std::abs(matrix[size_t(j*links+i)]);}
  if(!(mag>1e-16)||!std::isfinite(mag))return fail("non-invertible multirate interface Schur complement");
  if(best!=i)for(int col=0;col<links;col++){
   std::swap(matrix[size_t(i*links+col)],matrix[size_t(best*links+col)]);
   std::swap(x->schurInverse_[size_t(i*links+col)],x->schurInverse_[size_t(best*links+col)]);
  }
  const double pivot=matrix[size_t(i*links+i)];
  for(int col=0;col<links;col++){
   matrix[size_t(i*links+col)]/=pivot;x->schurInverse_[size_t(i*links+col)]/=pivot;
  }
  for(int j=0;j<links;j++)if(j!=i){
   const double factor=matrix[size_t(j*links+i)];
   for(int col=0;col<links;col++){
    matrix[size_t(j*links+col)]-=factor*matrix[size_t(i*links+col)];
    x->schurInverse_[size_t(j*links+col)]-=factor*x->schurInverse_[size_t(i*links+col)];
   }
  }
 }
 x->qNext_.assign(size_t(m),0.);x->vNext_=x->qNext_;x->knownQ_=x->qNext_;x->knownV_=x->qNext_;
 x->interfaceForce_=x->qNext_;x->interfaceRhs_.assign(size_t(links),0.);x->interfaceLambda_=x->interfaceRhs_;
 size_t maxBlock=0;for(const auto&b:x->blocks_)maxBlock=std::max(maxBlock,size_t(b.count));
 x->localInput_.assign(3*maxBlock,0.);
 x->initial_=x->energy();
 // Retain only reduced prepared matrices, port weights and region partitions.
 x->basis_.clear();x->basis_.shrink_to_fit();
 x->directK_.clear();x->directR_.clear();x->directK_.shrink_to_fit();x->directR_.shrink_to_fit();
 if(reason)reason->clear();return x;
}
bool Instrument::solve(const Matrix&A,const std::vector<double>&b,std::vector<double>&solution,bool blockPrecondition) noexcept{
 const int n=A.n;
 A.apply(solution,Ap_);
 double norm=0;
 for(int i=0;i<n;i++){
  res_[size_t(i)]=b[size_t(i)]-Ap_[size_t(i)];norm+=b[size_t(i)]*b[size_t(i)];
 }
 const double tol=recipe_.relativeTolerance*std::sqrt(norm)+1e-18;
 auto residual=[&](){double s=0;for(double v:res_)s+=v*v;return std::sqrt(s);};
 if(residual()<=tol){lastIterations_=0;return true;}
 auto precondition=[&](){
  if(blockPrecondition){
   for(size_t k=0;k<blocks_.size();k++){
    const auto&bk=blocks_[k];
    triangular(blockFactor_[k],bk.count,res_.data()+bk.begin,z_.data()+bk.begin);
   }
  }else{
   for(int i=0;i<n;i++){
    double diagonal=0;
    for(int k=A.ptr[size_t(i)];k<A.ptr[size_t(i+1)];k++)if(A.index[size_t(k)]==i){diagonal=A.val[size_t(k)];break;}
    z_[size_t(i)]=diagonal>0?res_[size_t(i)]/diagonal:res_[size_t(i)];
   }
  }
 };
 precondition();double rz=0;
 for(int i=0;i<n;i++){dir_[size_t(i)]=z_[size_t(i)];rz+=res_[size_t(i)]*z_[size_t(i)];}
 for(int count=0;count<recipe_.maxIterations;count++){
  A.apply(dir_,Ap_);double denom=0;for(int i=0;i<n;i++)denom+=dir_[size_t(i)]*Ap_[size_t(i)];
   if(!(denom>0)||!std::isfinite(rz))return false;
  const double alpha=rz/denom;
  for(int i=0;i<n;i++){
   solution[size_t(i)]+=alpha*dir_[size_t(i)];res_[size_t(i)]-=alpha*Ap_[size_t(i)];
  }
  if(residual()<=tol){lastIterations_=count+1;return true;}
  precondition();double next=0;for(int i=0;i<n;i++)next+=res_[size_t(i)]*z_[size_t(i)];
  if(!(next>0)||!std::isfinite(next))return false;
  const double beta=next/rz;
  for(int i=0;i<n;i++)dir_[size_t(i)]=z_[size_t(i)]+beta*dir_[size_t(i)];
  rz=next;
 }
 return false;
}
bool Instrument::advanceFull(double h) noexcept{
 const int n=int(q_.size());K_.apply(q_,scratchK_);
 for(int i=0;i<n;i++){
  double momentum=0;for(int j=0;j<n;j++)momentum+=Mdense_[size_t(i*n+j)]*v_[size_t(j)];
  rhs_[size_t(i)]=2*momentum/h-scratchK_[size_t(i)]+force_[size_t(i)];
 }
 if(!solve(A_,rhs_,vmid_,true))return false;
 R_.apply(vmid_,scratchR_);
 double active=0,loss=0;
 for(int i=0;i<n;i++){
  active+=force_[size_t(i)]*vmid_[size_t(i)];loss+=vmid_[size_t(i)]*scratchR_[size_t(i)];
  q_[size_t(i)]+=h*vmid_[size_t(i)];v_[size_t(i)]=2*vmid_[size_t(i)]-v_[size_t(i)];
 }
 active_+=h*active;dissipated_+=std::max(0.,h*loss);
 return true;
}
bool Instrument::advanceImplicitInterface() noexcept{
 const int n=int(q_.size());
 // Free response under the independently known ACTIVE forcing, with each
 // partition integrated at its own rate (1,2,4) by prepared propagators.
 for(const auto&b:blocks_){
  const int z=b.count,off=b.begin;
  for(int i=0;i<z;i++){
   double q=0,v=0;
   for(int j=0;j<z;j++){
    q+=b.transitionQ[size_t(i*2*z+j)]*q_[size_t(off+j)]
      +b.transitionQ[size_t(i*2*z+z+j)]*v_[size_t(off+j)]
      +b.responseQ[size_t(i*z+j)]*force_[size_t(off+j)];
    v+=b.transitionV[size_t(i*2*z+j)]*q_[size_t(off+j)]
      +b.transitionV[size_t(i*2*z+z+j)]*v_[size_t(off+j)]
      +b.responseV[size_t(i*z+j)]*force_[size_t(off+j)];
   }
   knownQ_[size_t(off+i)]=q;knownV_[size_t(off+i)]=v;
  }
 }
 const int links=int(couplers_.size());
 for(int l=0;l<links;l++){
  const auto&c=couplers_[size_t(l)];double sum=0;
  for(auto&term:c.row){
   const int i=term.first;
   sum+=term.second*(.5*c.stiffness*(q_[size_t(i)]+knownQ_[size_t(i)])
     +c.damping/h_*(knownQ_[size_t(i)]-q_[size_t(i)]));
  }
  interfaceRhs_[size_t(l)]=-sum;
 }
 for(int i=0;i<links;i++){
  double lambda=0;for(int j=0;j<links;j++)lambda+=schurInverse_[size_t(i*links+j)]*interfaceRhs_[size_t(j)];
  interfaceLambda_[size_t(i)]=lambda;
 }
 std::copy(force_.begin(),force_.end(),interfaceForce_.begin());
 for(int l=0;l<links;l++)for(const auto&term:couplers_[size_t(l)].row)
  interfaceForce_[size_t(term.first)]+=term.second*interfaceLambda_[size_t(l)];
 for(const auto&b:blocks_){
  const int z=b.count,off=b.begin;
  for(int i=0;i<z;i++){
   double q=0,v=0;
   for(int j=0;j<z;j++){
    q+=b.transitionQ[size_t(i*2*z+j)]*q_[size_t(off+j)]
      +b.transitionQ[size_t(i*2*z+z+j)]*v_[size_t(off+j)]
      +b.responseQ[size_t(i*z+j)]*interfaceForce_[size_t(off+j)];
    v+=b.transitionV[size_t(i*2*z+j)]*q_[size_t(off+j)]
      +b.transitionV[size_t(i*2*z+z+j)]*v_[size_t(off+j)]
      +b.responseV[size_t(i*z+j)]*interfaceForce_[size_t(off+j)];
   }
   if(!std::isfinite(q)||!std::isfinite(v))return false;
   qNext_[size_t(off+i)]=q;vNext_[size_t(off+i)]=v;
  }
 }
 // Independently integrated local damping over every substep, using a
 // PRECOMPUTED PSD quadratic form from the actual subcycled midpoint flows.
 // We do not invent dissipation from a measured energy difference.
 for(const auto&b:blocks_){
  const int z=b.count,off=b.begin;
  for(int i=0;i<z;i++){
   const double delta=qNext_[size_t(off+i)]-q_[size_t(off+i)];
   active_+=force_[size_t(off+i)]*delta;
   localInput_[size_t(i)]=q_[size_t(off+i)];
   localInput_[size_t(z+i)]=v_[size_t(off+i)];
   localInput_[size_t(2*z+i)]=interfaceForce_[size_t(off+i)];
  }
  double loss=0;
  for(int i=0;i<3*z;i++)for(int j=0;j<3*z;j++)
   loss+=localInput_[size_t(i)]*b.dissipKernel[size_t(i*3*z+j)]*localInput_[size_t(j)];
  dissipated_+=std::max(0.,loss);
 }
 for(const auto&c:couplers_){
  double change=0;for(auto&term:c.row)change+=term.second*(qNext_[size_t(term.first)]-q_[size_t(term.first)]);
  dissipated_+=c.damping*change*change/h_;
 }
 q_.swap(qNext_);v_.swap(vNext_);
 return true;
}
void Instrument::noteOn(int midi,float velocity)noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 lastMidi_=midi;pulse_+=std::clamp(double(velocity),0.,1.)*recipe_.source.impulseForce;
}
void Instrument::noteOff(int midi)noexcept{if(lastMidi_==midi)lastMidi_=-1;}
void Instrument::allNotesOff()noexcept{lastMidi_=-1;pulse_=0;}
float Instrument::processOne()noexcept{
 std::fill(force_.begin(),force_.end(),0.);
 for(auto&t:ports_[size_t(recipe_.source.excitePort)])force_[size_t(t.first)]+=t.second*pulse_;
 bool success=true;
 if(recipe_.multirate){
  success=advanceImplicitInterface();
 }else success=advanceFull(h_);
 if(!success){fails_++;pulse_=0;frames_++;return 0.f;}
 pulse_*=.65;frames_++;
 const double audio=portVelocity(recipe_.source.outputPort)*recipe_.source.outputGain;
 return std::isfinite(audio)?float(std::tanh(audio)):0.f;
}
double Instrument::energy()const noexcept{
 double e=0;
 for(size_t i=0;i<q_.size();i++)for(size_t j=0;j<q_.size();j++)e+=.5*v_[i]*Mdense_[i*q_.size()+j]*v_[j];
 for(int i=0;i<K_.n;i++)for(int j=K_.ptr[size_t(i)];j<K_.ptr[size_t(i+1)];j++)
  e+=.5*q_[size_t(i)]*K_.val[size_t(j)]*q_[size_t(K_.index[size_t(j)])];
 return e;
}
double Instrument::portVelocity(int port)const noexcept{
 if(port<0||port>=int(ports_.size()))return 0;
 double v=0;for(auto&term:ports_[size_t(port)])v+=term.second*v_[size_t(term.first)];return v;
}
double Instrument::regionEnergy(int r)const noexcept{
 if(r<0||r>=int(blocks_.size()))return 0;
 const auto&b=blocks_[size_t(r)];double e=0;
 for(int i=0;i<b.count;i++){
  const int id=b.begin+i;
  for(int j=0;j<b.count;j++)e+=.5*v_[size_t(id)]*b.internalM[size_t(i*b.count+j)]*v_[size_t(b.begin+j)];
  for(int j=0;j<b.count;j++)e+=.5*q_[size_t(id)]*b.internalK[size_t(i*b.count+j)]*q_[size_t(b.begin+j)];
 }return e;
}
Ledger Instrument::ledger()const noexcept{
 Ledger l;l.initial=initial_;l.active=active_;l.dissipated=dissipated_;l.finalEnergy=energy();
 l.splitError=l.finalEnergy-l.initial-l.active+l.dissipated;return l;
}
Stats Instrument::stats()const noexcept{
 Stats s;s.fullDofs=fullDofs_;s.reducedDofs=int(q_.size());s.boundaryDofs=boundaryDofs_;
 s.nnz=int(A_.val.size());s.iterations=lastIterations_;s.regions=int(blocks_.size());
 s.multirate=recipe_.multirate;s.fails=fails_;s.frames=frames_;
 s.residual=ledger().splitError;return s;
}
} // namespace panorama::reduced44
