//#include <stdio.h>

void loop(int i, int range);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg, 20);
    return 0;
}

void loop(int i, int range) {
    if (0 <= i && i <= range) {
        if (!(0 == i && i == range)) {
            if (i == range) {
                i = 0;
                range--;
            } else {
                i++;
            }
        }
        loop(i, range);
    }
}

