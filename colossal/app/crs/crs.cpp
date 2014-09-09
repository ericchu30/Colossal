/* Colossal
 * Copyright (c) 2014 Zilong Tan (eric.zltan@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, subject to the conditions listed
 * in the Colossal LICENSE file. These conditions include: you must preserve this
 * copyright notice, and you cannot mention the copyright holders in
 * advertising related to the Software without their permission.  The Software
 * is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This notice is a
 * summary of the Colossal LICENSE file; the license in that file is legally
 * binding.
 */

#include <ctime>
#include <string>
#include <cstdlib>
#include <iostream>
#include <libconfig.h++>
#include <colossal/colossal.hpp>

using namespace std;
using namespace colossal;
using namespace libconfig;

const char *CONFIG_FILE = "./conf/crs.conf";

Config        g_conf;
job_tracker * g_job_tracker = NULL;
int           g_sim_period;
int           g_nmaps;
int           g_nreduces;
int           g_metrics_win;
string        g_metrics;
string        g_output;

void initialize_simulator()
{
	g_sim_period = g_conf.lookup("simulator.simulation_period");
	g_output  = (const char *)g_conf.lookup("simulator.output");
	g_metrics = (const char *)g_conf.lookup("simulator.metrics");
	g_metrics_win = g_conf.lookup("simulator.metrics_win");
}

void create_job_tracker()
{
	g_nmaps = g_conf.lookup("cluster.total_maps");
	g_nreduces = g_conf.lookup("cluster.total_reduces");
	g_job_tracker = new job_tracker(g_nmaps, g_nreduces);
	if (!g_job_tracker->set_metrics(
		    g_metrics.size()? g_metrics.c_str(): NULL, g_metrics_win)) {
		ULIB_FATAL("failed to set metrics");
		exit(EXIT_FAILURE);
	}
}

void create_pools()
{
	const Setting &pools = g_conf.lookup("pools");
	int npools = pools.getLength();
	size_t njobs = 0;

	for (int i = 0; i < npools; ++i) {
		const Setting &pool = pools[i];
		string name;
		string sched_mode;
		double min_share_timeout;
		double fair_share_timeout;
		double weight;
		int    map_min_share;
		int    reduce_min_share;
		double job_arrival_rate;
		double logmean_maps_per_job;
		double logmean_reduces_per_job;
		double logsd_maps_per_job;
		double logsd_reduces_per_job;
		double logmean_map_duration;
		double logmean_reduce_duration;
		double logsd_map_duration;
		double logsd_reduce_duration;
		if (!(pool.lookupValue("name", name) &&
		      pool.lookupValue("sched_mode", sched_mode) &&
		      pool.lookupValue("min_share_timeout", min_share_timeout) &&
		      pool.lookupValue("fair_share_timeout", fair_share_timeout) &&
		      pool.lookupValue("weight", weight) &&
		      pool.lookupValue("map_min_share", map_min_share) &&
		      pool.lookupValue("reduce_min_share", reduce_min_share) &&
		      pool.lookupValue("job_arrival_rate", job_arrival_rate) &&
		      pool.lookupValue("logmean_maps_per_job", logmean_maps_per_job) &&
		      pool.lookupValue("logmean_reduces_per_job", logmean_reduces_per_job) &&
		      pool.lookupValue("logsd_maps_per_job", logsd_maps_per_job) &&
		      pool.lookupValue("logsd_reduces_per_job", logsd_reduces_per_job) &&
		      pool.lookupValue("logmean_map_duration", logmean_map_duration) &&
		      pool.lookupValue("logmean_reduce_duration", logmean_reduce_duration) &&
		      pool.lookupValue("logsd_map_duration", logsd_map_duration) &&
		      pool.lookupValue("logsd_reduce_duration", logsd_reduce_duration))) {
			cerr << "Missing pool settings for pool " << i << endl;
			exit(EXIT_FAILURE);
		}
		pool::sched_mode sched;
		if (sched_mode == "fair")
			sched = pool::SCHED_FAIR;
		else if (sched_mode == "fcfs" || sched_mode == "fifo")
			sched = pool::SCHED_FCFS;
		else {
			ULIB_WARNING("invalid scheduling mode:%s for pool %s",
				     sched_mode.c_str(), name.c_str());
			exit(EXIT_FAILURE);
		}
		colossal::pool &p = g_job_tracker->add_pool(
			name, min_share_timeout, fair_share_timeout,
			weight, map_min_share, reduce_min_share, sched);
		job_generator gen(job_arrival_rate, logmean_maps_per_job, logmean_reduces_per_job,
				  logsd_maps_per_job, logsd_reduces_per_job, logmean_map_duration,
				  logmean_reduce_duration, logsd_map_duration, logsd_reduce_duration);
		gen.seed(time(NULL) + i);
		while (gen.get_time() < g_sim_period)
			p.add_job(gen());
		njobs += p.jobs.size();
		cerr << "Generated " << p.jobs.size() << " jobs for pool " << p.name << endl;
	}
	g_job_tracker->scale_minshares();
	cerr << "Loaded " << npools << " pools, " << njobs << " jobs" << endl;
}

void calc_utils()
{
	cout << "Map utilization:"
	     <<	compute_utilization(g_job_tracker->getpools(),
				    task::TASK_TYPE_MAP, g_nmaps)
	     << endl;

	cout << "Reduce utilization:"
	     <<	compute_utilization(g_job_tracker->getpools(),
				    task::TASK_TYPE_REDUCE, g_nreduces)
	     << endl;

	job_tracker::pool_container_type &pools = g_job_tracker->getpools();
	for (job_tracker::pool_container_type::const_iterator pit = pools.begin();
	     pit != pools.end(); ++pit) {
		cout << ">> Pool " << pit->name << " map utilization:"
		     << compute_utilization(*pit, task::TASK_TYPE_MAP, g_nmaps)
		     << endl;
		cout << ">> Pool " << pit->name << " reduce utilization:"
		     << compute_utilization(*pit, task::TASK_TYPE_REDUCE, g_nreduces)
		     << endl;
	}
}

int main()
{
	try {
		g_conf.readFile(CONFIG_FILE);
	} catch (const FileIOException &e) {
		cerr << "I/O error while reading " << CONFIG_FILE << endl;
		exit(EXIT_FAILURE);
	} catch(const ParseException &pex) {
		cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
		     << " - " << pex.getError() << std::endl;
		exit(EXIT_FAILURE);
	}

	try {
		initialize_simulator();
		create_job_tracker();
		create_pools();
	} catch (const SettingNotFoundException &e) {
		cerr << "Missing a setting in configuration file" << endl;
		exit(EXIT_FAILURE);
	}

	g_job_tracker->process();

	calc_utils();

	if (export_schedule(g_output.c_str(), g_job_tracker->getpools()) == 0)
		cerr << "Saved schedule to output " << g_output << endl;

	delete g_job_tracker;

	return 0;
}