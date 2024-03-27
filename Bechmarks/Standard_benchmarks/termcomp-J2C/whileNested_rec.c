//#include <stdio.h>

void increase_aux(int j);
void increase(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    increase(arg);
    return 0;
}

void increase_aux(int j) {
    if (j > 0)
        increase_aux(j - 1);
}

void increase(int i) {
    if (i < 10) {
        increase_aux(i);
        increase(i + 1);
    }
}

