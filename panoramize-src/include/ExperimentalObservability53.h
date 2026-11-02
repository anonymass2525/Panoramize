#pragma once
// v5.3 - local inverse-problem observability and experiment selection.
// This is NOT an oracle for arbitrary geometry, material, or vocal-tract recovery.
#include <functional>
#include <string>
#include <vector>
namespace panorama::observability53 {
struct Measurement {
 std::string label;
 std::vector<double> sigma; // independent observation standard deviations, positive
 std::function<std::vector<double>(const std::vector<double>&)> observe;
};
struct Diagnostics {
 int parameterCount=0, measurementCount=0, rank=0;
 double minSingular=0, maxSingular=0, condition=0;
 bool locallyIdentifiable=false;
 std::vector<double> singularValues;
 std::string warning;
};
// Sensitivities d y / d log(theta); sigma puts observations in comparable units.
// Only local identifiability near theta. It does not prove global uniqueness.
Diagnostics evaluate(const std::vector<double>& positiveTheta,
 const std::vector<Measurement>& probes,
 double relativeStep=1e-4,double relativeTolerance=1e-5);
struct Plan {int selected=-1;Diagnostics after;std::string rationale;};
Plan selectNext(const std::vector<double>&theta,
 const std::vector<Measurement>&existing,
 const std::vector<Measurement>&candidates);
}
