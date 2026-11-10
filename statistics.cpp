#include <iostream>
#include <cmath>
#include <algorithm>
#include "statistics.h"

Stat describe(const vector<double>& v) {
    Stat s;
    if (v.empty()) return s;
    s.mn = *min_element(v.begin(), v.end());
    s.mx = *max_element(v.begin(), v.end());
    for (double x : v) s.mean += x;
    s.mean /= v.size();
    for (double x : v) s.sd += (x - s.mean) * (x - s.mean);
    if (v.size() > 1) s.sd = sqrt(s.sd / (v.size() - 1));
    return s;
}

double fcrit(int df1, int df2) {
    double z = 1.6449, a = 2.0 / (9 * df1), b = 2.0 / (9 * df2);
    double num = (1 - a) * (1 - b) + z * sqrt((1 - a) * (1 - a) * b + (1 - b) * (1 - b) * a - z * z * a * b);
    double den = (1 - b) * (1 - b) - z * z * b;
    return pow(num / den, 3);
}

void anova(const vector<vector<double>>& g) {
    int k = g.size(), n = 0;
    double total = 0;
    for (auto& v : g) {
        for (double x : v) total += x;
        n += v.size();
    }
    total /= n;
    double ssb = 0, ssw = 0;
    for (auto& v : g) {
        double m = 0;
        for (double x : v) m += x;
        m /= v.size();
        ssb += v.size() * (m - total) * (m - total);
        for (double x : v) ssw += (x - m) * (x - m);
    }
    int df1 = k - 1, df2 = n - k;
    cout << "SS_факт = " << ssb << ", SS_залиш = " << ssw << ", df1 = " << df1 << ", df2 = " << df2
         << ", F = " << (ssb / df1) / (ssw / df2) << ", Fкр = " << fcrit(df1, df2) << "\n";
}
