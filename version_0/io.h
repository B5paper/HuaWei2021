#ifndef IO
#define IO
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;


// map string to integer
unordered_map<string, int> server_names_map;
int server_names_id_top = 0;
vector<string> server_names_vec;

unordered_map<string, int> vm_names_map;
int vm_names_id_top = 0;
vector<string> vm_names_vec;

string server_id_to_name(int id);
string vm_id_to_name(int id);


// machine specifications and requirement infomation
struct ServerSpec
{
    vector<int> name_id;
    vector<int> cores;
    vector<int> memcap;
    vector<int> cost;
    vector<int> consume;
};

struct VMSpec
{
    vector<int> name_id;
    vector<int> cores;
    vector<int> memcap;
    vector<int> nodes;
};

struct DayReq
{
    vector<int> op;  // 0 for delete, 1 for add
    vector<int> name_id;  // -1 for none
    vector<int> id;
};


class StringProcesser
{
    public:
    string lstrip(string &str, string &str_to_strip);
    string rstrip(string &str, string &str_to_strip);
    vector<string> split_line(string str, string delimiter, string lstrip_str, string rstrip_str);
};

// requirements sequence reader
class ReqSeqReader: public StringProcesser
{
    public:
    void read_from_file(string path);
    void read_from_stdin();
    ~ReqSeqReader();

    private:
    void read_server_spec_line(string line);
    void read_vm_spec_line(string line);
    void read_vm_req_line(string line, DayReq *day_req);

    ServerSpec servers;
    VMSpec vms;
    vector<DayReq*> reqs;
};


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


// dispatch sequence writer
class DispSeqWriter: public StringProcesser
{
    public:
    DispSeqWriter();
    DispSeqWriter(bool is_test);
    ~DispSeqWriter();
    void write_to_stdout(vector<DayDisp*> disps);
    void test_output() {write_to_stdout(test_data);}

    private:
    bool is_test;
    vector<DayDisp*> test_data;
};
#endif