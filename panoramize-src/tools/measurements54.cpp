#include "InverseDecision54.h"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: panoramize_measurements54 OUT_DIRECTORY\n";return 2;}
 const std::filesystem::path dir(argv[1]);std::filesystem::create_directories(dir);
 auto suggestions=panorama::decision54::suggestedMeasurements();
 std::ofstream file(dir/"next_measurements54.csv");
 file<<panorama::decision54::measurementsCSV(suggestions);
 std::cout<<"Wrote "<<suggestions.size()<<" instrument-design experiment suggestions\n";
 return file?0:3;
}
