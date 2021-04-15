#ifndef LINOPT
#define LINOPT
#include <vector>
#include <cmath>
using namespace std;
#include <iostream>
#include <map>

struct LP
{
    vector<vector<float>> A;
    vector<float> b;
    vector<float> c;
    vector<float> x;
    float max_val;
    bool feasible;
    map<int, int> var_cons;  // idx_var -> idx_cons

    int num_init_cons;

    LP() {}

    LP(vector<vector<float>> &A, vector<float> &b, vector<float> &c)
    : A(A), b(b), c(c), max_val(0), feasible(true)
    {
        x.assign(c.size(), 0);
    }

    // LP(vector<vector<float>> &A, vector<float> &b, vector<float> &c, vector<float> &x, float max_val)
    // : A(A), b(b), c(c), x(x), max_val(max_val)
    // { 

    // }
    void mod_cons(int idx_x, float val, char type)
    {
        // type: 's' for smaller, 'g' for greater

        map<int, int>::iterator iter;
        iter = var_cons.find(idx_x);
        // 若不存在，则创建一个新的约束条件
        if (iter == var_cons.end())
        {
            var_cons.insert(make_pair(idx_x, b.size()));
            A.push_back(vector<float>(c.size(), 0));
            A.back().at(idx_x) =  type == 's' ? 1 : -1;
            b.push_back(type == 's' ? val : -val);
        }
        else
        {
            A.at(iter->second).at(idx_x) = type == 's' ? 1 : -1;
            b.at(iter->second) = type == 's' ? val : -val;
        }        
    }

    bool is_integer()
    {
        for (int i = 0; i < x.size(); ++i)
        {
            if (abs(floor(x[i] + 0.5) - x[i]) > 1e-4)
                return false;
        }
        return true;
    }
    void clear_state()
    {
        for (int i = 0; i < x.size(); ++i)
        {
            x[i] = 0;
        }
        max_val = 0;
    }
};


struct Solution
{
    vector<float> x;
    float max_val;
    Solution() {}
    Solution(int num_vars, float max_val)
    : max_val(max_val)
    {
        x.assign(num_vars, 0);
    }
};
              
void linpro(LP &lp);
vector<int> branch_and_cut_max(vector<vector<float>> &A, vector<float> &b, vector<float> &c, float &max_val, float max_obj);
vector<int> branch_and_cut_min(vector<vector<float>> &A, vector<float> &b, vector<float> &c, float &min_val, float min_obj);
#endif