#include "eval.h"

ulong get_cost(ServerStatRecorder &server_stat_recorder, ServerSpecList &server_specs)
{
    ulong cost;
    for (int i = 0; i < server_stat_recorder.types.size(); ++i)
    {
        if (server_stat_recorder.run_days[i]->size() != 0)
            cost += server_specs.cost[server_stat_recorder.types[i]];
    }

    for (int i = 0; i < server_stat_recorder.types.size(); ++i)
    {
        cost += server_specs.consume[server_stat_recorder.types[i]] * server_stat_recorder.run_days[i]->size();
    }
    return cost;
}
