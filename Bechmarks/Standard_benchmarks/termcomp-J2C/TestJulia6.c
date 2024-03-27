//#include <stdio.h>

struct TestJulia6 {
    struct TestJulia6* f;
};

static struct TestJulia6* last = NULL;

static void init(struct TestJulia6* this) {
    last = this;
}

void iter(struct TestJulia6* this);

int main() {
    struct TestJulia6 l;
    init(&l);
    l.f = last;
    iter(&l);
    return 0;
}

void iter(struct TestJulia6* this) {
    if (this->f != NULL)
        iter(this->f);
}

