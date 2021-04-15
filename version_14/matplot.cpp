#include "matplot.h"
#include <vector>
#include <string>
#include "types.h"

using namespace std;

void write_py(vector<string> &strs, string file_path)
{
    FILE *py_file = fopen(file_path.c_str(), "w");
    if (py_file == NULL)
    {
        printf("fail to open py file.\n");
    }

    for (auto &str: strs)
    {
        fprintf(py_file, "%s\n", str.c_str()); 
    }

    fclose(py_file);
}

void write_serv_info(ServSpecList &serv_specs, string path)
{
    FILE *f = fopen(path.c_str(), "w");
    for (auto serv: serv_specs)
    {
        fprintf(f, "%d %d %d %d\n", serv.cores, serv.memcap, serv.cost, serv.consume);
    }
    fclose(f);
}

void plot_int_vec(vector<int> &nums)
{
    vector<string> strs;
    strs.push_back("import matplotlib.pyplot as plt");
    strs.push_back("import numpy as np");
    strs.push_back("nums = np.array([");
    char num_buf[20] = {0};
    for (auto &num: nums)
    {
        sprintf(num_buf, "%d", num);
        // strcat(num_buf, ", ");
        strs.back().append(num_buf);
        strs.back().append(", ");
    }
    // int pos_comma = strs.back().rfind(',');
    strs.back().erase(strs.back().size() - 2);
    strs.back().append("])");
    strs.push_back("plt.plot(nums)");
    strs.push_back("plt.show()");

    write_py(strs, "D:\\Documents\\script.py");
}

void plot_servs(vector<int> &cores, vector<int> &mems, vector<int> &costs, vector<int> &consums)
{
    vector<string> strs;
    strs.push_back("import numpy as np");
    strs.push_back("import matplotlib as mpl");
    strs.push_back("mpl.use('tkagg')");
    strs.push_back("import mpl_toolkits.mplot3d");
    strs.push_back("import matplotlib.pyplot as plt");
    strs.push_back("cores = np.array([");
    char buf[20];
    for (int i = 0; i < cores.size(); ++i)
    {
        sprintf(buf, "%d", cores[i]);
        strs.back().append(buf);
        strs.back().append(", ");
    }
    strs.back().erase(strs.back().size() - 2);
    strs.back().append("])");

    strs.push_back("mems = np.array([");
    for (int i = 0; i < mems.size(); ++i)
    {
        sprintf(buf, "%d", mems[i]);
        strs.back().append(buf);
        strs.back().append(", ");
    }
    strs.back().erase(strs.back().size() - 2);
    strs.back().append("])");

    strs.push_back("costs = np.array([");
    for (auto cost: costs)
    {
        sprintf(buf, "%d", cost);
        strs.back().append(buf);
        strs.back().append(", ");
    }
    strs.back().erase(strs.back().size() - 2);
    strs.back().append("])");

    strs.push_back("fig = plt.figure()");
    strs.push_back("ax = fig.gca(projection='3d')");
    strs.push_back("ax.plot_wireframe(cores, mems, costs)");
    strs.push_back("plt.show()");

    write_py(strs, "d:/documents/script.py");

}


