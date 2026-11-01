#pragma once
// 5.0 typed global control catalog; UI/editor and offline automation share one ID space.
// Structural controls require a prepared-graph swap; only the existing host IDs
// (tone, macro, node level) are audio-thread safe. Catalog is not full reflection
// of all historical internals: unsupported controls remain native to each engine.
#include "UniversalInstrument23.h"
#include <functional>
#include <string>
#include <vector>
namespace panorama::controls50 {
using Graph=universal23::Graph;
struct Parameter {
 std::string id,group,name,unit;
 double lo=0,hi=1;
 bool realtime=false;
 int hostIndex=-1;
 std::function<double(const Graph&)> read;
 std::function<void(Graph&,double)> write;
};
std::vector<Parameter> catalog(const Graph&g);
bool change(Graph&g,const std::string&id,double value,std::string*error=nullptr);
bool read(const Graph&g,const std::string&id,double&value);
// Applies a series of property changes atomically (all-or-none, checks runtime preparation).
bool transaction(Graph&g,const std::vector<std::pair<std::string,double>>&edits,
                 double sampleRate,std::string*error=nullptr);
std::string catalogCSV(const Graph&g);
}
