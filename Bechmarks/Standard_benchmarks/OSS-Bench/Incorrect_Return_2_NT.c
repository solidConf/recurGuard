#include <stdio.h>
#include <string.h>

typedef void (*LoadPageTreeFunction)(char*, char*);

void loadpagetree(char* obj, char* kobj, LoadPageTreeFunction indirect_loadpagetree);

void loadpagetree_recursive(char* obj, char* kobj, LoadPageTreeFunction indirect_loadpagetree) {
    char* type = obj;
    int kids = strlen(obj);

    if (strcmp(type, "Page") != 0) {
        for (int i = 0; i < kids; i++) {
            indirect_loadpagetree(obj, kobj);
        }
    }
}

int main() {
    char obj[5];
    char kobj[5];

    for (int i = 0; i < 4; i++) {
        // Assuming you have a way to provide non-deterministic characters
        obj[i] = __VERIFIER_nondet_char();
        kobj[i] = __VERIFIER_nondet_char();
    }

    obj[4] = '\0';
    kobj[4] = '\0';

    LoadPageTreeFunction indirect_loadpagetree = loadpagetree_recursive;
    indirect_loadpagetree(obj, kobj);

    return 0;
}

