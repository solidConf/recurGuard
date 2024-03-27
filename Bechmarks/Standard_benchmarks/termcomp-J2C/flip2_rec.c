//#include <stdio.h>

void flip(int i, int j);

int main() {
    flip(5, 10);
    return 0;
}

void flip(int i, int j) {
    int t = 0;
    if (i > 0 && j > 0) {
        if (i < j) {
            t = i;
            i = j;
            j = t;
        } else {
            if (i > j)
                j = i;
            else
                i--;
        }
        flip(i, j);
    }
}

