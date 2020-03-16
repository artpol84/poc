#include <stdio.h>

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    struct timeval tv;
    double cur_ts;
    double diff = 0;
    double req_ts;

    if( argc < 2){
        printf("Expect timestamp on the input\n");
        exit(0);
    }
    
    sscanf(argv[1], "%lf", &req_ts);

    gettimeofday( &tv, NULL);
    cur_ts = tv.tv_sec + 1E-6*tv.tv_usec;
    FILE *fp = fopen("/dev/kmsg", "w");
    fprintf(fp, "cvt timestamp: %lf", cur_ts);
    fclose(fp);

    fp = fopen("/dev/kmsg", "w");
    fprintf(fp, "flush prev");
    fclose(fp);

    fp = popen("dmesg | tail -n 10 ", "r");
    char c[1000];
    int i = 0;
    int ret;

    while(1 == (ret = fscanf(fp, "%c", &c[i]))) {
        if( c[i] == '\n' ){
            c[i] = '\0';
            if( strstr(c, "cvt timestamp") ) {
                double dmesg_ts, gtod_ts;
                sscanf(c, "[%lf] cvt timestamp: %lf\n", &dmesg_ts, &gtod_ts);
                if( gtod_ts == cur_ts ){
                    diff = gtod_ts - dmesg_ts;
                    printf("diff = %lf\n", diff);
                }
            }
            i = 0;
        } else {
            i++;
        }
    }    
    fclose(fp);

    printf("%lf => %lf\n", req_ts, req_ts - diff);
}
