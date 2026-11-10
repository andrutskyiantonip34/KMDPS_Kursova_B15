#pragma once
#include <string>
#include <vector>
#include <deque>
#include <limits>
#include <random>
#include <functional>

using namespace std;

inline const double INF_T = numeric_limits<double>::max();
inline const int INF_N = numeric_limits<int>::max();

inline double tcurr = 0.0;
inline mt19937 gen;

inline double uniform01() {
    return uniform_real_distribution<double>(0.0, 1.0)(gen);
}

class Element;

struct Route {
    Element* target;
    int priority;
    function<bool()> isBlocked;
};

class Element {
protected:
    string name;
    double tnext;
    double delayMean, delayDev;
    int quantity;
    vector<Route> routes;

    double getDelay() const {
        if (delayDev == 0.0) return delayMean;
        double a = delayMean - delayDev, b = delayMean + delayDev;
        return a + (b - a) * uniform01();
    }

    bool canSend() const {
        if (routes.empty()) return true;
        for (const auto& r : routes)
            //  check if route block func is nullptr or route is not blocked -> true
            if (!r.isBlocked || !r.isBlocked()) return true;
        return false;
    }

    //  go through all non-blocked routes and choose the one with the highest priority
    void sendToNext() {
        int best = -1, maxPrio = numeric_limits<int>::min();
        for (size_t i = 0; i < routes.size(); i++) {
            bool blocked = routes[i].isBlocked ? routes[i].isBlocked() : false;
            if (!blocked && routes[i].priority > maxPrio) {
                maxPrio = routes[i].priority;
                best = (int)i;
            }
        }
        if (best != -1) routes[best].target->inAct();
    }

public:
    Element(string n, double delay = 0.0, double dev = 0.0)
        : name(n), tnext(INF_T), delayMean(delay), delayDev(dev), quantity(0) {}
    virtual ~Element() {}

    void addRoute(Element* next, int priority = 1, function<bool()> cond = nullptr) {
        routes.push_back({next, priority, cond});
    }

    double getTnext() const { return tnext; }
    int getQuantity() const { return quantity; }
    string getName() const { return name; }

    virtual void inAct() {}
    virtual void outAct() {}
    virtual bool canUnblock() const { return false; }
    virtual void unblock() {}
    virtual double headTime() const { return INF_T; }
    virtual void doStatistics(double) {}
    virtual void resetStats() { quantity = 0; }
};

class Create : public Element {
public:
    Create(string n, double delay, double dev) : Element(n, delay, dev) {
        tnext = getDelay();
    }

    void outAct() override {
        quantity++;
        //  plan new generation
        tnext = tcurr + getDelay();
        //  move to the next
        sendToNext();
    }
};

class Process : public Element {
    //  0 - free, 1 - busy, 2 - blocked
    int state = 0;
    int queue = 0;
    double busyTime = 0.0;
    double waitArea = 0.0;
    deque<double> entryTimes;
    function<void()> onEnter;

    //  process the income заявка
    void completeService() {
        quantity++;
        if (!entryTimes.empty()) entryTimes.pop_front();
        state = 0;
        tnext = INF_T;
        sendToNext();
        if (queue > 0) {
            queue--;
            state = 1;
            tnext = tcurr + getDelay();
        }
    }

public:
    Process(string n, double delay, double dev = 0.0, function<void()> enter = nullptr)
        : Element(n, delay, dev), onEnter(enter) {}

    void inAct() override {
        //  if we have function to do on вхід заявки -> do it
        if (onEnter) onEnter();
        entryTimes.push_back(tcurr);
        if (state == 0) {
            state = 1;
            tnext = tcurr + getDelay();
        } else {
            queue++;
        }
    }

    void outAct() override {
        if (!canSend()) {
            state = 2;
            tnext = INF_T;
            return;
        }
        completeService();
    }

    bool canUnblock() const override { return state == 2 && canSend(); }
    void unblock() override { completeService(); }
    double headTime() const override { return entryTimes.empty() ? INF_T : entryTimes.front(); }

    void doStatistics(double dt) override {
        if (state == 1) busyTime += dt;
        waitArea += (queue + (state == 2 ? 1 : 0)) * dt;
    }

    void resetStats() override {
        quantity = 0;
        busyTime = 0;
        waitArea = 0;
    }

    int getState() const { return state; }
    bool isBusy() const { return state > 0; }
    int getQueue() const { return queue; }
    double getBusyTime() const { return busyTime; }
    double getWaitArea() const { return waitArea; }
};

class Dispose : public Element {
    function<void()> onDispose;
public:
    Dispose(string n, function<void()> f = nullptr) : Element(n), onDispose(f) {}

    void inAct() override {
        quantity++;
        //  if we have function to do on вихід заявки -> do it
        if (onDispose) onDispose();
    }
};
