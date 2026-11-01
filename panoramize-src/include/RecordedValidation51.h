#pragma once
// v5.1: reproducible assessment of real or simulated WAV *inputs*.
// Never claims to infer hidden geometry or establishes real provenance by itself.
#include "JointInverse50.h"
#include <functional>
#include <string>
#include <vector>
namespace panorama::recorded51 {
struct Audit {
 double rms=0,peak=0,dc=0,clippedFraction=0,initialRms=0,periodicity=0;
 bool silent=false,clipped=false,weakPeriodicity=false;
};
Audit inspect(const joint50::Take&take,int rate);
std::string auditCSV(const std::vector<joint50::Take>&takes,int rate);
// Generic, dimensionless, locally linearized observability diagnostic.
// Takes a model whose output is a set of independently measured quantities.
// Use strictly positive physical parameters in log domain, e.g. T and mu.
struct RankReport {int rank=0,parameters=0,observations=0; bool locallyIdentifiable=false;double minIndependentNorm=0;std::string warning;};
RankReport logSensitivityRank(const std::vector<double>&positiveParameters,
 const std::function<std::vector<double>(const std::vector<double>&)>&observe,double relStep=1e-4,double tolerance=1e-5);
}
