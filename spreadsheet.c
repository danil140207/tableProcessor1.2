#include "spreadsheet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static double parse_add_sub(const char *str, Spreadsheet *ss, int *pos, int *cycle);
static double parse_mul_div(const char *str, Spreadsheet *ss, int *pos, int *cycle);
static double parse_primary(const char *str, Spreadsheet *ss, int *pos, int *cycle);

void ss_init(Spreadsheet *ss) { memset(ss, 0, sizeof(Spreadsheet)); }

int validate_formula(const char *str) {
    int pos = 1;
    int paren_depth = 0;
    int last_was_operator = 1;
    if (str[0] != '=') return 0;
    if (str[1] == '\0') return 0;
    while (str[pos] != '\0') {
        char c = str[pos];
        if (c == ' ') { pos++; continue; }
        if (c == '(') { paren_depth++; last_was_operator = 1; pos++; continue; }
        if (c == ')') { paren_depth--; if (paren_depth < 0) return 0; last_was_operator = 0; pos++; continue; }
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            if (last_was_operator) return 0;
            last_was_operator = 1;
            pos++;
            continue;
        }
        if (c == '<' || c == '>' || c == '=') {
            if (last_was_operator && c != '-') return 0;
            if ((c == '<' && str[pos+1] == '=') || (c == '>' && str[pos+1] == '=') || (c == '=' && str[pos+1] == '=')) pos++;
            last_was_operator = 1;
            pos++;
            continue;
        }
        if (isalpha(c) && isdigit(str[pos+1])) {
            while (isalpha(str[pos]) || isdigit(str[pos])) pos++;
            last_was_operator = 0;
            continue;
        }
        if (isdigit(c) || c == '.') {
            char *end;
            strtod(&str[pos], &end);
            pos = (int)(end - str);
            last_was_operator = 0;
            continue;
        }
        return 0;
    }
    if (last_was_operator) return 0;
    if (paren_depth != 0) return 0;
    return 1;
}

int ss_load(Spreadsheet *ss, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    char line[8192];
    int row = 0, has_error = 0;
    while (fgets(line, sizeof(line), f) && row < MAX_ROWS) {
        char *ptr = line;
        int col = 0;
        char *token = strtok(ptr, ",\n\r");
        while (token && col < MAX_COLS) {
            while (*token == ' ') token++;
            if (token[0] == '=') {
                if (!validate_formula(token)) {
                    fprintf(stderr, "ERROR: Invalid formula in cell %c%d: '%s'\n", 'A'+col, row+1, token);
                    ss->cells[row][col].error = 1;
                    has_error = 1;
                }
                ss->cells[row][col].type = 2;
                strcpy(ss->cells[row][col].formula, token);
                ss->cells[row][col].value = 0;
            } else {
                char *endptr;
                double val = strtod(token, &endptr);
                if (*endptr == '\0') {
                    ss->cells[row][col].type = 1;
                    ss->cells[row][col].value = val;
                } else {
                    fprintf(stderr, "ERROR: Invalid value in cell %c%d: '%s'\n", 'A'+col, row+1, token);
                    ss->cells[row][col].error = 1;
                    has_error = 1;
                }
            }
            ss->cells[row][col].visited = 0;
            ss->cells[row][col].evaluating = 0;
            col++;
            token = strtok(NULL, ",\n\r");
        }
        if (col > ss->cols) ss->cols = col;
        row++;
    }
    ss->rows = row;
    fclose(f);
    if (has_error) return -1;
    return 0;
}

double eval(Spreadsheet *ss, int row, int col, int *cycle);

static double parse_number(const char *str, int *pos) {
    while (str[*pos] == ' ') (*pos)++;
    char *end;
    double val = strtod(&str[*pos], &end);
    *pos = (int)(end - str);
    return val;
}

static double parse_primary(const char *str, Spreadsheet *ss, int *pos, int *cycle) {
    while (str[*pos] == ' ') (*pos)++;
    if (str[*pos] == '(') {
        (*pos)++;
        double val = parse_add_sub(str, ss, pos, cycle);
        while (str[*pos] == ' ') (*pos)++;
        if (str[*pos] == ')') (*pos)++;
        return val;
    }
    if (isalpha(str[*pos]) && isdigit(str[*pos+1])) {
        int col = toupper(str[*pos]) - 'A';
        int r = atoi(&str[*pos+1]) - 1;
        double val = eval(ss, r, col, cycle);
        while (isalpha(str[*pos]) || isdigit(str[*pos])) (*pos)++;
        return val;
    }
    return parse_number(str, pos);
}

static double parse_mul_div(const char *str, Spreadsheet *ss, int *pos, int *cycle) {
    double left = parse_primary(str, ss, pos, cycle);
    if (*cycle) return 0;
    while (1) {
        while (str[*pos] == ' ') (*pos)++;
        char op = str[*pos];
        if (op == '*' || op == '/') {
            (*pos)++;
            double right = parse_primary(str, ss, pos, cycle);
            if (*cycle) return 0;
            if (op == '*') left *= right;
            else if (op == '/' && right != 0) left /= right;
            else left = 0;
        } else break;
    }
    return left;
}

static double parse_add_sub(const char *str, Spreadsheet *ss, int *pos, int *cycle) {
    double left = parse_mul_div(str, ss, pos, cycle);
    if (*cycle) return 0;
    while (1) {
        while (str[*pos] == ' ') (*pos)++;
        char op = str[*pos];
        if (op == '+' || op == '-') {
            (*pos)++;
            double right = parse_mul_div(str, ss, pos, cycle);
            if (*cycle) return 0;
            if (op == '+') left += right;
            else left -= right;
        } else break;
    }
    return left;
}

double eval(Spreadsheet *ss, int row, int col, int *cycle) {
    if (row < 0 || row >= ss->rows || col < 0 || col >= ss->cols) return 0;
    Cell *c = &ss->cells[row][col];
    if (c->error) return 0;
    if (c->type == 1) return c->value;
    if (c->type == 0) return 0;
    if (c->evaluating) { *cycle = 1; return 0; }
    if (c->visited) return c->value;
    c->evaluating = 1;
    int pos = 1;
    double res = parse_add_sub(c->formula, ss, &pos, cycle);
    c->evaluating = 0;
    c->value = res;
    c->visited = 1;
    return res;
}

int ss_evaluate(Spreadsheet *ss) {
    for (int i = 0; i < ss->rows; i++)
        for (int j = 0; j < ss->cols; j++)
            ss->cells[i][j].visited = ss->cells[i][j].evaluating = 0;
    int cycle = 0;
    for (int i = 0; i < ss->rows; i++) {
        for (int j = 0; j < ss->cols; j++) {
            if (ss->cells[i][j].type == 2 && !ss->cells[i][j].visited && !ss->cells[i][j].error) {
                eval(ss, i, j, &cycle);
                if (cycle) {
                    printf("\nERROR: Cyclic dependency detected!\n");
                    return -1;
                }
            }
        }
    }
    return 0;
}

int ss_save(Spreadsheet *ss, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    for (int i = 0; i < ss->rows; i++) {
        for (int j = 0; j < ss->cols; j++) {
            if (ss->cells[i][j].error) fprintf(f, "#ERROR!");
            else fprintf(f, "%.6g", ss->cells[i][j].value);
            if (j < ss->cols - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 0;
}
