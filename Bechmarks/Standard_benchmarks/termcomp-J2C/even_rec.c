//#include <stdio.h>

int even(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    even(-1 * arg);
    return 0;
}

int even(int i) {
    if (i == 0) {
        return 1; // true
    } else if (i == 1) {
        return 0; // false
    } else {
        return even(i - 2);
    }
}

