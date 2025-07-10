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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MutualFriends {

    public static class MutualFriendsMapper extends Mapper<LongWritable, Text, Text, Text> {
        @Override
        protected void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
            // 输入格式: user_name: friend1 friend2 friend3 ...
            String line = value.toString().trim();
            if (line.isEmpty() || !line.contains(":")) {
                return;
            }

            // 分割用户名和朋友列表
            String[] parts = line.split(":");
            String user = parts[0].trim();
            String[] friends = parts.length > 1 ? parts[1].trim().split("\\s+") : new String[0];

            // 为每个朋友生成键值对
            for (String friend : friends) {
                if (!friend.isEmpty()) {
                    // 创建排序后的用户对作为key，确保user_A < user_B
                    String pairKey = user.compareTo(friend) < 0 ? user + "-" + friend : friend + "-" + user;
                    // value为当前用户，表示这个用户关注了这个朋友
                    context.write(new Text(pairKey), new Text(user));
                }
            }
        }
    }

    public static class MutualFriendsReducer extends Reducer<Text, Text, Text, Text> {
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            // 收集所有用户
            List<String> users = new ArrayList<>();
            for (Text value : values) {
                users.add(value.toString());
            }

            // 如果恰好有两个不同的用户，说明是互相关注
            if (users.size() == 2 && !users.get(0).equals(users.get(1))) {
                context.write(key, new Text(""));
            }
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.err.println("Usage: MutualFriendsMapReduce <input path> <output path>");
            System.exit(-1);
        }

        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Mutual Friends Finder");
        job.setJarByClass(MutualFriends.class);

        // 设置MapReduce类
        job.setMapperClass(MutualFriendsMapper.class);
        job.setReducerClass(MutualFriendsReducer.class);

        // 设置输出类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);

        // 设置输入输出路径
        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));

        // 运行作业
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}