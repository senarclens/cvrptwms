#include <stdio.h>
#include <stdlib.h>

#include "wrappers.h"

void *safe_malloc_(size_t size, const char *filename, int line)
{
  void *ptr = malloc(size);

  if (ptr == NULL)
  {
    fprintf(stderr, "malloc %lu bytes failed at %s:%d\n",
                    (unsigned long)size, filename, line);
    exit(EXIT_FAILURE);
  }
  return ptr;
}
