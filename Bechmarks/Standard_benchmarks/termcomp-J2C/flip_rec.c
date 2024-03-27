//#include <stdio.h>

void flip(int i, int j);

int main() {
int arg = __VERIFIER_nondet_int();
    flip(arg, arg+5);
    return 0;
}

void flip(int i, int j) {
	int t = 0;
	if (i != 0 && j != 0) {
	    t = i;
	    i = j;
	    j = t;
	    flip(i, j);
	}
    }


