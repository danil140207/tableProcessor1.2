#include "spreadsheet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

void ss_init(Spreadsheet *ss) { memset(ss, 0, sizeof(Spreadsheet)); }

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
                    ss->cells[row][col].type = 0;
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

double parse_expression(const char *str, Spreadsheet *ss, int *pos, int *cycle) {
    double left = 0;
    char op = '+';
    int need_val = 1;
    
    while (1) {
        while (str[*pos] == ' ') (*pos)++;
        if (str[*pos] == '\0') break;
        
        double val = 0;
        
        if (str[*pos] == '(') {
            (*pos)++;
            val = parse_expression(str, ss, pos, cycle);
            while (str[*pos] == ' ') (*pos)++;
            if (str[*pos] == ')') (*pos)++;
        }
        else if (isalpha(str[*pos]) && isdigit(str[*pos+1])) {
            int c = toupper(str[*pos]) - 'A';
            int r = atoi(&str[*pos+1]) - 1;
            val = eval(ss, r, c, cycle);
            while (isalpha(str[*pos]) || isdigit(str[*pos])) (*pos)++;
        }
        else {
            char *end;
            val = strtod(&str[*pos], &end);
            *pos = (int)(end - str);
        }
        
        if (op == '+') left += val;
        else if (op == '-') left -= val;
        else if (op == '*') left *= val;
        else if (op == '/') left /= val;
        
        while (str[*pos] == ' ') (*pos)++;
        if (str[*pos] == '+' || str[*pos] == '-' || str[*pos] == '*' || str[*pos] == '/') {
            op = str[*pos];
            (*pos)++;
        } else break;
    }
    return left;
}

double parse_comparison(const char *str, Spreadsheet *ss, int *pos, int *cycle) {
    double left = parse_expression(str, ss, pos, cycle);
    if (*cycle) return 0;
    
    while (1) {
        while (str[*pos] == ' ') (*pos)++;
        char op1 = str[*pos];
        char op2 = str[*pos+1];
        int cmp = 0;
        
        if (op1 == '<' && op2 == '=') { cmp = 3; *pos += 2; }
        else if (op1 == '>' && op2 == '=') { cmp = 4; *pos += 2; }
        else if (op1 == '=' && op2 == '=') { cmp = 5; *pos += 2; }
        else if (op1 == '<') { cmp = 1; (*pos)++; }
        else if (op1 == '>') { cmp = 2; (*pos)++; }
        else break;
        
        double right = parse_expression(str, ss, pos, cycle);
        if (*cycle) return 0;
        
        int res = 0;
        if (cmp == 1) res = (left < right);
        else if (cmp == 2) res = (left > right);
        else if (cmp == 3) res = (left <= right);
        else if (cmp == 4) res = (left >= right);
        else if (cmp == 5) res = (left == right);
        
        left = (double)res;
    }
    return left;
}

double eval(Spreadsheet *ss, int row, int col, int *cycle) {
    if (row < 0 || row >= ss->rows || col < 0 || col >= ss->cols) return 0;
    Cell *c = &ss->cells[row][col];
    if (c->type == 1) return c->value;
    if (c->type == 0) return 0;
    if (c->evaluating) { *cycle = 1; return 0; }
    if (c->visited) return c->value;
    
    c->evaluating = 1;
    int pos = 1;
    double res = parse_comparison(c->formula, ss, &pos, cycle);
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
            if (ss->cells[i][j].type == 2 && !ss->cells[i][j].visited) {
                eval(ss, i, j, &cycle);
                if (cycle) {
                    printf("\n╔════════════════════════════════════════════╗\n");
                    printf("║  ERROR: Cyclic dependency detected!        ║\n");
                    printf("╚════════════════════════════════════════════╝\n");
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
            fprintf(f, "%.6g", ss->cells[i][j].value);
            if (j < ss->cols - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 0;
}