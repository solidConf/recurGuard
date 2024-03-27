//#include <stdio.h>

int middle(int i, int j);

int main() {
    int arg = __VERIFIER_nondet_int();
    middle(arg, arg + 5);
    return 0;
}

int middle(int i, int j) {
    if (i != j) {
        i--;
        j++;
        return middle(i, j);
    }
    return i;
}

