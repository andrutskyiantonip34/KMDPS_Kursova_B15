#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include "simulation.h"
#include "experiments.h"

void printProtocol() {
    cout << "1. Протокол подій\n";
    Params p;
    p.Tmod = 200;
    p.warmupN = 0;
    p.trace = true;
    p.traceLimit = 40;
    p.seed = 7;
    runOnce(p);
    cout << "\n";
}

void baseRuns() {
    cout << "2. Базові прогони\n";
    for (int nm : {3, INF_N}) {
        Params p;
        p.Nmax = nm;
        printResult(p, runOnce(p));
    }
}

void verification() {
    cout << "3. Верифікація\n";
    cout << "надх обр лін Nmax | U_пр U_лін1 P_відм T1 n1 | U_пр(розр) U_лін1(розр)\n";
    struct V { double am, ad, pt, lm, ld; int nm; };
    vector<V> vs = {
        {15, 7, 7, 15, 5, 3}, {30, 7, 7, 15, 5, 3}, {60, 7, 7, 15, 5, 3}, {10, 5, 7, 15, 5, 3},
        {15, 7, 3, 15, 5, 3}, {15, 7, 7, 25, 5, 3}, {15, 7, 7, 15, 0, 3}, {15, 0, 7, 15, 0, 3},
        {15, 7, 7, 15, 5, 1}, {15, 7, 7, 15, 5, 10}, {30, 7, 7, 15, 5, INF_N}};
    for (auto& v : vs) {
        Params p;
        p.arrMean = v.am;
        p.arrDev = v.ad;
        p.procT = v.pt;
        p.linMean = v.lm;
        p.linDev = v.ld;
        p.Nmax = v.nm;
        Result r = runOnce(p);
        double lam1 = r.K[0] / r.Tmod, lam2 = r.K[1] / r.Tmod;
        cout << setprecision(0) << v.am << "±" << v.ad << " " << v.pt << " " << v.lm << "±" << v.ld << " "
             << nmaxStr(v.nm) << " | " << setprecision(4) << r.Upr << " " << r.Ulin[0] << " " << r.PrefTotal
             << " " << r.T[0].mean << " " << r.nAvg[0] << " | " << (lam1 + lam2) * v.pt << " " << lam1 * v.lm << "\n";
    }
    cout << "\n";
}

void transientPeriod() {
    cout << "4. Перехідний період (5 прогонів)\n";
    cout << "t | T сер | P_відм | dT dP\n";
    int R = 5;
    vector<vector<vector<double>>> all(R);
    for (int s = 0; s < R; s++) {
        Params p;
        p.warmupN = 0;
        p.Tmod = 20000;
        p.checkpointStep = 500;
        p.seed = 100 + s;
        runOnce(p, &all[s]);
    }
    for (size_t i = 0; i < all[0].size(); i++) {
        cout << setprecision(0) << all[0][i][0] << " |" << setprecision(2);
        for (int s = 0; s < R; s++) cout << " " << all[s][i][1];
        cout << " |" << setprecision(4);
        double tMin = 1e9, tMax = -1e9, pMin = 1e9, pMax = -1e9;
        for (int s = 0; s < R; s++) {
            cout << " " << all[s][i][2];
            tMin = min(tMin, all[s][i][1]);
            tMax = max(tMax, all[s][i][1]);
            pMin = min(pMin, all[s][i][2]);
            pMax = max(pMax, all[s][i][2]);
        }
        cout << " | " << setprecision(2) << tMax - tMin << " " << setprecision(4) << pMax - pMin << "\n";
    }
    cout << "\n";
}

