//#include <stdio.h>

void loop(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    loop(arg);
    return 0;
}

void loop(int i) {
    if (i > 5) {
        if (i != 10)
            i--;
        loop(i);
    }
}

