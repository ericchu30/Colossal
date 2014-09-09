#!/bin/awk -f

{
	if ($9 > ftime[$1,$2])
		ftime[$1,$2] = $9;
	if ($10 > ftime1[$1,$2])
		ftime1[$1,$2] = $10;
	if ($7 < ctime[$1,$2] || ctime[$1,$2] == 0)
		ctime[$1,$2] = $7;
	if ($8 < ctime1[$1,$2] || ctime1[$1,$2] == 0)
		ctime1[$1,$2] = $8;
}

END {
	for (i in ctime) {
		split(i, a, SUBSEP);
		print a[1]"\t"a[2]"\t"ftime[i] - ctime[i]"\t"ftime1[i] - ctime1[i];
	}
}
