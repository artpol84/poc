#include <string>
#include <stdio.h>

class test {
private:
    test *next;
public:
    test() {
	printf("Init test!!\n");
        next = NULL;
    }
};

class test2 {
private:
public:
    test2() {
	printf("Init test 2!!\n");
    }
};


class test3 : public test, test2 {
private:
    int completed;
public:

    test3() : test(), test2() {
	completed = 0;
    }
};

int main()
{
    void *ptr = calloc(sizeof(test3), 1);
    test3 *t = new(ptr) test3;
    

}
