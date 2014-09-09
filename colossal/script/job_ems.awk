#!/bin/awk -f

{
	if ($8 > ftime[$1,$2])
		ftime[$1,$2] = $8;
	if ($7 < ctime[$1,$2] || ctime[$1,$2] == 0)
		ctime[$1,$2] = $7;
}

END {
	for (i in ctime) {
		split(i, a, SUBSEP);
		print a[1]"\t"a[2]"\t"ftime[i] - ctime[i]
	}
}
