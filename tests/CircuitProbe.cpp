// Copyright (c) 2026 c.d.r.c plugins. Licensed under GNU AGPLv3.
#include "../Source/Circuit.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <iomanip>
int main(int argc,char** argv) {
  const std::filesystem::path folder=argc>1?argv[1]:"spice-results";
  std::filesystem::create_directories(folder);
  for(int mode=0;mode<2;++mode)for(double boost:{1.0,0.5}) {
    rm::Circuit c;c.prepare(192000,mode?rm::Transistor::silicon():rm::Transistor::germanium(),boost);
    std::string prefix=std::string(mode?"si-":"ge-")+(boost==1.0?"1":"0.5");
    std::ofstream file(folder/(prefix+"-cpp.txt"));file<<std::setprecision(12);
    for(int i=0;i<=96000;++i) {
      double t=i/192000.0;
      double y=c.tick(0.3*std::sin(6.283185307179586*3000*t),boost);
      file<<t<<' '<<y<<'\n';
    }
  }
}
