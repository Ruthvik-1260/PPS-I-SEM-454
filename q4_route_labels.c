/*
 * Q4: Route labels such as "East Gate 2" in a fixed 32-byte buffer
 *
 * - Each label buffer is 32 bytes including the terminator (max 31 characters).
 * - Exact matches are case-sensitive and use the ORIGINAL input line
 *   (identity is compared BEFORE normalization).
 * - A normalized DISPLAY copy is built separately: leading/trailing blanks
 *   removed, runs of blanks collapsed to one space. The original is never
 *   overwritten.
 * - Input is read as a whole line (not %s), so "East Gate 2" and "East Gate 3"
 *   (labels that differ only after the first embedded space) stay distinct.
 * - A line longer than 31 characters is rejected, never silently truncated.
 *
 * What the original must retain so that distinctness stays checkable:
 * every character exactly as typed (case, inner/extra spaces), minus only the
 * line terminator.
 *
 * Compile : gcc q4_route_labels.c -o q4
 * Run     : ./q4          (enter labels, then search)
 *           ./q4 test     (regression suite)
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define LABEL_SIZE 32
#define MAXLABELS  10

enum { LBL_OK = 0, LBL_BLANK, LBL_TOO_LONG, LBL_EOF };

/* Reads one line into orig (original text, newline removed). */
int captureLabel(FILE *in, char orig[LABEL_SIZE])
{
    orig[0] = '\0';
    if (fgets(orig, LABEL_SIZE, in) == NULL) { orig[0] = '\0'; return LBL_EOF; }

    size_t len = strlen(orig);
    if (len > 0 && orig[len - 1] == '\n') {
        orig[--len] = '\0';
    } else if (!feof(in)) {                 /* buffer full without newline */
        int c = fgetc(in);
        if (c != EOF && c != '\n') {        /* more text follows: too long */
            while (c != EOF && c != '\n') c = fgetc(in);   /* discard the rest */
            orig[0] = '\0';
            return LBL_TOO_LONG;
        }
    }
    while (len > 0 && orig[len - 1] == '\r') orig[--len] = '\0';

    for (size_t i = 0; i < len; i++)
        if (!isspace((unsigned char)orig[i])) return LBL_OK;
    return LBL_BLANK;                       /* empty or only blanks */
}

/* Builds the display copy. orig is const: it can never be overwritten. */
size_t normalizeLabel(const char *orig, char disp[LABEL_SIZE])
{
    size_t o = 0;
    int pendingSpace = 0;

    for (const char *p = orig; *p; p++) {
        if (isspace((unsigned char)*p)) {
            if (o > 0) pendingSpace = 1;    /* leading blanks are dropped */
            continue;
        }
        size_t need = pendingSpace ? 2 : 1;
        if (o + need > LABEL_SIZE - 1) break;   /* bounded: keep room for '\0' */
        if (pendingSpace) { disp[o++] = ' '; pendingSpace = 0; }
        disp[o++] = *p;
    }
    disp[o] = '\0';                         /* trailing blanks never added */
    return o;
}

/* Exact, case-sensitive search on the ORIGINAL labels. */
int searchLabel(const char labels[][LABEL_SIZE], int n, const char key[])
{
    for (int i = 0; i < n; i++)
        if (strcmp(labels[i], key) == 0) return i;
    return -1;
}

