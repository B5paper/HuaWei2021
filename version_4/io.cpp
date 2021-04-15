#include "io.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <unordered_set>
#include <cstring>
using namespace std;

#define BUFFERSIZE 50

// util functions
string server_id_to_name(int id)
{
    return server_names_vec[id];
}


string vm_id_to_name(int id)
{
    return vm_names_vec[id];
}


// string lstrip(string &str, string &str_to_strip)
// {
//     return str.substr(str.find(str_to_strip) + str_to_strip.size(), str.npos);
// }


// string rstrip(string &str, string &str_to_strip)
// {
//     return str.substr(0, str.rfind(str_to_strip));
// }


// vector<string> split_line(string str, string delimiter, string lstrip_str, string rstrip_str)
// {
//     vector<string> strs;
//     str = lstrip(str, lstrip_str);
//     str = rstrip(str, rstrip_str);
//     vector<int> idx_start, len_substr;
//     int idx_temp = 0;
//     idx_start.push_back(idx_temp);
//     while (true)
//     {
//         idx_temp = str.find(delimiter, idx_temp+1);
//         if (idx_temp == str.npos)
//         {
//             len_substr.push_back(str.npos);
//             break;
//         }
//         len_substr.push_back(idx_temp - idx_start[idx_start.size()-1]);
//         idx_start.push_back(idx_temp + delimiter.size());
//     }

//     for (int i = 0; i < idx_start.size(); ++i)
//     {
//         strs.push_back(str.substr(idx_start[i], len_substr[i]));
//     }
//     return strs;
// }


void read_from_stdin(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs)
{
    int num_serv;
    scanf("%d", &num_serv);
    char serv_name[BUFFERSIZE];
    int serv_core, serv_mem, serv_cost, serv_consume;
    for (int i = 0; i < num_serv; ++i)
    {
        scanf("%s", serv_name);
        serv_name[strlen(serv_name)-1] = '\0';
        scanf("%d,", &serv_core);
        scanf("%d,", &serv_mem);
        scanf("%d,", &serv_cost);
        scanf("%d)", &serv_consume);
        // scanf("(%s, %d, %d, %d, %d)", serv_name, &serv_core, &serv_mem, &serv_cost, &serv_consume);
        server_names_map.insert(make_pair(string(serv_name+1), server_names_id_top));
        ++server_names_id_top;
        server_names_vec.push_back(serv_name+1);
        server_specs.cores.push_back(serv_core);
        server_specs.memcap.push_back(serv_mem);
        server_specs.cost.push_back(serv_cost);
        server_specs.consume.push_back(serv_consume);
    }

    int num_vm;
    scanf("%d", &num_vm);
    char vm_name[BUFFERSIZE];
    int vm_core, vm_mem, vm_node;
    for (int i = 0; i < num_vm; ++i)
    {
        scanf("%s", vm_name);
        vm_name[strlen(vm_name)-1] = '\0';
        scanf("%d,", &vm_core);
        scanf("%d,", &vm_mem);
        scanf("%d)", &vm_node);
        vm_names_map.insert(make_pair(vm_name+1, vm_names_id_top));
        ++vm_names_id_top;
        vm_names_vec.push_back(vm_name+1);
        vm_specs.cores.push_back(vm_core);
        vm_specs.memcap.push_back(vm_mem);
        vm_specs.nodes.push_back(vm_node+1);
    }

    int num_req_day;
    scanf("%d", &num_req_day);
    int num_req;
    char buf_req[BUFFERSIZE];
    char buf_req_vm_name[BUFFERSIZE];
    int vm_id;
    DayReq *day_req = NULL;
    for (int i = 0; i < num_req_day; ++i)
    {
        day_req = new DayReq;
        scanf("%d", &num_req);
        for (int j = 0; j < num_req; ++j)
        {
            scanf("%s", buf_req);
            if (buf_req[1] == 'a')
            {
                scanf("%s", buf_req_vm_name);
                buf_req_vm_name[strlen(buf_req_vm_name)-1] = '\0';
                scanf("%d)", &vm_id);
                day_req->id.push_back(vm_id);
                day_req->name_id.push_back(vm_names_map.find(buf_req_vm_name)->second);
                day_req->op.push_back(1);
            }
            else
            {
                scanf("%d)", &vm_id);
                day_req->op.push_back(0);
                day_req->name_id.push_back(-1);
                day_req->id.push_back(vm_id);
            }
        }
        reqs.push_back(day_req);
    }
}



