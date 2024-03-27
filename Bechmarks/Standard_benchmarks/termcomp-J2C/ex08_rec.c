//#include <stdio.h>

void loop_aux(int i, int up);
void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(10 * arg);
    return 0;
}

void loop_aux(int i, int up) {
    if (i > 0) {
        if (i == 1) up = 1;
        if (i == 10) up = 0;
        if (up) i++;
        else i--;
        loop_aux(i, up);
    }
}

void loop(int i) {
    loop_aux(i, 0);
}
