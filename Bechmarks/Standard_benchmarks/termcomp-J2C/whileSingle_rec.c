//#include <stdio.h>

void increase(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    increase(arg);
    return 0;
}

void increase(int i) {
    while (i < 10) {
        if (i != 3) {
            i++;
        }
        increase(i);
    }
}

