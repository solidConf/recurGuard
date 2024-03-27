//#include <stdio.h>

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg);
    return 0;
}

void loop(int i) {
    if (i != 0) {
        if (i > -5 && i < 5) {
            if (i < 0) {
                i++;
            }
            if (i > 0) {
                i--;
            }
        }
        loop(i);
    }
}

