//#include <stdio.h>

void loop_aux(int i, int j);

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg);
    return 0;
}

void loop_aux(int i, int j) {
    if (i > 0) {
        if (i >= j) {
            i--;
            if (j < 5) {
                j++;
                if (i - j > 2)
                    i++;
                else
                    j++;
            } else
                j--;
        } else {
            if (i > 0 && j < 0) {
                i--;
                if (j < -1)
                    j++;
                else
                    i++;
            } else {
                i++;
                if (j * 2 > i)
                    j--;
                else
                    j++;
            }
        }
        loop_aux(i, j);
    }
}

void loop(int i) {
    loop_aux(i, i);
}

