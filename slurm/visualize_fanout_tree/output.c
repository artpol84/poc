#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "output.h"
#include "hostlist.h"


void add_launch_std(char *buf, char *who, char *child_list)
{
    sprintf(buf,"%s%s: will launch %s\n",buf, who, child_list);

}

void add_launch_dot(char *buf, char *who, char *child_list)
{
    char *hl = strdup(child_list);
    char *name;

    while ( hl[0] != '\0' ){
        hl = hostlist_shift(hl, &name);
        sprintf(buf,"%s%s -> %s\n",buf,who,name);
    }
}

void output_dot(char *fname, char *hl, char *buf)
{
    FILE *f = fopen(fname, "w" );
    fprintf(f, "digraph launch {\n");
    hl = skip_spaces(hl);
    while ( hl[0] != '\0' ){
        char *name;
        hl = hostlist_shift(hl, &name);
        fprintf(f,"%s [height=0.4 width=0.6 fixedsize=true style=filled fillcolor=white]\n", name);
    }
    fprintf(f, "%s\n", buf );
    fprintf(f, "}\n" );
    fclose(f);
}
