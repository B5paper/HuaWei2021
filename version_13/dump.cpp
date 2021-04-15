#include "dump.h"

void dump_serv_stats(ServStatList &serv_stats, ServSpecList &serv_specs, string path)
{
    FILE *f = fopen(path.c_str(), "w");
    for (auto serv_stat: serv_stats.servs)
    {
        fprintf(f, "%d %d %d %d %d %d %d %d\n", 
            serv_stat.first, serv_stat.second.type, 
            serv_specs[serv_stat.second.type].cores, serv_specs[serv_stat.second.type].memcap, 
            serv_stat.second.nodes[0].cores_ream, serv_stat.second.nodes[0].mem_ream,
            serv_stat.second.nodes[1].cores_ream, serv_stat.second.nodes[1].mem_ream);  // id, type, core cap, mem cap, node 0 cores ream, mem ream, node 1 cores ream, mem ream
    }
    fclose(f);
}