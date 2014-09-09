#include <algorithm>
#include <ulib/util_log.h>
#include <colossal/fsched.hpp>

using namespace colossal;

int main()
{
        int total = 9096;

        fs_context ctx[6];

        ctx[0] = fs_context(2, 2287, 357, 0);
        ctx[1] = fs_context(2, 274, 0, 1);
        ctx[2] = fs_context(1, 274, 5, 2);
        ctx[3] = fs_context(2, 1738, 7, 3);
        ctx[4] = fs_context(6, 1830, 27921, 4);
        ctx[5] = fs_context(6, 2745, 878, 5);

        scale_minshares(ctx, ctx + 6, total);
        for (fs_context *p = ctx; p < ctx + 6; ++p) {
                ULIB_DEBUG("scaled min share=%f", p->minshare);
        }

        compute_fairshares(ctx, ctx + 6, total);

        for (fs_context *p = ctx; p < ctx + 6; ++p) {
                ULIB_DEBUG("fair share = %f", p->fairshare);
        }

        printf("Simulating allocations ...\n");
        fs_select<fs_context *> selector(ctx, ctx + 6);
        int alloc = 0;
        for (;;) {
                if (alloc >= total) {
                        printf("slots used up\n");
                        break;
                }
                fs_context *p = selector();
                if (p == ctx + 6)
                        break;
                ++alloc;
                printf("Pool %ld: d=%d, a=%d\n", p - ctx, p->demand, p->alloc);
        }

        printf("\nFinalized results:\n");
        for (int i = 0; i < 6; ++i) {
                printf("Pool %d: w=%f\tm=%f\td=%d\tr=%f\ta=%d\n",
                       i, ctx[i].weight, ctx[i].minshare, ctx[i].demand, ctx[i].fairshare, ctx[i].alloc);
        }

	printf("\nTotal users left:%lu\n", selector.user_count());

        return 0;
}
