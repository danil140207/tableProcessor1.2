#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#define MAX_ROWS 50
#define MAX_COLS 26
#define MAX_CELL_LEN 256

typedef struct {
    int type;
    double value;
    char formula[MAX_CELL_LEN];
    int visited;
    int evaluating;
    int error;
    char error_msg[100];
} Cell;

typedef struct {
    Cell cells[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Spreadsheet;

void ss_init(Spreadsheet *ss);
int ss_load(Spreadsheet *ss, const char *filename);
int ss_save(Spreadsheet *ss, const char *filename);
int ss_evaluate(Spreadsheet *ss);

#endif