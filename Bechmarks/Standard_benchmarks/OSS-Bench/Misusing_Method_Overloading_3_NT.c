#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* trigger;
} Rule;

Rule Rule_Create(const char* trigger) {
    Rule rule;
    rule.trigger = trigger;
    return rule;
}

Rule Rule_Copy(const Rule* other) {
    Rule rule;
    rule.trigger = other->trigger;
    return rule;
}

// Non-terminating recursive function
void induce(Rule* self) {
    printf("Trigger: %s\n", self->trigger);

    // Recursive call without termination condition
    induce(self);
}

int main() {
    const char* test = "test";

    Rule rule = Rule_Create(test);

    // Call the induce method
    induce(&rule);

    return 0;
}

