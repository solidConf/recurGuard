//#include <stdio.h>

void loop_aux(int i, int range, int up);

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg + 15);
    return 0;
}

void loop_aux(int i, int range, int up) {
    if (0 <= i && i <= range) {
        if (i == 0) up = 1;
        if (i == range) up = 0;
        if (up) i++;
        if (!up) i--;
        if (i == range - 2) range--;
        loop_aux(i, range, up);
    }
}

void loop(int i) {
    loop_aux(i, 20, 0);
}

