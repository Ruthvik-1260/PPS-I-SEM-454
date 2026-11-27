/*
 * Q2: Rider fare with a policy change applied midway (campus shuttle)
 *
 * Fare: 20 for 0-5 km, 35 for >5-15 km, 55 beyond 15 km.
 *       Peak adds 10, student status (S) subtracts 5.
 * Riders are counted until an end marker (a negative distance).
 *
 * Policy versions (chosen per record from its index):
 *   OLD  (index <  k): any status other than 'S' is charged as a normal rider.
 *   NEW  (index >= k): status must be 'S' or 'N'. Anything else is REJECTED
 *                      before any money is accumulated.
 *
 * Earlier results are never revised: every decision is stored in an audit log
 * together with the policy version that produced it.
 *
 * Compile : gcc q2_rider_fare.c -o q2
 * Run     : ./q2          (enter data)
 *           ./q2 test     (regression suite)
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAXREC 100
#define OLD_POLICY 1
#define NEW_POLICY 2

typedef struct {
    double km;
    char status;
    int peak;
    int policy;     /* version used for this record  */
    int accepted;   /* 1 = charged, 0 = rejected     */
    int fare;       /* 0 when rejected               */
} Decision;

typedef struct {
    int count;              /* accepted riders          */
    int total;              /* money accumulated        */
    int n;                  /* records seen (index)     */
    Decision log[MAXREC];   /* per-record audit trail   */
} Session;

int baseFare(double km)
{
    if (km <= 5.0)  return 20;
    if (km <= 15.0) return 35;
    return 55;
}

int policyFor(int index, int k)
{
    return (index >= k) ? NEW_POLICY : OLD_POLICY;
}

/* Pure function: decides one record, touches no totals. */
int computeFare(double km, char status, int peak, int policy, int *accepted)
{
    if (policy == NEW_POLICY && status != 'S' && status != 'N') {
        *accepted = 0;          /* rejected BEFORE any money is accumulated */
        return 0;
    }
    int fare = baseFare(km);
    if (peak)            fare += 10;
    if (status == 'S')   fare -= 5;
    *accepted = 1;
    return fare;
}

/* Returns the record index, or -1 if the audit log is full. */
int processRecord(Session *s, int k, double km, char status, int peak)
{
    if (s->n >= MAXREC) return -1;

    int idx = s->n;
    Decision *d = &s->log[idx];
    d->km = km; d->status = status; d->peak = peak;
    d->policy = policyFor(idx, k);
    d->fare = computeFare(km, status, peak, d->policy, &d->accepted);

    if (d->accepted) {          /* totals change only for accepted records */
        s->count++;
        s->total += d->fare;
    }
    s->n++;
    return idx;
}

/* Count and total must always match the accepted records in the log. */
int invariantsHold(const Session *s)
{
    int c = 0, t = 0;
    for (int i = 0; i < s->n; i++)
        if (s->log[i].accepted) { c++; t += s->log[i].fare; }
    return c == s->count && t == s->total;
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
    int acc;

    /* Fare table and boundaries */
    check("5 km normal = 20",            computeFare(5.0,  'N', 0, OLD_POLICY, &acc) == 20);
    check("5.1 km normal = 35",          computeFare(5.1,  'N', 0, OLD_POLICY, &acc) == 35);
    check("15 km normal = 35",           computeFare(15.0, 'N', 0, OLD_POLICY, &acc) == 35);
    check("15.1 km normal = 55",         computeFare(15.1, 'N', 0, OLD_POLICY, &acc) == 55);
    check("student + peak, 3 km = 25",   computeFare(3.0,  'S', 1, OLD_POLICY, &acc) == 25);

    /* Policy changes between two consecutive records (k = 1) */
    Session a = {0};
    processRecord(&a, 1, 3.0, 'X', 0);      /* index 0 -> OLD: accepted */
    processRecord(&a, 1, 3.0, 'X', 0);      /* index 1 -> NEW: rejected */
    check("record before k accepted under old policy", a.log[0].accepted == 1 && a.log[0].fare == 20);
    check("record at k rejected under new policy",     a.log[1].accepted == 0 && a.log[1].fare == 0);
    check("earlier result not revised",                a.log[0].accepted == 1 && a.log[0].policy == OLD_POLICY);
    check("count/total ignore the rejected record",    a.count == 1 && a.total == 20);
    check("invariants hold after rejection (new)",     invariantsHold(&a));

    /* Paired streams: identical records, only the rule differs */
    Session without = {0}, with = {0};
    double km[]   = {3.0, 10.0, 20.0};
    char   st[]   = {'S', 'X',  'N'};
    int    pk[]   = {0,   1,    0};
    for (int i = 0; i < 3; i++) {
        processRecord(&without, 100, km[i], st[i], pk[i]);   /* rule never enforced */
        processRecord(&with,      0, km[i], st[i], pk[i]);   /* rule enforced from 0 */
    }
    check("stream without rule: count 3, total 115", without.count == 3 && without.total == 115);
    check("stream with rule:    count 2, total 70",  with.count == 2 && with.total == 70);
    check("difference equals the unknown-status fare (45)", without.total - with.total == 45);
    check("invariants hold in both streams", invariantsHold(&without) && invariantsHold(&with));

    /* Audit log vs final-total-only cache */
    check("log shows WHICH record was rejected (index 1)",
          with.log[0].accepted && !with.log[1].accepted && with.log[2].accepted);

    printf("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        runTests();
        return failures ? 1 : 0;
    }

    Session s = {0};
    int k;
    printf("Enter k (record index from which the NEW policy applies): ");
    if (scanf("%d", &k) != 1 || k < 0) { printf("Invalid k.\n"); return 1; }

    for (;;) {
        double km; char st; int pk;
        printf("Record %d - distance km (negative = end), status (S/N), peak (0/1): ", s.n);
        if (scanf("%lf", &km) != 1 || km < 0) break;           /* end marker */
        if (scanf(" %c %d", &st, &pk) != 2) break;
        if (pk != 0 && pk != 1) { printf("  Peak must be 0 or 1, record ignored.\n"); continue; }
        st = (char)toupper((unsigned char)st);

        int idx = processRecord(&s, k, km, st, pk);
        if (idx < 0) { printf("  Log full.\n"); break; }
        const Decision *d = &s.log[idx];
        if (d->accepted) printf("  Accepted: fare %d (policy %s)\n", d->fare, d->policy == NEW_POLICY ? "NEW" : "OLD");
        else             printf("  REJECTED: unknown status '%c' (policy NEW)\n", d->status);
    }

    printf("\nAudit log:\n");
    for (int i = 0; i < s.n; i++) {
        const Decision *d = &s.log[i];
        printf("  [%d] %.1f km, status %c, peak %d, policy %s -> %s %d\n", i, d->km, d->status, d->peak,
               d->policy == NEW_POLICY ? "NEW" : "OLD", d->accepted ? "fare" : "rejected", d->fare);
    }
    printf("Riders counted: %d, total collected: %d\n", s.count, s.total);
    return 0;
}
