#include <stdio.h>                                                                                                                                                                                                                            
#include <unistd.h>                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                              
int main() {                                                                                                                                                                                                                                  
    int fd[2];     

    printf("__USE_FORTIFY_LEVEL %d\n", __USE_FORTIFY_LEVEL);
#if defined _FORTIFY_SOURCE
    printf("_FORTIFY_SOURCE %d\n", _FORTIFY_SOURCE);
#endif
                                                                                                                                                                                                                                          
    pipe(fd);                                                                                                                                                                                                                                 
                                                                                                                                                                                                                                          
    return 0;                                                                                                                                                                                                                                 
}  