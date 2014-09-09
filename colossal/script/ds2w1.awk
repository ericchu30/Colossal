#!/bin/awk -f

{
	if ($14 && $15) {
		if ($13 == "REDUCE")
			printf("%s\t%s:%s\t%s\tREDUCE\t%.3f\t%.3f\n",
			       $2, $1, $3, $12, $7/1000.0, ($15 - $14)/1000.0);
		else
			printf("%s\t%s:%s\t%s\tMAP\t%.3f\t%.3f\n",
			       $2, $1, $3, $12, $7/1000.0, ($15 - $14)/1000.0);
	}
}
