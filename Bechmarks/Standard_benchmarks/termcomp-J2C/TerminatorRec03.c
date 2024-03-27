//#include <stdio.h>

void f(int x);

int main() {
    int arg = __VERIFIER_nondet_int();
    f(arg);
    return 0;
}

void f(int x) {
    if (x == 0) {
        f(1);
    } else {
        f(0);
    }
}

