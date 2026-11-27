/*
 * Q3: Bus-by-seat occupancy matrix, spare seats, first three ranks
 *
 * Data (parallel by bus index):  occ[bus][seat]  (0 free, 1 occupied, max 8x8)
 *                                routeId[bus]    (integer route ID)
 *                                cap[bus]        (seats in service, 0..8)
 *
 * Eligible bus : routeId > 0 (in service) AND spare seats > 0.
 * Rank order   : 1) more spare seats first
 *                2) tie -> smaller route ID first
 *                3) tie -> smaller bus index (original position) first
 *
 * The three ranks come from three repeated selection passes over the ORIGINAL
 * arrays; nothing is sorted or moved, so every unselected record stays intact
 * and each rank is the bus INDEX (identity), not just a key value.
 *
 * Cancelling a booking frees exactly the cell (bus, seat) that was booked.
 *
 * Compile : gcc q3_seat_ranking.c -o q3
 * Run     : ./q3          (enter data)
 *           ./q3 test     (regression suite)
 */
#include <stdio.h>
#include <string.h>

#define MAXBUS 8
#define SEATS  8
#define TOP    3

void computeSpare(const int occ[MAXBUS][SEATS], const int cap[], int nb, int spare[])
{
    for (int b = 0; b < nb; b++) {
        spare[b] = 0;
        for (int s = 0; s < cap[b] && s < SEATS; s++)
            if (occ[b][s] == 0) spare[b]++;
    }
}

static int eligible(int routeId, int spare) { return routeId > 0 && spare > 0; }

/* 1 if bus a ranks ahead of bus b (documented tie rule). */
static int better(int a, int b, const int spare[], const int routeId[])
{
    if (spare[a] != spare[b])     return spare[a] > spare[b];
    if (routeId[a] != routeId[b]) return routeId[a] < routeId[b];
    return a < b;
}

/* Fills ranks[] with up to TOP bus indices; returns how many were found. */
int topRanks(const int occ[MAXBUS][SEATS], const int routeId[], const int cap[],
             int nb, int ranks[TOP])
{
    int spare[MAXBUS], taken[MAXBUS] = {0}, found = 0;
    computeSpare(occ, cap, nb, spare);

    for (int pass = 0; pass < TOP; pass++) {
        int best = -1;
        for (int b = 0; b < nb; b++) {
            if (taken[b] || !eligible(routeId[b], spare[b])) continue;
            if (best < 0 || better(b, best, spare, routeId)) best = b;
        }
        if (best < 0) break;            /* fewer than TOP eligible buses */
        ranks[found++] = best;
        taken[best] = 1;                /* flag only; the data is not touched */
    }
    return found;
}

/* 0 = freed, -1 = out of range, -2 = that seat was not occupied. */
int cancelBooking(int occ[MAXBUS][SEATS], const int cap[], int nb, int bus, int seat)
{
    if (bus < 0 || bus >= nb || seat < 0 || seat >= cap[bus] || seat >= SEATS) return -1;
    if (occ[bus][seat] == 0) return -2;
    occ[bus][seat] = 0;                 /* exactly the original seat */
    return 0;
}

static void report(const int occ[MAXBUS][SEATS], const int routeId[], const int cap[], int nb)
{
    int spare[MAXBUS], ranks[TOP];
    computeSpare(occ, cap, nb, spare);
    for (int b = 0; b < nb; b++)
        printf("  bus %d: route %d, capacity %d, spare %d%s\n", b, routeId[b], cap[b], spare[b],
               eligible(routeId[b], spare[b]) ? "" : "  (not eligible)");
    int found = topRanks(occ, routeId, cap, nb, ranks);
    printf("Top %d ranks:\n", TOP);
    for (int r = 0; r < found; r++)
        printf("  rank %d: bus %d (route %d, spare %d)\n", r + 1, ranks[r], routeId[ranks[r]], spare[ranks[r]]);
    if (found == 0) printf("  no eligible bus\n");
}

