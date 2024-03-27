//#include <stdio.h>

void loop(int i) {
    if (i > 0) {
        if (i != 5)
            i--;
        loop(i);
    }
}

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg);
    return 0;
}
