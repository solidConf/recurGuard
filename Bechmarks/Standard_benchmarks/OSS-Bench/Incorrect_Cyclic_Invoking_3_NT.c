#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    bool vd;
    int clm;
} QModelIndex;

void init_QModelIndex(QModelIndex* index, bool vld, int clmn) {
    index->vd = vld;
    index->clm = clmn;
}

bool isValid(const QModelIndex* index) {
    return index->vd;
}

int column(const QModelIndex* index) {
    return index->clm;
}

typedef struct {
    QModelIndex parent;
} EntityTreeModel;

void init_EntityTreeModel(EntityTreeModel* model, const QModelIndex* QL) {
    model->parent = *QL;
}

int getColumnCount(const EntityTreeModel* model);
int columnCount(const EntityTreeModel* model);

EntityTreeModel* create_EntityTreeModel(const QModelIndex* QL);
void destroy_EntityTreeModel(EntityTreeModel* model);

int getColumnCount(const EntityTreeModel* model) {
    return columnCount(model);
}

int columnCount(const EntityTreeModel* model) {
    if (isValid(&model->parent) && column(&model->parent) != 0)
        return 0;

    // Non-terminating recursion, replace this with your actual logic
    return getColumnCount(model);
}

int main() {
    QModelIndex QL;
    init_QModelIndex(&QL, true, 0);

    EntityTreeModel* (*createModelPtr)(const QModelIndex*) = &create_EntityTreeModel;
    void (*destroyModelPtr)(EntityTreeModel*) = &destroy_EntityTreeModel;

    EntityTreeModel* E1 = createModelPtr(&QL);

    // Non-terminating recursion
    getColumnCount(E1);

    destroyModelPtr(E1);

    return 0;
}

EntityTreeModel* create_EntityTreeModel(const QModelIndex* QL) {
    EntityTreeModel* model = (EntityTreeModel*)malloc(sizeof(EntityTreeModel));
    if (model) {
        init_EntityTreeModel(model, QL);
    }
    return model;
}

void destroy_EntityTreeModel(EntityTreeModel* model) {
    free(model);
}

