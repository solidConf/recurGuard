//#include <stdio.h>

int sum(int n);

int main() {
    sum(-1);
    return 0;
}

int sum(int n) {
    if (n == 0)
        return 0;
    else
        return n + sum(n - 1);
}

