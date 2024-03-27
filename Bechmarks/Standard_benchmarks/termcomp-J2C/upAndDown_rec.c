//#include <stdio.h>

void upAndDown(int i, int up);

int main() {
    int arg = __VERIFIER_nondet_int();
    upAndDown(arg, 0);
    return 0;
}

void upAndDown(int i, int up) {
    if (0 <= i && i <= 10) {
        if (i == 10)
            up = 0;
        if (i == 0)
            up = 1;
        if (up)
            i++;
        else
            i--;
        upAndDown(i, up);
    }
}