/* ---------------- Regression suite ---------------- */
static int failures = 0;
static void check(const char *name, int cond)
{
    printf("[%s] %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond) failures++;
}

static void occupy(int occ[MAXBUS][SEATS], int bus, int start, int count)
{
    for (int s = start; s < start + count; s++) occ[bus][s] = 1;
}

void runTests(void)
{
    int occ[MAXBUS][SEATS] = {{0}};
    int routeId[MAXBUS] = {20, 10, 30, 0, 40, 15};
    int cap[MAXBUS]     = { 8,  8,  8, 8,  6,  8};
    int nb = 6, ranks[TOP];

    occupy(occ, 0, 0, 3);   /* bus0 spare 5 */
    occupy(occ, 1, 3, 3);   /* bus1 spare 5 */
    occupy(occ, 2, 0, 8);   /* bus2 full -> excluded            */
                            /* bus3 route 0 (inactive) -> excluded despite 8 spare */
    occupy(occ, 4, 0, 2);   /* bus4 spare 4 */
    occupy(occ, 5, 0, 1);   /* bus5 spare 5 */
    occupy(occ, 5, 6, 2);

    int occCopy[MAXBUS][SEATS], routeCopy[MAXBUS], capCopy[MAXBUS];
    memcpy(occCopy, occ, sizeof occ);
    memcpy(routeCopy, routeId, sizeof routeId);
    memcpy(capCopy, cap, sizeof cap);

    int found = topRanks(occ, routeId, cap, nb, ranks);
    printf("Equal keys (spare 5) on buses 0,1,5 -> rank order: bus %d, bus %d, bus %d\n",
           ranks[0], ranks[1], ranks[2]);
    check("three ranks found", found == 3);
    check("tie rule: spare 5 ties ordered by route ID (10, 15, 20) -> buses 1,5,0",
          ranks[0] == 1 && ranks[1] == 5 && ranks[2] == 0);
    check("full bus and inactive route are excluded",
          ranks[0] != 2 && ranks[1] != 2 && ranks[2] != 2 && ranks[0] != 3 && ranks[1] != 3 && ranks[2] != 3);
    check("unselected records preserved (matrix, routes, capacities unchanged)",
          memcmp(occ, occCopy, sizeof occ) == 0 && memcmp(routeId, routeCopy, sizeof routeId) == 0 &&
          memcmp(cap, capCopy, sizeof cap) == 0);

    found = topRanks(occ, routeId, cap, 3, ranks);      /* buses 0,1,2 only */
    check("fewer than three eligible -> only 2 ranks (buses 1, 0)", found == 2 && ranks[0] == 1 && ranks[1] == 0);

    /* equal spare AND equal route ID -> smaller bus index first */
    int occ2[MAXBUS][SEATS] = {{0}};
    int route2[MAXBUS] = {5, 5}, cap2[MAXBUS] = {4, 4};
    found = topRanks(occ2, route2, cap2, 2, ranks);
    check("same spare and same route ID -> lower bus index first", found == 2 && ranks[0] == 0 && ranks[1] == 1);

    /* cancellation frees exactly the original seat */
    int before[MAXBUS][SEATS], spareB[MAXBUS], spareA[MAXBUS];
    memcpy(before, occ, sizeof occ);
    computeSpare(occ, cap, nb, spareB);
    int rc = cancelBooking(occ, cap, nb, 0, 1);          /* bus0 seat1 was occupied */
    computeSpare(occ, cap, nb, spareA);
    int diff = 0;
    for (int b = 0; b < MAXBUS; b++)
        for (int s = 0; s < SEATS; s++)
            if (occ[b][s] != before[b][s]) diff++;
    check("cancel returns success", rc == 0);
    check("cancel changes exactly one cell, bus0 seat1: 1 -> 0",
          diff == 1 && before[0][1] == 1 && occ[0][1] == 0);
    check("spare seats of that bus +1, others unchanged",
          spareA[0] == spareB[0] + 1 && spareA[1] == spareB[1] && spareA[5] == spareB[5]);

    memcpy(before, occ, sizeof occ);
    check("cancelling a free seat is refused (-2)", cancelBooking(occ, cap, nb, 0, 1) == -2);
    check("cancelling out-of-range bus/seat is refused (-1)",
          cancelBooking(occ, cap, nb, 9, 0) == -1 && cancelBooking(occ, cap, nb, 0, -1) == -1 &&
          cancelBooking(occ, cap, nb, 4, 7) == -1);
    check("refused cancellations leave the matrix unchanged", memcmp(before, occ, sizeof occ) == 0);

    printf("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        runTests();
        return failures ? 1 : 0;
    }

    int occ[MAXBUS][SEATS] = {{0}}, routeId[MAXBUS] = {0}, cap[MAXBUS] = {0}, nb;

    printf("Number of buses (1 to %d): ", MAXBUS);
    if (scanf("%d", &nb) != 1 || nb < 1 || nb > MAXBUS) { printf("Invalid.\n"); return 1; }
    for (int b = 0; b < nb; b++) {
        printf("Bus %d - route ID and capacity (0-%d): ", b, SEATS);
        if (scanf("%d %d", &routeId[b], &cap[b]) != 2 || cap[b] < 0 || cap[b] > SEATS) { printf("Invalid.\n"); return 1; }
        printf("  %d seat values (0 free, 1 occupied): ", cap[b]);
        for (int s = 0; s < cap[b]; s++)
            if (scanf("%d", &occ[b][s]) != 1 || (occ[b][s] != 0 && occ[b][s] != 1)) { printf("Invalid.\n"); return 1; }
    }

    char cmd;
    printf("\nCommands: r = report, c <bus> <seat> = cancel booking, q = quit\n");
    while (printf("> "), scanf(" %c", &cmd) == 1 && cmd != 'q') {
        if (cmd == 'r') {
            report(occ, routeId, cap, nb);
        } else if (cmd == 'c') {
            int bus, seat;
            if (scanf("%d %d", &bus, &seat) != 2) { printf("Invalid.\n"); break; }
            int rc = cancelBooking(occ, cap, nb, bus, seat);
            if (rc == 0)       printf("Seat %d of bus %d freed.\n", seat, bus);
            else if (rc == -1) printf("Out of range.\n");
            else               printf("That seat was not occupied; nothing changed.\n");
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
