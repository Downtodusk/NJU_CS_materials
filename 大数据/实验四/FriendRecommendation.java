package com.example;

import java.io.IOException;
import java.util.*;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.*;
import org.apache.hadoop.mapreduce.lib.output.*;

/**
 * FriendRecommendation.java
 *
 * Usage:
 *   hadoop jar FriendRecommendation.jar FriendRecommendation \
 *       <mutualPairsInput> <followListInputDir> <tempCandidatesOutput> <finalOutput>
 */
public class FriendRecommendation {

    /*** === Job1 Mapper: 读取互相关注对，生成“用户 -> 好友列表” === */
    public static class MutualMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text user = new Text();
        private Text friend = new Text();

        @Override
        protected void map(LongWritable key, Text value, Context ctx)
                throws IOException, InterruptedException {
            String line = value.toString().trim();
            if (line.isEmpty() || !line.contains("-")) return;
            String[] parts = line.split("-", 2);
            if (parts.length != 2) return;

            // A-B 互相关注，双向输出
            String a = parts[0].trim();
            String b = parts[1].trim();
            if (a.isEmpty() || b.isEmpty()) return;

            user.set(a); friend.set(b);
            ctx.write(user, friend);

            user.set(b); friend.set(a);
            ctx.write(user, friend);
        }
    }

    /*** === Job1 Reducer: 对每个用户构造所有“朋友的朋友”候选对 === */
    public static class CandidatesReducer extends Reducer<Text, Text, Text, Text> {
        private Text outKey = new Text();
        private Text outVal = new Text();

        @Override
        protected void reduce(Text user, Iterable<Text> vals, Context ctx)
                throws IOException, InterruptedException {
            // 收集所有直接好友
            List<String> friends = new ArrayList<>();
            for (Text t : vals) {
                friends.add(t.toString());
            }
            // 两两配对，生成候选推荐对：如果 A 与 B 和 C 互粉，则输出 B->C 和 C->B
            int n = friends.size();
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (i == j) continue;
                    String b = friends.get(i);
                    String c = friends.get(j);
                    outKey.set(b);
                    outVal.set(c);
                    ctx.write(outKey, outVal);
                }
            }
        }
    }


    /*** === Job2 MapperA: 读取 Job1 的候选对 (userB \t userC) === */
    public static class CandidateMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text user = new Text();
        private Text tagAndVal = new Text();

        @Override
        protected void map(LongWritable key, Text value, Context ctx)
                throws IOException, InterruptedException {
            String line = value.toString().trim();
            if (line.isEmpty()) return;
            String[] parts = line.split("\\t");
            if (parts.length != 2) return;
            String b = parts[0].trim();
            String c = parts[1].trim();
            if (b.isEmpty() || c.isEmpty()) return;

            user.set(b);
            tagAndVal.set("C:" + c);
            ctx.write(user, tagAndVal);
        }
    }

    /*** === Job2 MapperB: 读取关注列表 (user: f1 f2 ...) === */
    public static class FollowListMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text user = new Text();
        private Text tagAndVal = new Text();

        @Override
        protected void map(LongWritable key, Text value, Context ctx)
                throws IOException, InterruptedException {
            String line = value.toString().trim();
            if (!line.contains(":")) return;
            String[] parts = line.split(":", 2);
            String u = parts[0].trim();
            String[] fs = parts[1].trim().split("\\s+");
            if (u.isEmpty()) return;

            // 将整个列表当成一个值
            user.set(u);
            tagAndVal.set("F:" + String.join(",", fs));
            ctx.write(user, tagAndVal);
        }
    }

    /*** === Job2 Reducer: 过滤已有关注，仅保留最多 5 个候选 === */
    public static class FilterReducer extends Reducer<Text, Text, Text, Text> {
        private Text result = new Text();

        @Override
        protected void reduce(Text user, Iterable<Text> vals, Context ctx)
                throws IOException, InterruptedException {
            // 收集已关注列表
            Set<String> followed = new HashSet<>();
            // 收集所有候选
            Set<String> candidates = new LinkedHashSet<>();

            for (Text t : vals) {
                String s = t.toString();
                if (s.startsWith("F:")) {
                    String[] fs = s.substring(2).split(",");
                    for (String f : fs) if (!f.isEmpty())
                        followed.add(f);
                } else if (s.startsWith("C:")) {
                    String c = s.substring(2);
                    if (!c.isEmpty()) {
                        candidates.add(c);
                    }
                }
            }

            // 过滤掉已关注的
            candidates.removeAll(followed);

            // 取前5
            Iterator<String> it = candidates.iterator();
            List<String> out = new ArrayList<>();
            int cnt = 0;
            while (it.hasNext() && cnt < 5) {
                out.add(it.next());
                cnt++;
            }

            if (!out.isEmpty()) {
                result.set(String.join(",", out));
                ctx.write(user, result);
            }
        }
    }


    /*** === Driver: 依次提交两个 Job === */
    public static void main(String[] args) throws Exception {
        if (args.length != 4) {
            System.err.println("Usage: FriendRecommendation " +
                "<mutualPairsIn> <followListInDir> <tempCandidatesOut> <finalOut>");
            System.exit(-1);
        }
        String mutualIn = args[0];
        String followIn = args[1];
        String tempOut  = args[2];
        String finalOut = args[3];

        Configuration conf = new Configuration();

        // ---- Job1: 互相关注 -> 候选推荐对 ----
        Job job1 = Job.getInstance(conf, "GenerateCandidates");
        job1.setJarByClass(FriendRecommendation.class);
        job1.setMapperClass(MutualMapper.class);
        job1.setReducerClass(CandidatesReducer.class);
        job1.setMapOutputKeyClass(Text.class);
        job1.setMapOutputValueClass(Text.class);
        job1.setOutputKeyClass(Text.class);
        job1.setOutputValueClass(Text.class);
        TextInputFormat.addInputPath(job1, new Path(mutualIn));
        TextOutputFormat.setOutputPath(job1, new Path(tempOut));
        job1.waitForCompletion(true);

        // ---- Job2: 过滤已关注 -> 最终推荐 ----
        Job job2 = Job.getInstance(conf, "FilterAndRecommend");
        job2.setJarByClass(FriendRecommendation.class);

        // 多路输入：候选对 和 关注列表
        MultipleInputs.addInputPath(job2, new Path(tempOut),
                                    TextInputFormat.class,
                                    CandidateMapper.class);
        MultipleInputs.addInputPath(job2, new Path(followIn),
                                    TextInputFormat.class,
                                    FollowListMapper.class);

        job2.setReducerClass(FilterReducer.class);
        job2.setMapOutputKeyClass(Text.class);
        job2.setMapOutputValueClass(Text.class);
        job2.setOutputKeyClass(Text.class);
        job2.setOutputValueClass(Text.class);
        FileOutputFormat.setOutputPath(job2, new Path(finalOut));

        System.exit(job2.waitForCompletion(true) ? 0 : 1);
    }
}
