
#ifndef HASH_H_
#define HASH_H_


struct hash_elem {
    int hash;
    struct hash_elem *next;
    void *value;
    int name_len;
    char *name;
    char buf[1];
};

struct hash_table {
    int size;
    struct hash_elem *elem[1];
};

struct hash_elem *hash_elem__new(char *name, int len);
int hash_elem__init(struct hash_elem *self, char *name, int len);
int hash_elem__dispose(struct hash_elem *self);
int hash_elem__hash(char *name, int len);


struct hash_table *hash_table__new(int size);
int hash_table__dispose(struct hash_table *self);
int hash_table__add(struct hash_table *self, struct hash_elem *elem);
struct hash_elem *hash_table__get(struct hash_table *self, 
		int hash, char *name, int len);
int hash_table__foreach(struct hash_table *self, 
		int (*cb)(const void*, const void*, void*), void *arg);

#endif /* HASH_H_ */
