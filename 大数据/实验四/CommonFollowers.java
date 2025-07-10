package com.example;

import java.io.IOException;
import java.util.*;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.MultipleOutputs;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;

public class CommonFollowers {

    // ========== JOB 1 ==========
    public static class Job1Mapper extends Mapper<LongWritable, Text, Text, Text> {
        @Override
        protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
            String line = value.toString().trim();
            if (line.contains(":")) { // 处理关注列表: user: friend1 friend2 ...
                String[] parts = line.split(":");
                if (parts.length != 2) return;
                String user = parts[0].trim();
                String[] friends = parts[1].trim().split("\\s+");
                context.write(new Text(user), new Text("L:" + String.join(",", friends)));
            } else if (line.contains("-")) { // 处理互相关注对: userA-userB
                String[] users = line.split("-");
                if (users.length != 2) return;
                String userA = users[0].trim();
                String userB = users[1].trim();
                context.write(new Text(userA), new Text("P:" + userB));
                context.write(new Text(userB), new Text("P:" + userA));
            }
        }
    }

    public static class Job1Reducer extends Reducer<Text, Text, Text, Text> {
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            List<String> partners = new ArrayList<>();
            String friendList = "";

            for (Text val : values) {
                String value = val.toString();
                if (value.startsWith("P:")) {
                    partners.add(value.substring(2));
                } else if (value.startsWith("L:")) {
                    friendList = value.substring(2);
                }
            }

            for (String partner : partners) {
                String pair = key.toString().compareTo(partner) < 0 ? key + "-" + partner : partner + "-" + key;
                context.write(new Text(pair), new Text(friendList));
            }
        }
    }

    // ========== JOB 2 ==========
    public static class Job2Mapper extends Mapper<LongWritable, Text, Text, Text> {
        @Override
        protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
            String[] parts = value.toString().split("\\t");
            if (parts.length != 2) return;
            context.write(new Text(parts[0]), new Text(parts[1]));
        }
    }

    public static class Job2Reducer extends Reducer<Text, Text, Text, Text> {
        private MultipleOutputs<Text, Text> mos;
        private int threshold;

        @Override
        protected void setup(Context context) throws IOException, InterruptedException {
            mos = new MultipleOutputs<>(context);
            threshold = context.getConfiguration().getInt("threshold", 0);
        }

        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            List<String> lists = new ArrayList<>();
            for (Text val : values) {
                lists.add(val.toString());
            }
            if (lists.size() != 2) return;

            Set<String> set1 = new HashSet<>(Arrays.asList(lists.get(0).split(",")));
            Set<String> set2 = new HashSet<>(Arrays.asList(lists.get(1).split(",")));
            set1.retainAll(set2);

            if (set1.isEmpty()) return;

            String commonFriends = String.join(" ", set1);
            Text formattedLine = new Text(key.toString() + ": " + commonFriends);

            if (set1.size() <= threshold) {
                mos.write("small", NullWritable.get(), formattedLine, "small/part");
            } else {
                mos.write("large", NullWritable.get(), formattedLine, "large/part");
            }
        }

        @Override
        protected void cleanup(Context context) throws IOException, InterruptedException {
            mos.close();
        }
    }

    // ========== MAIN ==========
    public static void main(String[] args) throws Exception {
        if (args.length != 5) {
            System.err.println("Usage: CommonFollowers <mutualPairs> <friendListDir> <x> <outputJob1> <outputJob2>");
            System.exit(1);
        }

        // ----- Job 1 -----
        Configuration conf1 = new Configuration();
        Job job1 = Job.getInstance(conf1, "Job1: Emit pair and list");
        job1.setJarByClass(CommonFollowers.class);
        job1.setMapperClass(Job1Mapper.class);
        job1.setReducerClass(Job1Reducer.class);
        job1.setOutputKeyClass(Text.class);
        job1.setOutputValueClass(Text.class);

        FileInputFormat.addInputPath(job1, new Path(args[0]));
        FileInputFormat.addInputPath(job1, new Path(args[1]));
        FileOutputFormat.setOutputPath(job1, new Path(args[3]));

        if (!job1.waitForCompletion(true)) System.exit(1);

        // ----- Job 2 -----
        Configuration conf2 = new Configuration();
        conf2.setInt("threshold", Integer.parseInt(args[2]));

        Job job2 = Job.getInstance(conf2, "Job2: Find common friends");
        job2.setJarByClass(CommonFollowers.class);
        job2.setMapperClass(Job2Mapper.class);
        job2.setReducerClass(Job2Reducer.class);
        job2.setOutputKeyClass(Text.class);
        job2.setOutputValueClass(Text.class);

        FileInputFormat.addInputPath(job2, new Path(args[3]));
        FileOutputFormat.setOutputPath(job2, new Path(args[4]));
        MultipleOutputs.addNamedOutput(job2, "small", TextOutputFormat.class, NullWritable.class, Text.class);
        MultipleOutputs.addNamedOutput(job2, "large", TextOutputFormat.class, NullWritable.class, Text.class);

        System.exit(job2.waitForCompletion(true) ? 0 : 1);
    }
}
