//#include <stdio.h>

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg + 28);
    return 0;
}

void loop(int i) {
    if (i != 0) {
        if (-5 <= i && i <= 35) {
            if (i < 0) {
                i = -5;
            } else {
                if (i > 30) {
                    i = 35;
                } else {
                    i--;
                }
            }
        } else {
            i = 0;
        }
        loop(i);
    }
}

