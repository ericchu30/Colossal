#!/bin/awk -f

{
	split($12, att, "_");
	if ($14 && $15 && $11 == "SUCCESS" && ($16 == "SUCCESS" || att[6] > 0)) {
		if($13=="MAP")
			nmap[$1]++;
		else if($13=="REDUCE")
			nred[$1]++;
		if(!pool[$1])
			pool[$1]=$2;
	}
}

END {
	for (j in pool) {
		if (!nmap[j])
			nmap[j] = 0;
		if (!nred[j])
			nred[j] = 0;
		print pool[j] "\t" j "\t" nmap[j] "\t" nred[j];
	}
}
