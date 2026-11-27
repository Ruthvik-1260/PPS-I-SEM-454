/*
 * Q5: Seat reservation with replaceable input policy (campus shuttle)
 *
 * Seat rows/columns are zero-based internally (8 x 8). All out-of-range
 * coordinates are rejected. Fare and acceptance are returned through
 * OUTPUT PARAMETERS; there is no global state.
 *
 * STRUCTURE CHART
 *
 *                          main / caller
 *                               |
 *                          attempt()                 (prints result, snapshot check)
 *                               |
 *                          reserveSeat()             CORE: never changes with policy
 *                  +------------+-------------+
 *                  |                          |
 *          policy->toIndex()          policy->fare()     <- replaceable policy
 *        (input interpretation)        (fare rule)
 *                  |
 *   oldToIndex (zero-based)   newToIndex (one-based)
 *
 * Interface:
 *   void reserveSeat(int occ[ROWS][COLS], const Policy *p,
 *                    int inRow, int inCol, int *fare, int *accepted);
 *
 * Rule enforced: the matrix is written in exactly ONE place, the last
 * statement before success. Every rejection returns earlier, so on rejection
 * the occupancy matrix stays unchanged. A later module needing a different
 * input policy supplies a new Policy; reserveSeat() is not edited.
 *
 * Compile : gcc q5_seat_reservation.c -o q5
 * Run     : ./q5          (choose policy, enter seats)
 *           ./q5 test     (regression suite)
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define ROWS 8
#define COLS 8

typedef struct {
    const char *name;
    int (*toIndex)(int inRow, int inCol, int *row, int *col);  /* 1 ok, 0 reject */
    int (*fare)(int row, int col);
} Policy;

/* ---- OLD policy: coordinates are zero-based, flat fare ---- */
static int oldToIndex(int inRow, int inCol, int *row, int *col)
{
    if (inRow < 0 || inRow >= ROWS || inCol < 0 || inCol >= COLS) return 0;
    *row = inRow; *col = inCol;
    return 1;
}
static int oldFare(int row, int col) { (void)row; (void)col; return 30; }

/* ---- NEW policy: coordinates are one-based, front two rows cost 5 more ---- */
static int newToIndex(int inRow, int inCol, int *row, int *col)
{
    if (inRow < 1 || inRow > ROWS || inCol < 1 || inCol > COLS) return 0;  /* check before subtracting */
    *row = inRow - 1; *col = inCol - 1;
    return 1;
}
static int newFare(int row, int col) { (void)col; return row < 2 ? 35 : 30; }

static const Policy OLD_P = { "old (zero-based)", oldToIndex, oldFare };
static const Policy NEW_P = { "new (one-based)",  newToIndex, newFare };

/* CORE computation: independent of which policy is plugged in. */
void reserveSeat(int occ[ROWS][COLS], const Policy *p,
                 int inRow, int inCol, int *fare, int *accepted)
{
    int row, col;
    *fare = 0;
    *accepted = 0;

    if (!p->toIndex(inRow, inCol, &row, &col)) return;               /* policy rejects */
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return;    /* core guard     */
    if (occ[row][col] != 0) return;                                  /* already taken  */

    *fare = p->fare(row, col);
    occ[row][col] = 1;          /* the only write to the matrix */
    *accepted = 1;
}

static void printMatrix(int occ[ROWS][COLS])
{
    for (int r = 0; r < ROWS; r++) {
        printf("  ");
        for (int c = 0; c < COLS; c++) printf("%d ", occ[r][c]);
        printf("\n");
    }
}

