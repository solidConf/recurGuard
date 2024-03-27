//#include <stdlib.h>

struct TestJulia7 {
    struct TestJulia7 *f;
};

void test(struct TestJulia7 *tj1, struct TestJulia7 *tj2) {
    tj1->f = tj2->f = (struct TestJulia7 *)malloc(sizeof(struct TestJulia7));
}

void loop(struct TestJulia7 *t) {
    if (t->f != NULL) {
        loop(t->f);
    }
}

int main() {
    struct TestJulia7 tj1, tj2;
    test(&tj1, &tj2);
    tj1.f->f = &tj2;
    loop(&tj1);
    
    // Free allocated memory (optional)
    free(tj1.f->f);
    free(tj1.f);
    
    return 0;
}

