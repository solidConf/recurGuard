//#include <stdio.h>

void loop_aux(int i, int range);

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg);
    return 0;
}

void loop_aux(int i, int range) {
    if (-range <= i && i <= range) {
        if (range - i < 5 || range + i < 5)
            i = i * (-1);
        else {
            range++;
            i--;
            if (i == 0) range = -1;
        }
        loop_aux(i, range);
    }
}

void loop(int i) {
    loop_aux(i, 20);
}

