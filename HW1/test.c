#include <stdio.h>

#ifdef H1
    void f() {
        printf("hi1\n");
    }
#endif

#ifdef H2
    void f() {
        printf("hi2\n");
    }
#endif

int main() {
    f();
}