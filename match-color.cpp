#include <iostream>
#include <fstream>
#include <utility>
#include <ctime>
// TCLAP
#include "tclap/CmdLine.h"

#include <sdsl/bit_vectors.hpp>

// C STDLIB Headers
#include <cstdio>
#include <cstdlib>

#include <libgen.h>

// Custom Headers
#include "uint128_t.hpp"
#include "debug.h"

using namespace std;
using namespace sdsl;

#include <cstdlib>
#include <sys/timeb.h>

int getMilliCount();
int getMilliCount(){
  timeb tb;
  ftime(&tb);
  int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
  return nCount;
}

int getMilliSpan(int nTimeStart);
int getMilliSpan(int nTimeStart){
  int nSpan = getMilliCount() - nTimeStart;
  if(nSpan < 0)
    nSpan += 0x100000 * 1000;
  return nSpan;
}

typedef struct p
{
  std::string input_filename = "";
  int num_colors;
  int target_color;
} parameters_t;

void parse_arguments(int argc, char **argv, parameters_t & params);
void parse_arguments(int argc, char **argv, parameters_t & params)
{
  TCLAP::CmdLine cmd("Cosmo Copyright (c) Alex Bowe (alexbowe.com) 2014", ' ', VERSION);
  TCLAP::UnlabeledValueArg<std::string> input_filename_arg("input",
            "Input file. Currently only supports DSK's binary format (for k<=64).", true, "", "input_file", cmd);
  TCLAP::UnlabeledValueArg<std::string> num_colors_arg("num_colors",
            "Number of colors", true, "", "num colors", cmd);
  TCLAP::UnlabeledValueArg<std::string> target_color_arg("target_color",
            "Target color", true, "", "target color", cmd);
  cmd.parse( argc, argv );
  params.input_filename  = input_filename_arg.getValue();
  params.num_colors  = atoi(num_colors_arg.getValue().c_str());
  params.target_color  = atoi(target_color_arg.getValue().c_str());

}

int main(int argc, char * argv[]) {
  cout <<"Starting\n";
  parameters_t params;
  parse_arguments(argc, argv, params);

  const char * file_name = params.input_filename.c_str();
  cout << file_name << "\n";

  // Open File
  ifstream colorfile(file_name, ios::in|ios::binary);

  colorfile.seekg(0, colorfile.end);
  size_t end = colorfile.tellg();
  colorfile.seekg(0, colorfile.beg);

  size_t num_color = params.num_colors;
  size_t num_edges = end / 8;
  size_t cnt[55] = {0};
  size_t match[55] = {0};
  uint64_t mask[55];
  for (size_t j=0; j < num_color; j++) {
    mask[j] = 1ULL << j;
  }

  uint64_t target_color = params.target_color;
  for (size_t i=0; i < num_edges; i++) {
    uint64_t value;
    colorfile.read((char *)&value, sizeof(uint64_t));
    for (size_t j=0; j < num_color; j++) {
      if (value & mask[j]) {
	cnt[j]++;
	if (value & mask[target_color])
	  match[j]++;
      }
    }
  }
  cout << "edges: " << num_edges << " colors: " << num_color << " Total: " << num_edges * num_color << endl;
  for (size_t j=0; j < num_color; j++) {
    cout << j << ",\t" << cnt[j] << ",\t" << match[j] << ",\t" << float(match[j])/cnt[j] << "\n";
  }
}
