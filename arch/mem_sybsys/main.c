#include <mem_sys.h>
#include <sys/types.h>


int main()
{
    int nlevels;
    size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX];

    discover_caches(&nlevels, cache_sizes);

    return 0;
}
