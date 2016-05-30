#include <stdio.h>

void sort(int size, int *array);

int main()
{
    int array[] = { 5, 1, 3, 8, 3, 2, 10 };
    int size = sizeof(array) / sizeof(array[0]);
    int i;

    sort(size, array);

    printf("Result: ");
    for(i = 0; i < size; i++){
        printf("%d ", array[i]);
    }
    printf("\n");
    return 0;
}