void runsCount() {
    cout << "5. Кількість прогонів\n";
    vector<double> P, T, U, UL;
    for (int s = 0; s < 20; s++) {
        Params p;
        p.Tmod = 20000;
        p.seed = 1000 + s;
        Result r = runOnce(p);
        P.push_back(r.PrefTotal);
        T.push_back(r.Tall.mean);
        U.push_back(r.Upr);
        UL.push_back(r.Ulin[0]);
    }
    Stat sP = describe(P), sT = describe(T), sU = describe(U), sUL = describe(UL);
    double eP = 0.005, eT = 0.05 * sT.mean, eU = 0.01;
    cout << "P_відм: сер = " << sP.mean << ", sigma = " << sP.sd << ", p = " << 20 * pow(sP.sd / eP, 2) << "\n";
    cout << "T: сер = " << sT.mean << ", sigma = " << sT.sd << ", p = " << 20 * pow(sT.sd / eT, 2) << "\n";
    cout << "U_пр: сер = " << sU.mean << ", sigma = " << sU.sd << ", p = " << 20 * pow(sU.sd / eU, 2) << "\n";
    cout << "U_лін1: сер = " << sUL.mean << ", sigma = " << sUL.sd << ", p = " << 20 * pow(sUL.sd / eU, 2) << "\n\n";
}

void nmaxInfluence() {
    cout << "6. Вплив Nmax (20 прогонів)\n";
    cout << "Nmax | P_відм U_пр U_лін1 T сер СКВ T max\n";
    vector<vector<double>> gT, gP;
    vector<double> T3, Tinf;
    for (int nm : {1, 2, 3, 4, 5, 6, INF_N}) {
        vector<double> P, T, SD, U, UL, MX;
        for (int s = 0; s < 20; s++) {
            Params p;
            p.Nmax = nm;
            p.Tmod = 20000;
            p.seed = 2000 + s;
            Result r = runOnce(p);
            P.push_back(r.PrefTotal);
            T.push_back(r.Tall.mean);
            SD.push_back(r.Tall.sd);
            U.push_back(r.Upr);
            UL.push_back(r.Ulin[0]);
            MX.push_back(r.Tall.mx);
        }
        cout << nmaxStr(nm) << " | " << describe(P).mean << " " << describe(U).mean << " " << describe(UL).mean
             << " " << setprecision(2) << describe(T).mean << " " << describe(SD).mean << " "
             << describe(MX).mean << setprecision(4) << "\n";
        if (nm == 3) T3 = T;
        if (nm == INF_N) Tinf = T;
        else {
            gT.push_back(T);
            gP.push_back(P);
        }
    }
    cout << "\nДисперсійний аналіз, T, Nmax = 1..6: ";
    anova(gT);
    cout << "Дисперсійний аналіз, P_відм, Nmax = 1..6: ";
    anova(gP);
    cout << "Дисперсійний аналіз, T, Nmax = 3 та inf: ";
    anova({T3, Tinf});
    cout << "\n";
}

void noFlowControl() {
    cout << "7. Без контролю потоків\n";
    cout << "Tmod | T сер СКВ T max L_вих1 U_лін1\n";
    for (double T : {1e3, 1e4, 1e5, 1e6}) {
        Params p;
        p.Nmax = INF_N;
        p.Tmod = T;
        Result r = runOnce(p);
        cout << setprecision(0) << T << " | " << setprecision(2) << r.Tall.mean << " " << r.Tall.sd << " "
             << r.Tall.mx << " " << r.Lout[0] << " " << setprecision(4) << r.Ulin[0] << "\n";
    }
    cout << "\n";
}

void improvement() {
    cout << "8. Зменшення часу передачі по лінії (20 прогонів)\n";
    cout << "t_пер Nmax | P_відм U_пр U_лін1 T сер СКВ T max\n";
    for (double lin : {15.0, 13.0, 12.0, 10.0}) {
        for (int nm : {3, INF_N}) {
            vector<double> P, T, SD, U, UL, MX;
            for (int s = 0; s < 20; s++) {
                Params p;
                p.linMean = lin;
                p.Nmax = nm;
                p.Tmod = 20000;
                p.seed = 3000 + s;
                Result r = runOnce(p);
                P.push_back(r.PrefTotal);
                T.push_back(r.Tall.mean);
                SD.push_back(r.Tall.sd);
                U.push_back(r.Upr);
                UL.push_back(r.Ulin[0]);
                MX.push_back(r.Tall.mx);
            }
            cout << setprecision(0) << lin << " " << nmaxStr(nm) << " | " << setprecision(4) << describe(P).mean << " "
                 << describe(U).mean << " " << describe(UL).mean << " " << setprecision(2) << describe(T).mean << " "
                 << describe(SD).mean << " " << describe(MX).mean << setprecision(4) << "\n";
        }
    }
    cout << "\n";
}
