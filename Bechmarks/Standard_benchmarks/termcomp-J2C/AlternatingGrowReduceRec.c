//#include <stdio.h>
//#include <stdlib.h>

struct AlternatingGrowReduceRec {
    struct AlternatingGrowReduceRec* next;
};

void growReduce(int mode, struct AlternatingGrowReduceRec* list);

struct AlternatingGrowReduceRec* createList(int length);

int main(int argc, char* argv[]) {
    growReduce(0, createList(atoi(argv[1])));
    return 0;
}

void growReduce(int mode, struct AlternatingGrowReduceRec* list) {
    if (list == NULL)
        return;

    if (mode == 0) {
        list = list->next;
    } else if (mode == 1) {
        list = (struct AlternatingGrowReduceRec*)malloc(sizeof(struct AlternatingGrowReduceRec));
        list->next = NULL;
    } else if (mode > 1) {
        list = (struct AlternatingGrowReduceRec*)malloc(sizeof(struct AlternatingGrowReduceRec));
        list->next = (struct AlternatingGrowReduceRec*)malloc(sizeof(struct AlternatingGrowReduceRec));
        list->next->next = NULL;
    }

    mode++;
    if (mode > 2) {
        growReduce(0, list);
    } else {
        growReduce(mode, list);
    }
}

struct AlternatingGrowReduceRec* createList(int length) {
    struct AlternatingGrowReduceRec* res = (struct AlternatingGrowReduceRec*)malloc(sizeof(struct AlternatingGrowReduceRec));
    res->next = NULL;

    if (length > 1) {
        res->next = createList(length - 1);
    }

    return res;
}

