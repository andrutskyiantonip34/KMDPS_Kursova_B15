#pragma once
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "elements.h"

class Model {
    vector<Element*> list;
public:
    function<void(double)> onStep;
    function<bool()> warmupDone;
    function<void()> onWarmupEnd;
    double checkpointStep = 0.0;
    function<void(double)> onCheckpoint;
    bool trace = false;
    int traceLimit = 0;
    function<string()> stateString;
    double tStart = 0.0;

    Model(vector<Element*> el) : list(el) {}

    void resolveBlocked() {
        //  go through all blocked elements -> unblock all we can (by FIFO)
        while (true) {
            Element* best = nullptr;
            for (auto* e : list)
                if (e->canUnblock() && (!best || e->headTime() < best->headTime())) best = e;
            if (!best) return;
            best->unblock();
            if (trace && traceLimit > 0) print("розблокування " + best->getName());
        }
    }

    void print(const string& ev) {
        cout << fixed << setprecision(3) << tcurr << "  " << ev << "  " << stateString() << "\n";
        traceLimit--;
    }

    void advance(double t) {
        double dt = t - tcurr;
        for (auto* e : list) e->doStatistics(dt);
        if (onStep) onStep(dt);
        tcurr = t;
    }

    void simulate(double Tmod) {
        tcurr = 0.0;
        tStart = 0.0;
        bool warm = !warmupDone;
        double tEnd = warm ? Tmod : INF_T;
        double nextCp = checkpointStep;

        while (true) {
            resolveBlocked();

            //  find the nearest next event
            double tmin = INF_T;
            Element* next = nullptr;
            for (auto* e : list)
                if (e->getTnext() < tmin) {
                    tmin = e->getTnext();
                    next = e;
                }
            if (next == nullptr) break;

            //  calc statistics
            double tStop = min(tmin, tEnd);
            while (onCheckpoint && checkpointStep > 0 && nextCp <= tStop) {
                advance(nextCp);
                onCheckpoint(tcurr - tStart);
                nextCp += checkpointStep;
            }
            advance(tStop);
            if (tmin > tEnd) break;

            //  process the nearest event
            next->outAct();
            if (trace && traceLimit > 0) print(next->getName());

            //  check if warmup ended
            if (!warm && warmupDone()) {
                warm = true;
                tStart = tcurr;
                tEnd = tStart + Tmod;
                for (auto* e : list) e->resetStats();
                if (onWarmupEnd) onWarmupEnd();
            }
        }
    }
};
