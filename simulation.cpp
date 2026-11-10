#include <iostream>
#include "model.h"
#include "simulation.h"

Result runOnce(const Params& p, vector<vector<double>>* cps) {
    gen.seed(p.seed);
    tcurr = 0.0;

    int N[2] = {0, 0}, F[2] = {0, 0};
    long passed = 0;
    deque<double> tIn[2];
    vector<double> times[2];
    double area[2] = {0, 0};
    double sumT = 0;
    long cntT = 0;

    Create c1("Надходження 1", p.arrMean, p.arrDev);
    Create c2("Надходження 2", p.arrMean, p.arrDev);
    Dispose F1("Відмова F1", [&]() { F[0]++; });
    Dispose F2("Відмова F2", [&]() { F[1]++; });
    Process buf1("Буфер 1", 0.0, 0.0, [&]() { N[0]++; tIn[0].push_back(tcurr); });
    Process buf2("Буфер 2", 0.0, 0.0, [&]() { N[1]++; tIn[1].push_back(tcurr); });
    Process pr1("D_пр1", p.procT);
    Process pr2("D_пр2", p.procT);
    Process lin1("D_лін1", p.linMean, p.linDev);
    Process lin2("D_лін2", p.linMean, p.linDev);

    auto leave = [&](int k) {
        double T = tcurr - tIn[k].front();
        tIn[k].pop_front();
        times[k].push_back(T);
        sumT += T;
        cntT++;
        passed++;
    };
    Dispose K1("Вихід K1", [&]() { leave(0); });
    Dispose K2("Вихід K2", [&]() { leave(1); });

    c1.addRoute(&buf1, 2, [&]() { return (int)tIn[0].size() >= p.Nmax; });
    c1.addRoute(&F1, 1);
    c2.addRoute(&buf2, 2, [&]() { return (int)tIn[1].size() >= p.Nmax; });
    c2.addRoute(&F2, 1);

    auto procBusy = [&]() { return pr1.isBusy() || pr2.isBusy(); };
    buf1.addRoute(&pr1, 1, procBusy);
    buf2.addRoute(&pr2, 1, procBusy);
    pr1.addRoute(&lin1);
    pr2.addRoute(&lin2);
    lin1.addRoute(&K1);
    lin2.addRoute(&K2);

    Model m({&c1, &c2, &buf1, &buf2, &pr1, &pr2, &lin1, &lin2, &F1, &F2, &K1, &K2});

    m.onStep = [&](double dt) {
        area[0] += tIn[0].size() * dt;
        area[1] += tIn[1].size() * dt;
    };

    if (p.warmupN > 0) {
        m.warmupDone = [&]() { return passed >= p.warmupN; };
        m.onWarmupEnd = [&]() {
            N[0] = N[1] = F[0] = F[1] = 0;
            area[0] = area[1] = 0;
            times[0].clear();
            times[1].clear();
            sumT = 0;
            cntT = 0;
        };
    }

    m.trace = p.trace;
    m.traceLimit = p.traceLimit;
    m.stateString = [&]() {
        return "Lвх=" + to_string(buf1.getQueue()) + "," + to_string(buf2.getQueue()) +
               " Dбуф=" + to_string(buf1.getState()) + "," + to_string(buf2.getState()) +
               " Dпр=" + to_string(pr1.getState()) + "," + to_string(pr2.getState()) +
               " Lвих=" + to_string(lin1.getQueue()) + "," + to_string(lin2.getQueue()) +
               " Dлін=" + to_string(lin1.getState()) + "," + to_string(lin2.getState()) +
               " N-K=" + to_string(tIn[0].size()) + "," + to_string(tIn[1].size()) +
               " F=" + to_string(F[0]) + "," + to_string(F[1]);
    };

    if (cps && p.checkpointStep > 0) {
        m.checkpointStep = p.checkpointStep;
        m.onCheckpoint = [&](double t) {
            double arrived = N[0] + N[1] + F[0] + F[1];
            cps->push_back({t, cntT ? sumT / cntT : 0.0, arrived ? (F[0] + F[1]) / arrived : 0.0});
        };
    }

    m.simulate(p.Tmod);

    Result r;
    r.Tmod = tcurr - m.tStart;
    r.Upr = (pr1.getBusyTime() + pr2.getBusyTime()) / r.Tmod;
    Process* L[2] = {&lin1, &lin2};
    Process* B[2] = {&buf1, &buf2};
    for (int k = 0; k < 2; k++) {
        r.Ulin[k] = L[k]->getBusyTime() / r.Tmod;
        r.N[k] = N[k];
        r.F[k] = F[k];
        r.K[k] = times[k].size();
        r.Pref[k] = (N[k] + F[k]) ? (double)F[k] / (N[k] + F[k]) : 0.0;
        r.T[k] = describe(times[k]);
        r.nAvg[k] = area[k] / r.Tmod;
        r.Lin[k] = B[k]->getWaitArea() / r.Tmod;
        r.Lout[k] = L[k]->getWaitArea() / r.Tmod;
    }
    double arrived = N[0] + N[1] + F[0] + F[1];
    r.PrefTotal = arrived ? (F[0] + F[1]) / arrived : 0.0;
    r.times = times[0];
    r.times.insert(r.times.end(), times[1].begin(), times[1].end());
    r.Tall = describe(r.times);
    return r;
}

string nmaxStr(int n) {
    return n == INF_N ? "inf" : to_string(n);
}

void printResult(const Params& p, const Result& r) {
    cout << "Nmax = " << nmaxStr(p.Nmax) << ", Tmod = " << r.Tmod << "\n";
    cout << "U_пр = " << r.Upr << ", U_лін1 = " << r.Ulin[0] << ", U_лін2 = " << r.Ulin[1] << "\n";
    for (int k = 0; k < 2; k++) {
        cout << "Напрямок " << k + 1 << ": N = " << r.N[k] << ", K = " << r.K[k] << ", F = " << r.F[k]
             << ", P_відм = " << r.Pref[k] << "\n";
        cout << "  T: сер = " << r.T[k].mean << ", СКВ = " << r.T[k].sd
             << ", min = " << r.T[k].mn << ", max = " << r.T[k].mx << "\n";
        cout << "  n = " << r.nAvg[k] << ", L_вх = " << r.Lin[k] << ", L_вих = " << r.Lout[k]
             << ", K/Tmod*T = " << r.K[k] / r.Tmod * r.T[k].mean << "\n";
    }
    cout << "P_відм = " << r.PrefTotal << ", T сер = " << r.Tall.mean << ", СКВ = " << r.Tall.sd << "\n\n";
}