void disp_pur_server(int type, DayDispList &disps, int num)
{
    DayDisp *disp_back = disps.disp_list.back();
    int idx = -1;
    for (int i = 0; i < disp_back->pur_server_name_ids.size(); ++i)
    {
        if (disp_back->pur_server_name_ids[i] == type)
        {
            idx = i;
            break;
        }
    }

    if (idx != -1)
    {
        disp_back->pur_server_nums[idx] += num;
    }
    else
    {
        disp_back->pur_server_name_ids.push_back(type);
        disp_back->pur_server_nums.push_back(1);
    }
}

void disp_mig_vm(int vm_id, int server_id, DayDispList &disps)
{

}

void disp_add_vm(int server_id, int server_node, DayDispList &disps)
{

}

void write_to_stdout(DayDispList &disps)
{
    for (int i = 0; i < disps.disp_list.size(); ++i)
    {
        // purchase
        unordered_set<int> serv_type_nums;
        int serv_type_num = 0;
        for (int j = 0; j < disps.disp_list[i]->pur_server_name_ids.size(); ++j)
        {
            if (serv_type_nums.find(disps.disp_list[i]->pur_server_name_ids[j]) != serv_type_nums.end())
            {
                ++serv_type_num;
            }
            else
            {
                serv_type_nums.insert(disps.disp_list[i]->pur_server_name_ids[j]);
                ++serv_type_num;
            }
        }
        // cout << "(purchase, " << serv_nums << ")" << endl;
        // cout << "(purchase, " << serv_type_num << ")" << endl;
        printf("(purchase, %d)\n", serv_type_num);
        for (int j = 0; j < disps.disp_list[i]->pur_server_name_ids.size(); ++j)
        {
            printf("(%s, %d)\n", server_id_to_name(disps.disp_list[i]->pur_server_name_ids[j]).c_str(), disps.disp_list[i]->pur_server_nums[j]);
            // cout << "(" << server_id_to_name(disps.disp_list[i]->pur_server_name_ids[j]) << ", " << disps.disp_list[i]->pur_server_nums[j] << ")" << endl;
        }

        // migrate
        // cout << "(migration, 0)" << endl;
        printf("(migration, 0)\n");
        // cout << "(migration, " << disps.disp_list[i]->mig_vm_ids.size() << ")" << endl;
        // for (int j = 0; j < disps.disp_list[i]->mig_vm_ids.size(); ++j)
        // {
        //     cout << "(" << disps.disp_list[i]->mig_vm_ids[j] << ", " << disps.disp_list[i]->mig_server_ids[j];
        //     if (disps.disp_list[i]->mig_server_nodes[j] == 2)
        //         cout << ")" << endl;
        //     else
        //         cout << ", " << (disps.disp_list[i]->mig_server_nodes[j] == 0 ? "A)" : "B)") << endl;
        // }

        // process requirements
        for (int j = 0; j < disps.disp_list[i]->req_server_ids.size(); ++j)
        {          
            if (disps.disp_list[i]->req_server_nodes[j] == 2)
                printf("(%d)\n", disps.disp_list[i]->req_server_ids[j]);
            else
            {
                if (disps.disp_list[i]->req_server_nodes[j] == 0)
                    printf("(%d, %c)\n", disps.disp_list[i]->req_server_ids[j], 'A');
                else
                    printf("(%d, %c)\n", disps.disp_list[i]->req_server_ids[j], 'B');
            }
        }
    }

    fflush(stdout);
}


void DayDispList::clear()
{
    for (int i = 0; i < disp_list.size(); ++i)
    {
        delete disp_list[i];
    }
    disp_list.clear();
}


DayDispList::~DayDispList()
{
    clear();
}