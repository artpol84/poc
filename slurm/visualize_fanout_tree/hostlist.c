#include "hostlist.h"

char *skip_spaces(char *ptr)
{
	int i = 0;
	while( ptr[i] != '\0' && ptr[i] == ' ' ){
		i++;
	}
	return (ptr + i);
}

char *find_next(char *ptr){
	int i = 0;
	ptr = skip_spaces(ptr);
	while( ptr[i] != '\0' && ptr[i] != ' ' ){
		i++;
	}

	return ptr + i;
}

int hostlist_count(char *hl)
{
	char *ptr = hl;
	int count = 0;
	
	while( ptr[0] != '\0' ){
		char *ptr_new = find_next(ptr);
		if( ptr_new != ptr ){
			count++;
		}
		ptr = ptr_new;
	}
	return count;
}

char *hostlist_shift(char *hl, char **name)
{
	*name = skip_spaces(hl);
	hl = find_next(hl);
    if( hl[0] == ' ' ){
        hl[0] = '\0';
        hl++;
    }
    return hl;
}
