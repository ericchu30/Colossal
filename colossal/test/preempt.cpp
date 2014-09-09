#include <time.h>
#include <colossal/colossal.hpp>

int main()
{
	colossal::job j1;
        colossal::job j2;

	colossal::task t1;
	t1.id = 1;
	t1.ctime = 0;
	t1.ptime = 3;
	t1.stime = -1;
	t1.ftime = -1;
	t1.type = colossal::task::TASK_TYPE_MAP;


	colossal::task t2;
	t2.id = 2;
	t2.ctime = 0;
	t2.ptime = 3;
	t2.stime = -1;
	t2.ftime = -1;
	t2.type = colossal::task::TASK_TYPE_MAP;

	colossal::task t3;
	t3.id = 3;
	t3.ctime = 1;
	t3.ptime = 2;
	t3.stime = -1;
	t3.ftime = -1;
	t3.type = colossal::task::TASK_TYPE_MAP;

	j1.id = 1;
	j1.tasks[colossal::task::TASK_TYPE_MAP].push_back(t1);
	j1.tasks[colossal::task::TASK_TYPE_MAP].push_back(t2);

	j2.id = 2;
	j2.tasks[colossal::task::TASK_TYPE_MAP].push_back(t3);

	colossal::job_tracker jt(2, 0);

	colossal::pool &mod  = jt.add_pool("modeling", 1, 1, 1, 1, 0, colossal::pool::SCHED_FAIR);
	colossal::pool &prod = jt.add_pool("prod",     1, 1, 1, 1, 0, colossal::pool::SCHED_FAIR);

	mod.add_job(j1);
	prod.add_job(j2);

	jt.process();

	printf("------------ WORKLOAD ------------\n");
	printf("%s\n", mod.to_str().c_str());
	printf("%s\n", prod.to_str().c_str());
	printf("----------------------------------\n");

        return 0;
}
