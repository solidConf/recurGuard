//#include <stdio.h>

int sum(int n);

int main() {
    int arg = __VERIFIER_nondet_int();
    int n = arg;
    if (arg < 5) {
        n *= -1;
    }
    sum(n);
    return 0;
}

int sum(int n) {
    if (n == 0) {
        return 0;
    } else {
        return n + sum(n - 1);
    }
}

