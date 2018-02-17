#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "output.h"
#include "hostlist.h"

int out_dot = 1;

char buf[10*1024*1024] = "";
char hl[10*1024*1024] = "";
unsigned int max_deep = 0, min_deep = ~0;


void start_msg_tree(char *hl, int tree_width, int depth)
{
	int span[100];
	int j = 0, count = 0;
	char *name = NULL;
	int thr_count = 0;
	int host_count = 0;
    char *launcher;
    char *launcher_names[1024];
    int launcher_work[1024];
    int launcher_cnt;

    hl = hostlist_shift(hl, &name);
    launcher_names[0] = name;
    launcher_work[0] = 0;
    launcher_cnt++;
    while( hl[0] ) {
        int assign = 0, i;
        for(i=1; i<launcher_cnt; i++){
            if( launcher_work[assign] - (i - assign) > launcher_work[i]) {
                assign = i;
            }
        }
        for(i=0; i<tree_width; i++) {
            hl = hostlist_shift(hl, &name);
            launcher_names[launcher_cnt] = name;
            launcher_work[launcher_cnt++] = 0;
            if( out_dot ){
                add_launch_dot(buf, launcher_names[assign], name);
            } else {
                add_launch_std(buf, launcher_names[assign], name);
            }
            launcher_work[assign]++;
        }
    }

	return;
}

int main(int argc, char**argv)
{
	int count = 20;
	int tree_width = 50;
    int i;
	
    if( argc > 1 ){
        count = atoi(argv[1]);
    }
    if( argc > 2 ){
        tree_width = atoi(argv[2]);
    }

    printf("count=%d, width=%d\n",count, tree_width);

	for(i=0;i<count;i++){
		sprintf(hl,"%s cn%d",hl,i+1);
	}
	start_msg_tree(hl,tree_width,0);	

    if( out_dot ){
        for(i=0;i<count;i++){
            sprintf(hl,"%s cn%d",hl,i+1);
        }
        output_dot("knomial.dot", hl, buf);
    } else {
        printf("%s", buf );
    }

    printf("max_deep = %u, min_deep = %u\n", max_deep, min_deep);

    return 0;
}
