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


void set_span(int total,  int *span, int tree_width)
{
	int left = total;
	int i = 0;

	for(i = 0; i< tree_width; i++){
		span[i] = 0;
	}

	if (total <= tree_width) {
		return;
	}

	while (left > 0) {
		for(i = 0; i < tree_width; i++) {
			if ((tree_width-i) >= left) {
				if (span[i] == 0) {
					left = 0;
					break;
				} else {
					span[i] += left;
					left = 0;
					break;
				}
			} else if (left <= tree_width) {
				if( span[i] == 0 ){
					left--;
				}
				span[i] += left;
				left = 0;

				break;
			}
			if( span[i] == 0 ){
				left--;
			}
			span[i] += tree_width;
			left -= tree_width;
		}
	}
}


void start_msg_tree(char *hl, int tree_width, int depth)
{
	int span[100];
	int j = 0, count = 0;
	char *name = NULL;
	int thr_count = 0;
	int host_count = 0;
    char *launcher;

    depth++;
	hl = hostlist_shift(hl, &name);
	count = hostlist_count(hl);
    if( count == 0 ){
        if( depth < min_deep ){
            min_deep = depth;
        }
        if( depth > max_deep ){
            max_deep = depth;
        }
        return;
    }	
	
    launcher = name;

	set_span(count, span, tree_width);
    printf("%s span: ", name);
    for(j=0;j<tree_width;j++){
        printf("%d ",span[j]);
    }
    printf("\n");

	hl = skip_spaces(hl);
	while ( hl[0] != '\0' ){
        char *hl_child = malloc(strlen(hl)+1);
        hl_child[0] = '\0';
        hl = hostlist_shift(hl, &name);
        strcat(hl_child, name);

        if( out_dot ){
            add_launch_dot(buf, launcher, hl_child);
        } else {
            add_launch_std(buf, launcher, hl_child);
        }

		for (j = 0; j < span[thr_count]; j++) {
			hl = hostlist_shift(hl, &name);
			if ( name[0] == '\0' )
				break;
            strcat(hl_child, " ");
			strcat(hl_child, name);
		}
		start_msg_tree(hl_child, tree_width, depth);
        free(hl_child);
		thr_count++;
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
        output_dot("slurm_my.dot", hl, buf);
    } else {
        printf("%s", buf );
    }

    printf("max_deep = %u, min_deep = %u\n", max_deep, min_deep);

    return 0;
}
