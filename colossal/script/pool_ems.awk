#!/bin/awk -f

{
	sum[$1] += $3;
	++cnt[$1];
}

END {
	for (i in sum)
		printf("%s\t%.3f\n", i, sum[i]/cnt[i])
}
