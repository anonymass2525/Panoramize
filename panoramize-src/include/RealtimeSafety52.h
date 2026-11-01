#pragma once
// Panoramize 5.2: an explicitly bounded, callback-only DSP coefficient smoother.
// The producer performs validation and publishes targets atomically; the audio
// callback only reads atomics and applies fixed-cost exponential interpolation.
#include <atomic>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
namespace panorama::safe52 {
struct Range {double min=0.,max=1.,initial=.5;};
class CoefficientBank {
public:
 explicit CoefficientBank(const std::vector<Range>&ranges,double sampleRate,double milliseconds=5.);
 bool publish(std::size_t i,double value) noexcept;
 bool publishBatch(const std::vector<double>&all) noexcept; // all-or-none validation, control thread only
 double tick(std::size_t i) noexcept; // audio thread only; once/sample/parameter
 double current(std::size_t i) const noexcept {return i<values_.size()?values_[i]:0.;} // audio thread only
 std::size_t size()const noexcept{return ranges_.size();}
private:
 std::vector<Range> ranges_;
 std::unique_ptr<std::atomic<double>[]> targets_;
 std::vector<double> values_;
 double decay_=0.;
};
}
