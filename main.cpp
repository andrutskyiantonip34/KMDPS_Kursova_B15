#include <iostream>
#include <iomanip>
#include "experiments.h"

using namespace std;

int main() {
    cout << fixed << setprecision(4);

    printProtocol();
    baseRuns();
    verification();
    transientPeriod();
    runsCount();
    nmaxInfluence();
    noFlowControl();
    improvement();

    return 0;
}
