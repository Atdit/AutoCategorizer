#ifndef AUTOCATEGORIZER_AUTO_CATEGORIZER_H
#define AUTOCATEGORIZER_AUTO_CATEGORIZER_H

#define print_error_line(format, args...) do { \
    fprintf(stderr, "%s:%d in %s: Error: ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(stderr, format, ##args); \
    fprintf(stderr, "\n"); \
} while (0)

#define print_error_line_q(format, args...) do { \
    fprintf(stderr, "%s:%d in %s: Error: ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(stderr, format, ##args); \
    fprintf(stderr, "\n"); \
    exit(EXIT_FAILURE); \
} while (0)

#endif //AUTOCATEGORIZER_AUTO_CATEGORIZER_H
