#pragma once
#include <string>
#include <vector>
#include "elements.h"
#include "statistics.h"

struct Params {
    double arrMean = 15, arrDev = 7;
    double procT = 7;
    double linMean = 15, linDev = 5;
    int Nmax = 3;
    int warmupN = 100;
    double Tmod = 100000;
    unsigned seed = 1;
    bool trace = false;
    int traceLimit = 0;
    double checkpointStep = 0;
};

struct Result {
    double Tmod;
    double Upr, Ulin[2];
    int N[2], K[2], F[2];
    double Pref[2], PrefTotal;
    Stat T[2], Tall;
    double nAvg[2], Lin[2], Lout[2];
    vector<double> times;
};

Result runOnce(const Params& p, vector<vector<double>>* cps = nullptr);
string nmaxStr(int n);
void printResult(const Params& p, const Result& r);