/* ---------------- Regression suite ---------------- */
static int failures = 0;
static void check(const char *name, int cond)
{
    printf("[%s] %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond) failures++;
}

/* The SAME caller exercises either policy. Returns 1 if accepted. */
static int attempt(int occ[ROWS][COLS], const Policy *p, int r, int c, int *fare, int *unchangedOnReject)
{
    int snap[ROWS][COLS], accepted;
    memcpy(snap, occ, sizeof snap);
    reserveSeat(occ, p, r, c, fare, &accepted);
    *unchangedOnReject = accepted ? 1 : (memcmp(snap, occ, sizeof snap) == 0);
    return accepted;
}

void runTests(void)
{
    int fare, same, acc;

    /* old policy through the shared caller */
    int occOld[ROWS][COLS] = {{0}};
    acc = attempt(occOld, &OLD_P, 0, 0, &fare, &same);
    check("old: (0,0) accepted, fare 30, cell set", acc && fare == 30 && occOld[0][0] == 1);
    acc = attempt(occOld, &OLD_P, 7, 7, &fare, &same);
    check("old: (7,7) accepted (last valid seat)", acc && occOld[7][7] == 1);
    acc = attempt(occOld, &OLD_P, 8, 0, &fare, &same);
    check("old: row 8 rejected, matrix unchanged, fare 0", !acc && same && fare == 0);
    acc = attempt(occOld, &OLD_P, 0, 8, &fare, &same);
    check("old: column 8 rejected, matrix unchanged", !acc && same);
    acc = attempt(occOld, &OLD_P, -1, 3, &fare, &same);
    check("old: negative row rejected, matrix unchanged", !acc && same);
    acc = attempt(occOld, &OLD_P, INT_MIN, INT_MAX, &fare, &same);
    check("old: extreme values rejected, matrix unchanged", !acc && same);
    acc = attempt(occOld, &OLD_P, 0, 0, &fare, &same);
    check("old: already occupied seat rejected, matrix unchanged", !acc && same && fare == 0);

    /* new policy through the SAME caller */
    int occNew[ROWS][COLS] = {{0}};
    acc = attempt(occNew, &NEW_P, 1, 1, &fare, &same);
    check("new: (1,1) maps to cell (0,0), front-row fare 35", acc && fare == 35 && occNew[0][0] == 1);
    acc = attempt(occNew, &NEW_P, 8, 8, &fare, &same);
    check("new: (8,8) maps to cell (7,7), fare 30", acc && fare == 30 && occNew[7][7] == 1);
    acc = attempt(occNew, &NEW_P, 0, 5, &fare, &same);
    check("new: row 0 is out of range, matrix unchanged", !acc && same);
    acc = attempt(occNew, &NEW_P, 9, 1, &fare, &same);
    check("new: row 9 rejected, matrix unchanged", !acc && same);
    acc = attempt(occNew, &NEW_P, INT_MIN, 1, &fare, &same);
    check("new: INT_MIN rejected without overflow, matrix unchanged", !acc && same);

    /* repeated calls do not interfere: no globals, outputs reset every call */
    int occR[ROWS][COLS] = {{0}};
    acc = attempt(occR, &OLD_P, 2, 2, &fare, &same);
    int f1 = fare;
    acc = attempt(occR, &OLD_P, 99, 99, &fare, &same);
    check("repeated calls: rejected call resets fare output to 0", !acc && fare == 0 && f1 == 30);
    check("repeated calls: earlier booking still intact", occR[2][2] == 1);

    printf("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        runTests();
        return failures ? 1 : 0;
    }

    int occ[ROWS][COLS] = {{0}}, choice;
    printf("Policy: 1 = old (zero-based seats), 2 = new (one-based seats): ");
    if (scanf("%d", &choice) != 1 || (choice != 1 && choice != 2)) { printf("Invalid.\n"); return 1; }
    const Policy *p = (choice == 1) ? &OLD_P : &NEW_P;
    printf("Using %s policy. Enter 'row col' per seat, anything else to quit.\n", p->name);

    for (;;) {
        int r, c, fare, accepted;
        printf("Seat: ");
        if (scanf("%d %d", &r, &c) != 2) break;
        reserveSeat(occ, p, r, c, &fare, &accepted);
        if (accepted) printf("  Accepted, fare %d\n", fare);
        else          printf("  Rejected, matrix unchanged\n");
    }
    printf("Occupancy matrix:\n");
    printMatrix(occ);
    return 0;
}
