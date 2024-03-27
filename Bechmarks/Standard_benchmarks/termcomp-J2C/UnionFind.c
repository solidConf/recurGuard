//#include <stdio.h>
//#include <stdlib.h>

struct UnionFind {
    struct UnionFind* parent;
};

void makeSet(struct UnionFind* uf);

struct UnionFind* find(struct UnionFind* uf);

void unionSets(struct UnionFind* x, struct UnionFind* y);

int main(int argc, char* argv[]) {
    struct UnionFind y;
    y.parent = NULL;
    makeSet(&y);

    for (int i = 0; i < atoi(argv[1]); ++i) {
        struct UnionFind x;
        x.parent = NULL;
        makeSet(&x);
        unionSets(&x, &y);
    }

    return 0;
}

void makeSet(struct UnionFind* uf) {
    uf->parent = uf;
}

struct UnionFind* find(struct UnionFind* uf) {
    if (uf->parent == uf) {
        return uf;
    } else {
        uf->parent = find(uf->parent);  // Path compression
        return uf->parent;
    }
}

void unionSets(struct UnionFind* x, struct UnionFind* y) {
    struct UnionFind* xRoot = find(x);
    struct UnionFind* yRoot = find(y);
    xRoot->parent = yRoot;
}