/* ---------------- Regression suite ---------------- */
static int failures = 0;
static void check(const char *name, int cond)
{
    printf("[%s] %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond) failures++;
}

/* Feeds text to captureLabel through a temporary file. */
static int feed(const char *text, char out[LABEL_SIZE])
{
    FILE *f = tmpfile();
    if (!f) return LBL_EOF;
    fputs(text, f);
    rewind(f);
    int st = captureLabel(f, out);
    fclose(f);
    return st;
}

void runTests(void)
{
    char orig[LABEL_SIZE], disp[LABEL_SIZE];
    int st;

    st = feed("\n", orig);
    check("blank line (newline only) -> BLANK", st == LBL_BLANK);
    st = feed("    \n", orig);
    check("blank line (spaces only) -> BLANK", st == LBL_BLANK);
    st = feed("", orig);
    check("no input at all -> EOF", st == LBL_EOF);

    st = feed("   East Gate 2\n", orig);
    normalizeLabel(orig, disp);
    check("leading spaces: original keeps them", st == LBL_OK && strcmp(orig, "   East Gate 2") == 0);
    check("leading spaces: display copy is trimmed", strcmp(disp, "East Gate 2") == 0);

    char full31[40], over32[40];
    memset(full31, 'A', 31); strcpy(full31 + 31, "\n");
    memset(over32, 'A', 32); strcpy(over32 + 32, "\n");
    st = feed(full31, orig);
    size_t dl = normalizeLabel(orig, disp);
    check("full capacity (31 chars) accepted intact", st == LBL_OK && strlen(orig) == 31 && dl == 31);
    st = feed(over32, orig);
    check("32 chars rejected (no silent truncation)", st == LBL_TOO_LONG);
    st = feed("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", orig);     /* 31 chars, no newline, EOF */
    check("31 chars without newline at EOF accepted", st == LBL_OK && strlen(orig) == 31);

    st = feed("East Gate 2\n", orig);
    check("trailing newline removed from original", st == LBL_OK && strcmp(orig, "East Gate 2") == 0);
    st = feed("East Gate 2\r\n", orig);
    check("trailing CR+LF removed from original", st == LBL_OK && strcmp(orig, "East Gate 2") == 0);

    /* Labels differing only after the first embedded space stay distinct */
    char labels[MAXLABELS][LABEL_SIZE];
    strcpy(labels[0], "East Gate 2");
    strcpy(labels[1], "East Gate 3");
    strcpy(labels[2], "East");
    check("'East Gate 3' found at index 1, not confused with 'East Gate 2'",
          searchLabel(labels, 3, "East Gate 3") == 1);
    check("'East Gate 2' found at index 0", searchLabel(labels, 3, "East Gate 2") == 0);
    check("'East' (prefix before first space) is its own label", searchLabel(labels, 3, "East") == 2);
    check("search is case-sensitive", searchLabel(labels, 3, "east gate 2") == -1);

    /* Collision: normalization BEFORE comparison would merge two identities */
    char a[LABEL_SIZE] = "East  Gate 2", b[LABEL_SIZE] = "East Gate 2";
    char da[LABEL_SIZE], db[LABEL_SIZE];
    normalizeLabel(a, da); normalizeLabel(b, db);
    check("collision built: display copies are identical", strcmp(da, db) == 0);
    check("originals still distinct, so compare identity BEFORE normalizing", strcmp(a, b) != 0);

    /* Original is not overwritten by normalization */
    char keep[LABEL_SIZE], copy[LABEL_SIZE];
    strcpy(keep, "  East   Gate 2  ");
    strcpy(copy, keep);
    normalizeLabel(keep, disp);
    check("normalization leaves the original byte-for-byte unchanged", strcmp(keep, copy) == 0);
    check("display copy is 'East Gate 2'", strcmp(disp, "East Gate 2") == 0);

    printf("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        runTests();
        return failures ? 1 : 0;
    }

    char labels[MAXLABELS][LABEL_SIZE], line[64], disp[LABEL_SIZE], key[LABEL_SIZE];
    int n = 0, stored = 0;

    printf("How many labels (1 to %d)? ", MAXLABELS);
    if (!fgets(line, sizeof line, stdin) || sscanf(line, "%d", &n) != 1 || n < 1 || n > MAXLABELS) {
        printf("Invalid count.\n");
        return 1;
    }

    while (stored < n) {
        printf("Label %d: ", stored);
        int st = captureLabel(stdin, labels[stored]);
        if (st == LBL_EOF) break;
        if (st == LBL_BLANK)    { printf("  Blank label rejected.\n"); continue; }
        if (st == LBL_TOO_LONG) { printf("  Longer than %d characters, rejected.\n", LABEL_SIZE - 1); continue; }
        stored++;
    }

    printf("\nStored labels (original | display copy):\n");
    for (int i = 0; i < stored; i++) {
        normalizeLabel(labels[i], disp);
        printf("  [%d] |%s| |%s|\n", i, labels[i], disp);
    }

    for (;;) {
        printf("\nSearch label (blank to stop): ");
        int st = captureLabel(stdin, key);
        if (st == LBL_EOF || st == LBL_BLANK) break;
        if (st == LBL_TOO_LONG) { printf("  Too long, cannot match any label.\n"); continue; }
        int idx = searchLabel((const char (*)[LABEL_SIZE])labels, stored, key);
        if (idx >= 0) printf("  Exact match at index %d\n", idx);
        else          printf("  No exact match\n");
    }
    return 0;
}
