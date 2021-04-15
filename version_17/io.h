#ifndef IO
#define IO

#include "types.h"
#include <vector>
#include <map>
using namespace std;


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

struct DayDispList
{
    // int idx_day;
    // int idx_add;
    vector<int> add_nums;
    vector<DayDisp*> disp_list;

    DayDispList() {};
    ~DayDispList();
    void clear();
    void new_day() 
    {
        disp_list.push_back(new DayDisp);
    }
    void push_add_vm(int day, int serv_id, int serv_node);
    void push_mig_vm(int day, int vm_id, int server_id, int server_node);
    void push_pur_server(int day, int type, int num = 1);

    void output_day(int idx_day_start, int idx_day_end);
};


void read_serv_vm_specs(ServSpecList &serv_specs, VMSpecList &vm_specs);
void read_num_days_k(int &num_days, int &k);
void read_one_day_reqs(DayReqList &reqs);

#endif