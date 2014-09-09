//
// Create a job tracker and process randomly added jobs.
// Used for sanity check.
//

#include <time.h>
#include <colossal/colossal.hpp>

int main()
{
        colossal::job_generator gen1(
		0.002616272, 5.009914, 2.174681, 3.394791,
		2.532164, 4.070759, 6.570099, 0.9067149, 1.843567);
        gen1.seed(time(NULL));
        colossal::job_generator gen2(
		0.004299325, 2.562229, 1.038189, 2.600581,
		1.716938, 4.388895, 5.534811, 1.609197, 1.604422);
        gen2.seed(time(NULL) + 1);

	colossal::job j1 = gen1();
        colossal::job j2 = gen2();

	colossal::job_tracker jt(10000, 6000);

	colossal::pool &mod  = jt.add_pool("modeling", 1000, 1000, 1, 500, 5000, colossal::pool::SCHED_FAIR);
	colossal::pool &prod = jt.add_pool("prod",     1000, 1000, 1, 500, 5000, colossal::pool::SCHED_FAIR);

	mod.add_job(j1);
	prod.add_job(j2);

	jt.scale_minshares();
	jt.process();

	printf("------------ WORKLOAD ------------\n");
	printf("%s\n", mod.to_str().c_str());
	printf("%s\n", prod.to_str().c_str());
	printf("----------------------------------\n");

        return 0;
}
