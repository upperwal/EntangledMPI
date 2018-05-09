#ifndef __REPMPI_H__
#define __REPMPI_H__

void rep_assign_malloc_context(const void **, const void **);

void rep_malloc(void **, size_t);
void rep_free(void **);

#endif
