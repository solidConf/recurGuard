//#include <stdio.h>

void loop(int i, int w);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg, 1);
    return 0;
}

void loop(int i, int w) {
    if (i != 0) {
        if (i < -w) {
            loop(-1 * (i - 1), w + 1);
        } else {
            if (i > w) {
                loop(-1 * (i + 1), w + 1);
            } else {
                loop(0, w + 1);
            }
        }
    }
}

