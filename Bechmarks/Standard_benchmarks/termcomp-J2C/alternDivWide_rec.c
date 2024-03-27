//#include <stdio.h>

void loop(int i, int w);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg, -5);
    return 0;
}

void loop(int i, int w) {
    if (i != 0) {
        if (i < -w) {
            loop(-1 * (i - 1), w);
        } else {
            if (i > w) {
                loop(-1 * (i + 1), w);
            } else {
                loop(0, w);
            }
        }
    }
}

