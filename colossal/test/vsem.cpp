#include <stdio.h>
#include <colossal/vsem.hpp>

using namespace colossal;

struct obj;

vsem<obj *> sem(2);

struct obj {
        int i;

        bool operator()(engine *jt)
        {
		(void) jt;
		if (sem.wait(this)) {
			printf("%d\n", i);
			return true;
		}
		return false;
        }

        obj(int id) : i(id) { }
};



int main()
{
        obj *a1 = new obj(1);
        obj *a2 = new obj(2);
        obj *a3 = new obj(3);
        obj *a4 = new obj(4);

	if ((*a1)(0))
		delete a1;
	if ((*a2)(0))
		delete a2;
	if ((*a3)(0))
		delete a3;
	if ((*a4)(0))
		delete a4;

	getchar();

	sem.post(NULL);
	sem.post(NULL);
	sem.post(NULL);
	sem.post(NULL);

        return 0;
}
