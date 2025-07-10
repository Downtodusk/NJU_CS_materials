package com.example;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

import java.io.IOException;

public class IOlogTraceProcessor {

    public static class IOlogMapper extends Mapper<LongWritable, Text, Text, LongWritable> {
        private Text userNamespace = new Text();
        private LongWritable opCount = new LongWritable();

        @Override
        protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
            String[] fields = value.toString().split(" ");
            if (fields.length < 9) {
                return;
            }

            String opName = fields[4].trim(); // op_name 是第 5 个字段
            String userNamespaceStr = fields[5].trim(); // user_namespace 是第 6 个字段
            String opCountStr = fields[8].trim(); // op_count 是第 9 个字段

            // 筛选 op_name = 2 的记录
            if ("2".equals(opName)) {
                try {
                    long count = Long.parseLong(opCountStr);
                    userNamespace.set(userNamespaceStr);
                    opCount.set(count);
                    context.write(userNamespace, opCount);
                } catch (NumberFormatException e) {
                }
            }
        }
    }

    public static class IOlogReducer extends Reducer<Text, LongWritable, Text, LongWritable> {
        private LongWritable result = new LongWritable();

        @Override
        protected void reduce(Text key, Iterable<LongWritable> values, Context context) throws IOException, InterruptedException {
            long sum = 0;
            // 累加 op_count
            for (LongWritable val : values) {
                sum += val.get();
            }
            result.set(sum);
            context.write(key, result);
        }
    }

    public static void main(String[] args) throws Exception {
        Configuration conf = new Configuration();

        conf.set("mapreduce.output.textoutputformat.separator", ",");
        
        Job job = Job.getInstance(conf, "IOlog Trace Processor");

        job.setJarByClass(IOlogTraceProcessor.class);
        job.setMapperClass(IOlogMapper.class);
        job.setCombinerClass(IOlogReducer.class); 
        job.setReducerClass(IOlogReducer.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(LongWritable.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(LongWritable.class);
        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));

        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}
