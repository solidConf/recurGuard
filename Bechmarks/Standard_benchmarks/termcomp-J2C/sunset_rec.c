//#include <stdio.h>

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg + 20);
    return 0;
}

void loop(int i) {
    if (i > 10) {
        if (i == 25) {
            i = 30;
        }
        if (i <= 30) {
            i--;
        } else {
            i = 20;
        }
        loop(i);
    }
}

