#!/bin/awk -f

{
	sum[$1] += $3;
	sum1[$1] += $4;
	++cnt[$1];
}

END {
	for (i in sum)
		printf("%s\t%.3f\t%.15g\n", i, sum[i]/cnt[i], sum1[i]/cnt[i])
}
