#include "spreadsheet.h"
#include <stdio.h>
#include <string.h>


void print_help(const char *prog) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║              SPREADSHEET PROCESSOR - HELP                ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    printf("USAGE:\n");
    printf("  %s -i INPUT [-o OUTPUT] [-h]\n\n", prog);
    
    printf("OPTIONS:\n");
    printf("  -i, --input FILE   Input CSV file (required)\n");
    printf("  -o, --output FILE  Output CSV file (default: output.csv)\n");
    printf("  -h, --help         Show this help message\n\n");
    
    printf("FORMULA SYNTAX:\n");
    printf("  Arithmetic:  =A1+B2*3  (supports + - * / and parentheses)\n");
    printf("  Comparisons: =A1>B2    (returns 1 if true, 0 if false)\n");
    printf("  Supported operators: < > <= >= ==\n\n");
    
    printf("EXAMPLES:\n");
    printf("  =A1*2+5              Basic arithmetic\n");
    printf("  =(A1+B2)/C3          With parentheses\n");
    printf("  =A1>B2               Greater than (result: 1 or 0)\n");
    printf("  =A1<=B2              Less than or equal\n");
    printf("  =A1==B2              Equality check\n");
    printf("  =IF(A1>B2, A1, B2)   Conditional (advanced)\n\n");
    
    printf("LIMITS:\n");
    printf("  Maximum rows: %d\n", MAX_ROWS);
    printf("  Maximum columns: %d (A-Z)\n", MAX_COLS);
    printf("  Maximum cell length: %d characters\n\n", MAX_CELL_LEN);
}


int main(int argc, char *argv[]) {
    const char *input = NULL;
    const char *output = "output.csv";
    
   
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (++i < argc) {
                input = argv[i];
            } else {
                fprintf(stderr, "\n╔════════════════════════════════════════════╗\n");
                fprintf(stderr, "║  ERROR: Missing argument for %-10s  ║\n", argv[i-1]);
                fprintf(stderr, "╚════════════════════════════════════════════╝\n");
                fprintf(stderr, "\n  Usage: %s -i <input_file>\n", argv[0]);
                fprintf(stderr, "  Example: %s -i data.csv\n", argv[0]);
                fprintf(stderr, "  Try '%s --help' for more information.\n\n", argv[0]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (++i < argc) {
                output = argv[i];
            } else {
                fprintf(stderr, "\n╔════════════════════════════════════════════╗\n");
                fprintf(stderr, "║  ERROR: Missing argument for %-10s  ║\n", argv[i-1]);
                fprintf(stderr, "╚════════════════════════════════════════════╝\n");
                fprintf(stderr, "\n  Usage: %s -o <output_file>\n", argv[0]);
                fprintf(stderr, "  Example: %s -o result.csv\n", argv[0]);
                fprintf(stderr, "  Try '%s --help' for more information.\n\n", argv[0]);
                return 1;
            }
        }
        else {
            fprintf(stderr, "\n╔════════════════════════════════════════════╗\n");
            fprintf(stderr, "║  ERROR: Unknown option: %-12s  ║\n", argv[i]);
            fprintf(stderr, "╚════════════════════════════════════════════╝\n");
            fprintf(stderr, "\n  Valid options: -i, -o, -h, --help\n");
            fprintf(stderr, "  Example: %s -i input.csv -o output.csv\n", argv[0]);
            fprintf(stderr, "  Try '%s --help' for full documentation.\n\n", argv[0]);
            return 1;
        }
    }
    
   
    if (!input) {
        fprintf(stderr, "\n╔════════════════════════════════════════════╗\n");
        fprintf(stderr, "║  ERROR: Input file is required!            ║\n");
        fprintf(stderr, "╚════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n  Correct usage: %s -i <file.csv>\n", argv[0]);
        fprintf(stderr, "  Example: %s -i data.csv -o result.csv\n", argv[0]);
        fprintf(stderr, "  Try '%s --help' for full documentation.\n\n", argv[0]);
        return 1;
    }
    
    
    Spreadsheet ss;
    ss_init(&ss);
    
    printf("\n[LOAD] Loading %s...\n", input);
    if (ss_load(&ss, input) != 0) {
        fprintf(stderr, "\n[ERROR] Cannot read file: %s\n", input);
        fprintf(stderr, "[INFO] Check if file exists and is readable\n\n");
        return 1;
    }
    
    printf("[OK] Table loaded: %d rows, %d columns\n", ss.rows, ss.cols);
    printf("[EVAL] Evaluating formulas...\n");
    
    if (ss_evaluate(&ss) != 0) {
        return 1;
    }
    
    printf("[OK] All formulas evaluated successfully\n");
    printf("[SAVE] Saving to %s...\n", output);
    
    if (ss_save(&ss, output) != 0) {
        fprintf(stderr, "\n[ERROR] Cannot write to: %s\n", output);
        fprintf(stderr, "[INFO] Check directory permissions\n\n");
        return 1;
    }
    
    printf("[OK] Done! Output saved to: %s\n\n", output);
    return 0;
}