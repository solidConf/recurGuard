//#include <stdio.h>

int odd(int i);

int even(int i) {
    if (i == 0)
        return 1;
    else if (i == 1)
        return 0;
    else
        return odd(i - 1);
}

int odd(int i) {
    if (i == 0)
        return 0;
    else if (i == 1)
        return 1;
    else
        return even(i - 1);
}

int main() {
    int arg = __VERIFIER_nondet_int();
    even(arg - 5);
    return 0;
}

