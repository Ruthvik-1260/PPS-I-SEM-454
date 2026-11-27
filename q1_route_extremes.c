/*
 * Q1: Shortest and longest observed route duration (campus shuttle)
 *
 * Rules implemented
 *  - Durations are positive minutes; -1 = cancelled route and must NOT participate.
 *  - Zero valid routes is allowed (no extreme is reported, no forbidden answer).
 *  - The original record order is never modified (no sorting / no reordering),
 *    so the audit trail stays intact.
 *  - Each extreme reports: duration, route number and original position (index).
 *  - Tie rule: the EARLIEST position in the original order wins (strict < and >).
 *
 * Compile : gcc q1_route_extremes.c -o q1
 * Run     : ./q1          (enter data)
 *           ./q1 test     (run regression suite)
 */
#include <stdio.h>
#include <string.h>

#define MAX 100
#define CANCELLED (-1)

typedef struct {
    int valid;        /* number of participating routes            */
    int cancelled;    /* number of -1 records skipped              */
    int rejected;     /* other invalid values (0, < -1) skipped    */
    int minDur, minRoute, minPos;
    int maxDur, maxRoute, maxPos;
} Result;

/* Reads arrays only (const) -> original order and values stay untouched. */
void findExtremes(const int route[], const int dur[], int n, Result *r)
{
    r->valid = r->cancelled = r->rejected = 0;
    r->minDur = r->minRoute = r->minPos = -1;
    r->maxDur = r->maxRoute = r->maxPos = -1;

    for (int i = 0; i < n; i++) {
        if (dur[i] == CANCELLED) { r->cancelled++; continue; }  /* must not participate */
        if (dur[i] <= 0)         { r->rejected++;  continue; }  /* not a positive minute */

        if (r->valid == 0) {                       /* first admissible record */
            r->minDur = r->maxDur = dur[i];
            r->minRoute = r->maxRoute = route[i];
            r->minPos = r->maxPos = i;
        } else {
            if (dur[i] < r->minDur) {              /* strict: earliest tie wins */
                r->minDur = dur[i]; r->minRoute = route[i]; r->minPos = i;
            }
            if (dur[i] > r->maxDur) {
                r->maxDur = dur[i]; r->maxRoute = route[i]; r->maxPos = i;
            }
        }
        r->valid++;
    }
}

void printResult(const Result *r)
{
    printf("Valid routes: %d, cancelled(-1): %d, other rejected: %d\n",
           r->valid, r->cancelled, r->rejected);
    if (r->valid == 0) {
        printf("No valid route: shortest/longest not defined.\n");
        return;
    }
    printf("Shortest: %d min | route %d | position %d\n", r->minDur, r->minRoute, r->minPos);
    printf("Longest : %d min | route %d | position %d\n", r->maxDur, r->maxRoute, r->maxPos);
}

/* ---------------- Regression suite ---------------- */
static int failures = 0;

static void check(const char *name, int cond)
{
    printf("[%s] %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond) failures++;
}

void runTests(void)
{
    Result r;

    /* T1: -1 must not become the shortest */
    int r1[] = {11, 12, 13}, d1[] = {25, -1, 40};
    findExtremes(r1, d1, 3, &r);
    check("T1 cancelled (-1) is not the minimum", r.minDur == 25 && r.minRoute == 11 && r.minPos == 0);
    check("T1 maximum correct", r.maxDur == 40 && r.maxRoute == 13 && r.maxPos == 2);

    /* T2: all cancelled -> zero valid routes, no forbidden answer */
    int r2[] = {1, 2}, d2[] = {-1, -1};
    findExtremes(r2, d2, 2, &r);
    check("T2 all cancelled -> valid = 0", r.valid == 0 && r.cancelled == 2);

    /* T3: empty input */
    findExtremes(r2, d2, 0, &r);
    check("T3 empty input -> valid = 0", r.valid == 0);

    /* T4: tie rule, earliest position wins */
    int r4[] = {7, 8, 9}, d4[] = {30, 30, 30};
    findExtremes(r4, d4, 3, &r);
    check("T4 tie -> earliest position (min and max)", r.minPos == 0 && r.maxPos == 0);

    /* T5: shortcut "reordering cannot change the extreme position" is FALSE */
    int ra[] = {7, 8, 9}, da[] = {30, 20, 20};      /* original order */
    int rb[] = {9, 8, 7}, db[] = {20, 20, 30};      /* reordered copy */
    Result ro, rr;
    findExtremes(ra, da, 3, &ro);
    findExtremes(rb, db, 3, &rr);
    check("T5 same minimum value after reorder", ro.minDur == rr.minDur);
    check("T5 position changes after reorder (shortcut is false)", ro.minPos != rr.minPos);
    check("T5 route number changes after reorder", ro.minRoute != rr.minRoute);

    /* T6: input arrays are not modified (audit trail preserved) */
    int r6[] = {1, 2, 3, 4}, d6[] = {50, -1, 10, 70};
    int r6c[4], d6c[4];
    memcpy(r6c, r6, sizeof r6); memcpy(d6c, d6, sizeof d6);
    findExtremes(r6, d6, 4, &r);
    check("T6 original order untouched",
          memcmp(r6, r6c, sizeof r6) == 0 && memcmp(d6, d6c, sizeof d6) == 0);
    check("T6 result: min 10 @2 route 3, max 70 @3 route 4",
          r.minDur == 10 && r.minPos == 2 && r.minRoute == 3 &&
          r.maxDur == 70 && r.maxPos == 3 && r.maxRoute == 4);

    printf("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        runTests();
        return failures ? 1 : 0;
    }

    int route[MAX], dur[MAX], n;

    printf("Enter number of records (0 to %d): ", MAX);
    if (scanf("%d", &n) != 1 || n < 0 || n > MAX) {
        printf("Invalid count.\n");
        return 1;
    }
    for (int i = 0; i < n; i++) {
        printf("Record %d - route number and duration (-1 = cancelled): ", i);
        if (scanf("%d %d", &route[i], &dur[i]) != 2) {
            printf("Invalid record.\n");
            return 1;
        }
    }

    printf("\nOriginal records (audit trail):\n");
    for (int i = 0; i < n; i++)
        printf("  [%d] route %d, duration %d\n", i, route[i], dur[i]);

    Result res;
    findExtremes(route, dur, n, &res);
    printf("\n");
    printResult(&res);
    return 0;
}
