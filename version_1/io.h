#ifndef IO
#define IO

#include <vector>
#include <string>
#include <unordered_map>
#include "types.h"
using namespace std;


// basic string processing
string lstrip(string &str, string &str_to_strip);
string rstrip(string &str, string &str_to_strip);
vector<string> split_line(string str, string delimiter, string lstrip_str, string rstrip_str);


// input functions
ServerSpec read_server_spec_line(string line);
VMSpec read_vm_spec_line(string line);
void read_vm_req_line(string line);
void read_from_stdin(ServerSpecList &servers, VMSpecList &vms, DayReqList &reqs);
void read_from_file(string path, ServerSpecList &servers, VMSpecList &vms, DayReqList &reqs);


// output functions
struct DayDisp
{
    // purchase
    vector<int> pur_server_name_ids;
    vector<int> pur_server_nums;

    // migrate
    vector<int> mig_vm_ids;
    vector<int> mig_server_ids;
    vector<int> mig_server_nodes;  // 0 for node A, 1 for node B, 2 for both

    // require
    vector<int> req_server_ids;
    vector<int> req_server_nodes;  // 0 for node A, 1 for node B, 2 for both
};

void write_to_stdout(vector<DayDisp*> disps);
void test_output();
#endif