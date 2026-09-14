#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    string A, B;
    if (!(cin >> A >> B)) return 0;

    int n = A.length();
    int m = B.length();

    if (n != m) {
        cout << -1 << endl;
        return 0;
    }

    vector<int> pi(m);
    for (int i = 1; i < m; ++i) {
        int j = pi[i - 1];
        while (j > 0 && B[i] != B[j]) {
            j = pi[j - 1];
        }
        if (B[i] == B[j]) {
            ++j;
        }
        pi[i] = j;
    }

    int j = 0;
    for (int i = 0; i < 2 * n; ++i) {
        char c;
        if (i < n) {
            c = A[i];
        } else {
            c = A[i - n];
        }
        while (j > 0 && c != B[j]) {
            j = pi[j - 1];
        }
        if (c == B[j]) {
            ++j;
        }
        if (j == m) {
            int startIdx = i - m + 1;
            if (startIdx < n) {
                cout << startIdx << endl;
                return 0;
            }
        }
    }

    cout << -1 << endl;

    return 0;
}