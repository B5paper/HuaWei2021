#ifndef MATPLOT
#define MATPLOT
#include <string>
#include <vector>
#include "types.h"
using namespace std;

void write_py(vector<string> &strs, string file_path);
void plot_int_vec(vector<int> &nums);
void plot_servs(vector<int> &cores, vector<int> &mems, vector<int> &costs, vector<int> &consums);
void write_serv_info(ServSpecList &serv_specs, string path="d:/documents/servs.txt");

#endif