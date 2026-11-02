#pragma once
// 5.4: auditable uncertainty and measurement planning around EDITABLE real DSP candidates.
// Scores are descriptor errors; NO probabilistic confidence or unique physical reconstruction is claimed.
#include "JointInverse50.h"
#include "ExperimentalObservability53.h"
#include <string>
#include <vector>
namespace panorama::decision54 {
struct CandidateView {
 std::string family;
 double trainError=0, holdoutError=0;
 double trainGapToBest=0,holdoutGapToBest=0;
 bool overfitRisk=false;
};
struct Summary {
 std::vector<CandidateView> candidates;
 int bestTrain=-1,bestHoldout=-1;
 double normalizedTrainingGap=0;
 bool rankingDisagrees=false,lowSeparation=true;
 std::string warning;
};
Summary review(const joint50::Result& result);
struct ProbeSuggestion {
 std::string family, measurement;
 int rankBefore=0,rankAfter=0,parameterCount=0;
 bool independentlyDistinguishes=false;
 std::string reason;
};
// Analytical surrogate checks: not sensitivities of complete Panoramize FEM or Native27.
std::vector<ProbeSuggestion> suggestedMeasurements();
std::string candidatesCSV(const Summary&);
std::string measurementsCSV(const std::vector<ProbeSuggestion>&);
std::string reportText(const Summary&);
}
