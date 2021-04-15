#include "types.h"
#include "io.h"
#include "status.h"
#include "eval.h"
#include "dispatch.h"
#include <iostream>
#include <ctime>
using namespace std;

bool check(ServerStat &server_stat, ServerSpecList &server_specs)
{
    for (int i = 0; i < server_stat.ids.size(); ++i)
    {
        if (server_stat.cores_used[i] > server_specs.memcap[server_stat.types[i]])
            return false;
        if (server_stat.mem_used[i] > server_specs.memcap[server_stat.types[i]])
            return false;
        for (int j = 0; j < 2; ++j)
        {
            if (server_stat.nodes[j][i]->cores_used > server_specs.memcap[server_stat.types[i]] / 2)
                return false;
            if (server_stat.nodes[j][i]->mem_used > server_specs.memcap[server_stat.types[i]] / 2)
                return false;
        }
    }
    return true;
}

int main()
{
    ServerSpecList *server_specs = NULL;
    VMSpecList *vm_specs = NULL;
    DayReqList *reqs = NULL;
    ServerStat *server_stat = NULL;
    VMStat *vm_stat = NULL;
    ServerStatRecorder *rec = NULL;
    DayDispList *disps = NULL;

    // freopen("C:\\Users\\Administrator\\Desktop\\training-2.txt", "r", stdin);
    // freopen("c:\\users\\administrator\\desktop\\output2.txt", "w", stdout);
    for (int i = 0; i < 1; ++i)
    {
        server_specs = new ServerSpecList;
        vm_specs = new VMSpecList;
        reqs = new DayReqList;
        server_stat = new ServerStat;
        vm_stat = new VMStat;
        rec = new ServerStatRecorder;
        disps = new DayDispList;

        read_from_stdin(*server_specs, *vm_specs, *reqs);
        
        int rtv = 0;
        for (int days_processed = 0; days_processed < (*reqs).size(); )
        {
            (*disps).clear();
            rtv = dispatch(*server_specs, *vm_specs, *reqs, *server_stat, *vm_stat, *rec, *disps);
            days_processed += rtv;
            write_to_stdout(*disps);
            // if (!check(*server_stat, *server_specs))
            // {
            //     cout << "exceed resources of servers." << endl;
            //     return 0;
            // }
        }

        fflush(stdout);

        return 0;

        delete server_specs;
        delete vm_specs;
        delete reqs;
        delete server_stat;
        delete vm_stat;
        delete rec;
        delete disps;
    }

    return 0;
}