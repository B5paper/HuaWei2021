#ifndef DISPATCH
#define DISPATCH

#include "types.h"
#include "io.h"
#include "eval.h"
using namespace std;

struct ExpandScheme
{
    vector<int> types;
    vector<int> nums;
};

struct MigrateScheme
{
    vector<int> vm_ids;
    vector<int> server_ids;
};

// ExpandScheme expand_servers(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs);
// MigrateScheme migrate_vms();
void assign_vms();
void update_server_stat();
void update_vm_stat();
int dispatch(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs, 
            ServerStat &server_stat, VMStat &vmstat, ServerStatRecorder &rec,
            DayDispList &disps);  // return the number of days processed

#endif