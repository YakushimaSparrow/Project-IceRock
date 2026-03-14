#include "../../include/calculate/R.hpp"

int calculateMaxDragdown(const std::vector<int>& data)
{
    if (data.empty()) return 0;
    int peak = data[0], maxDd = 0;
    for (int v : data) {
        if (v > peak) peak = v;
        int dd = peak - v;
        if (dd > maxDd) maxDd = dd;
    }
    return maxDd;
}
