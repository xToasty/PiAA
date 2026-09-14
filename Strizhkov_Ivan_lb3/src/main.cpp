#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

int main() {
    string S, T;
    if (!(cin >> S >> T)) return 0;

    int k;
    cin >> k;
    vector<bool> isNew(T.length(), false);
    for (int i = 0; i < k; i++) {
        int idx;
        cin >> idx;
        if (idx >= 1 && idx <= (int)T.length()) {
            isNew[idx - 1] = true;
        }
    }

    int n = (int)S.length();
    int m = (int)T.length();
    const int INF = 1000000000;

    vector<int> prev(m + 1, 0), curr(m + 1, 0);

    prev[0] = 0;
    for (int j = 1; j <= m; j++) {
        char t_char = T[j - 1];
        bool canInsert;
        if (isNew[j - 1]) {
            canInsert = (t_char == 'V'); 
        } else {
            canInsert = true;
        }
        if (canInsert && prev[j - 1] != INF) {
            prev[j] = prev[j - 1] + 1;
        } else {
            prev[j] = INF;
        }
    }

    for (int i = 1; i <= n; i++) {
        curr[0] = i;

        for (int j = 1; j <= m; j++) {
            char s_char = S[i - 1];
            char t_char = T[j - 1];
            bool tIsNew = isNew[j - 1];
            bool tIsV = (t_char == 'V');

            int best = INF;

            if (s_char == t_char) {
                bool canKeep;
                if (tIsNew) {
                    canKeep = !tIsV;
                } else {
                    canKeep = true;
                }
                if (canKeep && prev[j - 1] != INF) {
                    best = min(best, prev[j - 1]);
                }
            } else {
                bool canReplace;
                if (tIsNew) {
                    canReplace = !tIsV;
                } else {
                    canReplace = true;
                }
                if (canReplace && prev[j - 1] != INF) {
                    best = min(best, prev[j - 1] + 1);
                }
            }

            if (prev[j] != INF) {
                best = min(best, prev[j] + 1);
            }

            bool canInsert;
            if (tIsNew) {
                canInsert = tIsV;
            } else {
                canInsert = true;
            }
            if (canInsert && curr[j - 1] != INF) {
                best = min(best, curr[j - 1] + 1);
            }

            curr[j] = best;
        }
        swap(prev, curr);
    }

    cout << prev[m] << endl;

    return 0;
}