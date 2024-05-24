#include "log.h"

void write_log(char* msg) {
    /* Writes msg to file at LOGPATH */
    FILE *f = fopen(LOGPATH, "a");
    file_write_datetime(f);
    fprintf(f, "%s\n\0", msg);
    fclose(f);
}


// remake for gui case 
/*
int print_log() {
     
    //    Prints logfile from LOGPATH
     //   returns 0 on succ, 1 on fopen err
    
    FILE *f;
    char *s;
    size_t size;
    if (!(f = fopen(LOGPATH, "r"))) {
        fprintf(stderr, "Unable to open file");
        return 1;
    }
    size = get_file_len(f);
    //printf("%d\n", size);
    s = malloc(size + 1);
    //printf("Mallocd\n");
    fread(s, 1, size, f);
    //printf("Read\n");
    s[size] = '\0';
    printf("LEN: %d\n", strlen(s));
    printf("%s\n", s);
    fclose(f);
    free(s);
    return 0;
}
*/