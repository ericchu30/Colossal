#!/bin/awk -f

{
	if ($14 && $15 && $11 == "SUCCESS") {
		if ($15 > ftime[$1,$2])
			ftime[$1,$2] = $15;
		if ($14 < ctime[$1,$2] || ctime[$1,$2] == 0)
			ctime[$1,$2] = $14;
	}
}

END {
	for (i in ctime) {
		split(i, a, SUBSEP);
		print a[2]"\t"a[1]"\t"ftime[i] - ctime[i]
	}
}
