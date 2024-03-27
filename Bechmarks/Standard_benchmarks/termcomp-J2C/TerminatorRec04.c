//#include <stdio.h>

void f(int x);

int main() {
    int arg = __VERIFIER_nondet_int();
    int x = arg;
    while (x > 0) {
        f(x);
        x = x + 1;
    }
    return 0;
}

void f(int x) {
    if (x < 0) {
        f(x - 1);
    }
    return;
}

