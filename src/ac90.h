
#ifndef AC90_H_
#define AC90_H_

#ifdef _WIN32
#else
#include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define bp() __builtin_debugtrap()

#define _(a) (a)

#include "buf.h"
#include "hash.h"

int txt__ends_with(char *end, char *str);

#endif /* AC90_H_ */

