//#include <stdio.h>

void loop(int i, int j);

int main() {
    loop(2, 0);
    return 0;
}

void loop(int i, int j) {
    if (i < j) {
        i = i + 4;
    } else {
        j = j + 1;
        i = i + 2;
    }
    loop(i, j);
}

