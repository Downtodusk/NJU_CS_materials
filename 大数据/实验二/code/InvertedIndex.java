package com.example;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.GenericOptionsParser;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Set;
import java.util.HashSet;

public class InvertedIndex {
    // Mapper 类
    public static class InvertedIndexMapper extends Mapper<Object, Text, Text, Text> {
        private Text word = new Text();
        private Text docId = new Text();

        @Override
        protected void setup(Context context) throws IOException, InterruptedException {
            // 获取输入文件的名称作为文档ID
            FileSplit fileSplit = (FileSplit) context.getInputSplit();
            String fileName = fileSplit.getPath().getName();
            // skip stopwords file
            if (!fileName.contains("cn_stopwords.txt")) {
                docId.set(fileName);
            }
        }

        @Override
        public void map(Object key, Text value, Context context) throws IOException, InterruptedException {
            if (docId.toString().isEmpty()) {
                return;
            }
            // 分词
            StringTokenizer itr = new StringTokenizer(value.toString());
            while (itr.hasMoreTokens()) {
                word.set(itr.nextToken());
                // 输出格式：<词语, 文档ID:1>
                context.write(word, new Text(docId + ":1"));
            }
        }
    }

    // Reducer 类
    public static class InvertedIndexReducer extends Reducer<Text, Text, Text, Text> {
        @Override
        public void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            // 使用 HashMap 统计每个文档中的词频
            Map<String, Integer> docFreqMap = new HashMap<>();
            // 使用 HashSet 统计文档频率（DF）
            Set<String> docSet = new HashSet<>();
            // 遍历所有值，统计词频
            int totalFrequency = 0; // 总词频（TF）

            for (Text val : values) {
                String[] docAndFreq = val.toString().split(":");
                String docId = docAndFreq[0];
                int freq = Integer.parseInt(docAndFreq[1]);
                // 累加总词频
                totalFrequency += freq;
                docFreqMap.put(docId, docFreqMap.getOrDefault(docId, 0) + freq);
                // 添加到文档集合（去重）
                docSet.add(docId);
            }

            // 文档频率（DF）
            double df = docSet.size();
            // 计算平均词频（TF / DF）
            double averageFrequency = (df > 0) ? (double) totalFrequency / df : 0.0;

            StringBuilder outputBuilder = new StringBuilder();
            outputBuilder.append(String.format("%.2f", averageFrequency)).append(", ");
            for (Map.Entry<String, Integer> entry : docFreqMap.entrySet()) {
                outputBuilder.append(entry.getKey()).append(":").append(entry.getValue()).append("; ");
            }
            if (docFreqMap.size() > 0) {
                outputBuilder.setLength(outputBuilder.length() - 2);
            }
            Text outputKey = new Text("[" + key.toString() + "]");
            context.write(outputKey, new Text(outputBuilder.toString()));
        }
    }

    // 主程序
    public static void main(String[] args) throws Exception {
        Configuration conf = new Configuration();
        String[] programArgs = new GenericOptionsParser(conf, args).getRemainingArgs();
        // 调试：打印原始参数
        System.out.println("Raw args: " + String.join(", ", programArgs));

        Job job = Job.getInstance(conf, "inverted index");
        job.setJarByClass(InvertedIndex.class);
        job.setMapperClass(InvertedIndexMapper.class);
        job.setReducerClass(InvertedIndexReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);

        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}