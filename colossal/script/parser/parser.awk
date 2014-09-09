#!/bin/awk -f

BEGIN {
	RS = " [.]\n"; # Separator of a line in Hadoop job history files
}

# Matches the beginning of a job
# Job JOBID="job_201408150243_8359" JOBNAME="insert overwrite table q61_forecast_rev\.\.\.24(Stage-1)" USER="unravel" SUBMIT_TIME="1409962533962" JOBCONF="hdfs://master:8020/user/unravel/\.staging/job_201408150243_8359/job\.xml" VIEW_JOB="*" MODIFY_JOB="*" JOB_QUEUE="default" .
/SUBMIT_TIME=/ {
	# In case that files are cancatenated, clear all hash maps at the beginnning
	for (i in AT)
		delete AT[i]; # AttemptID-to-TaskType mapping
	for (i in AS)
		delete AS[i]; # AttemptID-to-StartTime mapping
	for (i in TS)
		delete TS[i]; # AttemptID-to-TaskStatus mapping
	for (i in AF)
		delete AF[i]; # AttemptID-to-FinishTime mapping

	match($0, /JOBID=\"(job[0-9_]+)\" .*SUBMIT_TIME=\"([0-9]+)\" .*JOB_QUEUE=\"(.*)\"/, s)
	if (s[1] == "" || s[2] == "" || s[3] == "") {
		print "JOBID, SUBMIT_TIME, and JOB_QUEUE must in one record";
		exit(-1);
	}
	jobid = s[1];
	submit_time = s[2];
	queue = s[3]
}

# Matches "Job JOBID="job_201408150243_8359" LAUNCH_TIME="1409962534101" TOTAL_MAPS="3" TOTAL_REDUCES="1" JOB_STATUS="PREP" ."
/LAUNCH_TIME=/ {
	match($0, /JOBID=\"(job[0-9_]+)\" LAUNCH_TIME=\"([0-9]+)\" TOTAL_MAPS=\"([0-9]+)\" TOTAL_REDUCES=\"([0-9]+)\"/, s);
	if (s[1] == "" || s[2] == "" || s[3] == "" || s[4] == "") {
		print "JOBID, LAUNCH_TIME, TOTAL_MAPS, and TOTAL_REDUCES must come together";
		exit(-2);
	}
	if (s[1] != jobid) {
		print "JOBID with LAUNCH_TIME:"s[1]" does not match JOBID with SUBMIT_TIME:"jobid;
		exit(-3);
	}
	launch_time = s[2];
	total_maps = s[3];
	total_reduces = s[4];
}

# Matches "Job JOBID="job_201408150243_8359" JOB_PRIORITY="NORMAL" ."
/JOB_PRIORITY=/ {
	match($0, /JOBID=\"(job[0-9_]+)\" JOB_PRIORITY=\"(.*)\"/, s);
	if (s[1] != jobid) {
		print "JOBID with JOB_PRIORITY:"s[1]" does not match JOBID with SUBMIT_TIME:"jobid;
		exit(-4);
	}
	priority = s[2];
}

# Matches "ReduceAttempt TASK_TYPE="SETUP" TASKID="task_201408150243_8359_r_000002" TASK_ATTEMPT_ID="attempt_201408150243_8359_r_000002_0" START_TIME="1409962534532" TRACKER_NAME="tracker_slave7:localhost/127\.0\.0\.1:36585" HTTP_PORT="50060" ."
/TASK_ATTEMPT_ID=.*START_TIME/ {
	match($0, /TASK_TYPE=\"([A-Z]+)\" .*TASK_ATTEMPT_ID=\"(attempt[0-9_mr]+)\" START_TIME=\"([0-9]+)\"/, s);
	if (s[1] == "" || s[2] == "" || s[3] == "") {
		print "TASK_TYPE, TASK_ATTEMPT_ID, and START_TIME must in one record";
		exit(-5);
	}
	AT[s[2]] = s[1];
	AS[s[2]] = s[3];
}

# Matches "ReduceAttempt TASK_TYPE="REDUCE" TASKID="task_201408150243_8359_r_000000" TASK_ATTEMPT_ID="attempt_201408150243_8359_r_000000_0" TASK_STATUS="SUCCESS" SHUFFLE_FINISHED="1409962584915" SORT_FINISHED="1409962584989" FINISH_TIME="1409962588637" HOSTNAME="/default/slave5" STATE_STRING="reduce > reduce" COUNTERS=..."
/TASK_ATTEMPT_ID=.*FINISH_TIME/ {
	match($0, /TASK_TYPE=\"([A-Z]+)\" .*TASK_ATTEMPT_ID=\"(attempt[0-9_mr]+)\" TASK_STATUS=\"([A-Z]+)\" .*FINISH_TIME=\"([0-9]+)\"/, s);
	if (s[1] == "" || s[2] == "" || s[3] == "" || s[4] == "") {
		print "TASK_TYPE, TASK_ATTEMPT_ID, TASK_STATUS, and FINISH_TIME must in one record";
		exit(-6);
	}
	if (AT[s[2]] != s[1]) {
		print "Finish attempt type:"s[1]" for attempt:"s[2]" mismatches the start type:"AT[s[2]];
		exit(-7);
	}
	AT[s[2]] = s[1];
	TS[s[2]] = s[3];
	AF[s[2]] = s[4];
}

# Matches "Job JOBID="job_201408150243_8359" FINISH_TIME="1409962592144" JOB_STATUS="SUCCESS" FINISHED_MAPS="3" FINISHED_REDUCES="1" FAILED_MAPS="0" FAILED_REDUCES="0" MAP_COUNTERS=..."
/FINISHED_MAPS=/ {
	match($0, /JOBID=\"(job[0-9_]+)\" FINISH_TIME=\"([0-9]+)\" JOB_STATUS=\"([A-Z]+)\" FINISHED_MAPS=\"([0-9]+)\" FINISHED_REDUCES=\"([0-9]+)\"/, s);
	if (s[1] == "" || s[2] == "" || s[3] == "" || s[4] == "" || s[5] == "") {
		print "JOBID, FINISH_TIME, JOB_STATUS, FINISHED_MAPS and FINISHED_REDUCES must be in one record";
		exit(-8);
	}
	if (s[1] != jobid) {
		print "JOBID with FINISHED_MAPS:"s[1]" does not match JOBID with SUBMIT_TIME:"jobid;
		exit(-9);
	}
	# Now job is finished
	for (a in AS) {
	# jobid pool priority total_maps total_reduces submit_time launch_time job_finish_time finished_maps finished_reduces job_status attID type start_time finish_time task_status
	print jobid"\t"queue"\t"priority"\t"total_maps"\t"total_reduces"\t"submit_time"\t"launch_time"\t"s[2]"\t"s[4]"\t"s[5]"\t"s[3]"\t"a"\t"AT[a]"\t"AS[a]"\t"AF[a]"\t"TS[a];
}
	}
