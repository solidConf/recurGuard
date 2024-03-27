//#include <stdio.h>

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg + 10);
    return 0;
}

void loop(int i) {
    if (i > 0 && i < 50) {
        if (i < 20) {
            i--;
        }
        if (i > 10) {
            i++;
        }
        if (30 <= i && i <= 40) {
            i--;
        }
        loop(i);
    }
}

