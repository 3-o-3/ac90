#include "token.h"
#include <stdlib.h>

struct token *token__new(int type, char *value)
{
    struct token *self;
    self = malloc(sizeof(*self));
    self->type = type;
    self->value = value;
    self->next = NULL;
    return self;
}

int token__dispose(struct token *self)
{
    free(self);
    return 0;
}

