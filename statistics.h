#pragma once
#include <vector>

using namespace std;

struct Stat {
    double mean = 0, sd = 0, mn = 0, mx = 0;
};

Stat describe(const vector<double>& v);
double fcrit(int df1, int df2);
void anova(const vector<vector<double>>& g);
