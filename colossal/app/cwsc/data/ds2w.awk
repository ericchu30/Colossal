#!/bin/awk -f

{
	split($12, att, "_");
	if ($14 && $15 && ($16 == "SUCCESS" || att[6] > 0)) {
		if ($13 == "REDUCE")
			printf("%s\t%s:%s\t%s\tREDUCE\t%.3f\t%.3f\t%.3f\n",
			       $2, $1, $3, $12, $7/1000.0, $14/1000.0, $15/1000.0)
		else if ($13 == "MAP")
			printf("%s\t%s:%s\t%s\tMAP\t%.3f\t%.3f\t%.3f\n",
			       $2, $1, $3, $12, $7/1000.0, $14/1000.0, $15/1000.0)
	}
}
