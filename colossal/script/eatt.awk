#!/bin/awk -f

{
	split($12, att, "_");
	if ($14 && $15 && $11 == "SUCCESS" && ($16 == "SUCCESS" || att[6] > 0))
		print $2 "\t" $13 "\t" $14 "\t" $15;
}
