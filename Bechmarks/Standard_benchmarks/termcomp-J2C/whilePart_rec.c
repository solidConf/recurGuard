//#include <stdio.h>

void increase(int i);

int main() {
    int arg = __VERIFIER_nondet_int();
    increase(arg + 5);
    return 0;
}

void increase(int i) {
    if (i > 5&&i<20) {
    	i++;
        if (i < 10) {
            i--;
        }
        increase(i);
    }
}

