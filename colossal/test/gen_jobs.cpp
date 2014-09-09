#include <time.h>
#include <colossal/colossal.hpp>

int main()
{
        colossal::job_generator gen(0.01, 5000, 2000, 80, 80, 0.7, 3000, 300, 300);
        gen.seed(time(NULL));
        colossal::job j1 = gen();
        printf("%s\n", j1.to_str().c_str());

        colossal::job_generator gen1(0.1, 20, 20, 80, 0.7, 0.7, 20, 100, 20);
        gen1.seed(time(NULL) + 1);
        colossal::job j2 = gen1();
        colossal::job j3 = gen1();

        printf("\n%s\n\n%s\n", j2.to_str().c_str(), j3.to_str().c_str());

        return 0;
}
