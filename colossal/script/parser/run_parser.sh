#!/bin/bash

#####################################
#   Hadoop environmental variables
#####################################
export HADOOP_HOME=/usr/lib/hadoop-0.20-mapreduce
export HADOOP_CLASSPATH="/etc/hadoop/conf:/usr/lib/hadoop/lib/*:/usr/lib/hadoop/.//*:/usr/lib/hadoop-hdfs/./:/usr/lib/hadoop-hdfs/lib/*:/usr/lib/hadoop-hdfs/.//*:/usr/lib/hadoop-yarn/lib/*:/usr/lib/hadoop-yarn/.//*:/usr/lib/hadoop-0.20-mapreduce/./:/usr/lib/hadoop-0.20-mapreduce/lib/*:/usr/lib/hadoop-0.20-mapreduce/.//*"

# Users can specify the range of the input data.
# How many days of job history files to parse back from (including) today.
DAYS=7

# Generate HDFS input paths
DATE_RANGE="{"
for i in `seq 0 $(($DAYS-1))`; do
    DATE_RANGE=$DATE_RANGE,`date -d "$i day ago" +"%Y/%m/%d"`;
done
DATE_RANGE=$DATE_RANGE"}"

# Users can modify the base HDFS input and output paths below
# HDFS path of Hadoop job history files
INPUT="/mapred/job_history/done/jobtracker.inw-hercules.rfiserve.net_1400569084740_/$DATE_RANGE/*/job_*_*_[^c]*"
# HDFS path of output dataset files
OUTPUT="/user/ztan/parsed/`date +\"%F\"`_$DAYS"

${HADOOP_HOME}/bin/hadoop fs -rm -r "$OUTPUT"

# Users can specify the #Map, #Reduce, and pool below
${HADOOP_HOME}/bin/hadoop \
    jar ${HADOOP_HOME}/contrib/streaming/*.jar \
    -D mapreduce.job.reduces=30 \
    -D mapred.reduce.tasks=30 \
    -D mapred.job.queue.name="engineer" \
    -D mapred.min.split.size=10000000000 \
    -input "$INPUT" \
    -output "$OUTPUT" \
    -mapper "parser.awk" \
    -reducer cat \
    -file parser.awk
#   -D textinputformat.record.delimiter=".\n" \
