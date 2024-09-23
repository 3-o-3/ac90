
#include "ac90.h"

struct hash_elem *hash_elem__new(char *name, int len)
{
    int s;
    struct hash_elem *self;

    s = sizeof(*self) + len;
    self = malloc(s);
    memset(self, 0, s);
    if (name) {
    	memcpy(self->buf, name, len);
    	self->buf[len] = '\0';
    	hash_elem__init(self, self->buf, len);
    	self->name = self->buf;
    }
    return self;
}

int hash_elem__init(struct hash_elem *self, char *name, int len)
{
    self->name = name;
    self->name_len = len;
    self->hash = hash_elem__hash(name, len);
    return 0;
}

int hash_elem__dispose(struct hash_elem *self)
{
    free(self);
    return 0;
}

int hash_elem__hash(char *name, int len)
{
    int h;
    int i;

    h = 0;
    for (i = 0; i < len; i++)
    {
	if (name[i] >= '0' && name[i] <= '9') {
		h = (h * 10) + name[i] - '0';
	} else {
        	h ^= (h << 4) + name[i];
	}
    }
    return h;
}

struct hash_table *hash_table__new(int size)
{
    struct hash_table *self;
    int i;
    int as;

    i = 1;
    while (i < size)
    {
        i <<= 1;
    }
    size = i;
    as = sizeof(*self) + (sizeof(self->elem[0]) * size);
    self = malloc(as);
    memset(self, 0, as);
    self->size = size;
    return self;
}

int hash_table__dispose(struct hash_table *self)
{
    int i;
    struct hash_elem *s;

    for (i = 0; i < self->size; i++)
    {
        s = self->elem[i];
        while (s)
        {
            hash_elem__dispose(s);
            s = s->next;
        }
    }
    free(self);
    return 0;
}

int hash_table__add(struct hash_table *self, struct hash_elem *elem)
{
    int c;
    int len = elem->name_len;
    char *name = elem->name;
    int hash = elem->hash;
    int mask = self->size - 1;
    struct hash_elem *s;
    struct hash_elem *p;
    s = self->elem[hash & mask];

    if (!s)
    {
        self->elem[hash & mask] = elem;
        return 0;
    }
    if (s->name_len == len)
    {
        c = strncmp(s->name, name, len);
        if (c > 0)
        {
            self->elem[hash & mask] = elem;
            elem->next = s;
            return 0;
        }
        else if (c == 0)
        {
            return -1;
        }
    }
    p = s;
    s = s->next;
    while (s)
    {
        if (s->name_len == len)
        {
            c = strncmp(s->name, name, len);
            if (c > 0)
            {
                elem->next = p->next;
                p->next = elem;
                return 0;
            }
            else if (c == 0)
            {
                return -1;
            }
        }
        p = s;
        s = s->next;
    }
    elem->next = NULL;
    p->next = elem;
    return 0;
}

struct hash_elem *hash_table__get(struct hash_table *self, 
		int hash, char *name, int len)
{
    int c;
    int mask = self->size - 1;
    struct hash_elem *s;

    s = self->elem[hash & mask];
    if (!s)
    {
        return NULL;
    }
    while (s)
    {
        if (s->name_len == len)
        {
            c = strncmp(s->name, name, len);
            if (c > 0)
            {
                return NULL;
            }
            else if (c == 0)
            {
                break;
            }
        }
        s = s->next;
    }
    return s;
}

int hash_table__foreach(struct hash_table *self,
		int (*cb)(const void*, const void*, void*), void *arg)
{
    int i;
    struct hash_elem *s;

    for (i = 0; i < self->size; i++)
    {
        s = self->elem[i];
        while (s)
        {
	    if (cb(s, NULL, arg)) {
		return -1;
            }
            s = s->next;
        }
    }

    return 0;
}

