#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
    string P, T;
    cin >> P >> T;
    
    int n = P.length();
    int m = T.length();
    
    if (n == 0) {
        cout << -1 << endl;
        return 0;
    }
    
    vector<int> pi(n, 0);
    for (int i = 1, j = 0; i < n; i++) {
        while (j > 0 && P[i] != P[j]) {
            j = pi[j - 1];
        }
        if (P[i] == P[j]) {
            j++;
        }
        pi[i] = j;
    }
    
    vector<int> positions;
    for (int i = 0, j = 0; i < m; i++) {
        while (j > 0 && T[i] != P[j]) {
            j = pi[j - 1];
        }
        if (T[i] == P[j]) {
            j++;
        }
        if (j == n) {
            positions.push_back(i - n + 1);
            j = pi[j - 1];
        }
    }
    
    if (positions.empty()) {
        cout << -1 << endl;
    } else {
        for (size_t i = 0; i < positions.size(); i++) {
            if (i > 0) cout << ",";
            cout << positions[i];
        }
        cout << endl;
    }
    
    return 0;
}