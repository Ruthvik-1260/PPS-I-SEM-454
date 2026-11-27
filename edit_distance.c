#include <stdio.h>
#include <string.h>

#define MAX 50

int min3(int a, int b, int c) {
    int m = a;
    if (b < m) m = b;
    if (c < m) m = c;
    return m;
}

int editDistance(const char *s1, const char *s2) {
    int m = strlen(s1), n = strlen(s2);
    int dp[MAX][MAX];

    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (s1[i - 1] == s2[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + min3(
                    dp[i - 1][j],        /* deletion */
                    dp[i][j - 1],        /* insertion */
                    dp[i - 1][j - 1]);   /* replacement */
        }
    }
    return dp[m][n];
}

int main(void) {
    const char *dict[] = {
        "apple", "orange", "banana", "computer", "program",
        "keyboard", "science", "network", "database", "algorithm"
    };
    int size = sizeof(dict) / sizeof(dict[0]);

    const char *tests[] = {
        "aple", "progam", "kyboard", "sceince",
        "algoritm", "databse", "compter", "banan"
    };
    int t = sizeof(tests) / sizeof(tests[0]);

    for (int k = 0; k < t; k++) {
        int best = 999, idx = 0;
        for (int i = 0; i < size; i++) {
            int d = editDistance(tests[k], dict[i]);
            if (d < best) {
                best = d;
                idx = i;
            }
        }
        printf("%s -> %s (distance = %d)\n", tests[k], dict[idx], best);
    }
    return 0;
}
