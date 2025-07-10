package com.example;

import org.apache.hadoop.conf.*;
import org.apache.hadoop.fs.*;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.*;
import org.apache.hadoop.mapreduce.lib.output.*;
import java.io.*;
import java.util.*;
import java.net.URI;

public class RemoveStopWords {
    public static class TokenMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Set<String> stopWords = new HashSet<>();
        private String docId;
        private Map<String, Integer> wordCounts = new HashMap<>();

        @Override
        protected void setup(Context context) throws IOException {
            // 加载停用词
            URI[] cacheFiles = context.getCacheFiles();
            if (cacheFiles != null && cacheFiles.length > 0) {
                for (URI file : cacheFiles) {
                    if (file.toString().contains("cn_stopwords.txt")) {
                        BufferedReader reader = new BufferedReader(
                            new InputStreamReader(FileSystem.get(context.getConfiguration())
                                .open(new Path(file))));
                        String line;
                        while ((line = reader.readLine()) != null) {
                            stopWords.add(line.trim());
                        }
                        reader.close();
                    }
                }
            }
            
            // 获取文档ID
            FileSplit split = (FileSplit) context.getInputSplit();
            docId = split.getPath().getName();
        }

        @Override
        public void map(LongWritable key, Text value, Context context) {
            String[] words = value.toString().split(" ");
            for (String word : words) {
                if (!word.isEmpty() && !stopWords.contains(word)) {
                    wordCounts.put(word, wordCounts.getOrDefault(word, 0) + 1);
                }
            }
        }

        @Override
        protected void cleanup(Context context) throws IOException, InterruptedException {
            for (Map.Entry<String, Integer> entry : wordCounts.entrySet()) {
                context.write(new Text(entry.getKey()), 
                    new Text(docId + ":" + entry.getValue()));
            }
        }
    }

    public static class AvgReducer extends Reducer<Text, Text, Text, Text> {
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            // 使用 HashMap 统计每个文档中的词频
            Map<String, Integer> docFreqMap = new HashMap<>();
            // 使用 HashSet 统计文档频率（DF）
            Set<String> docSet = new HashSet<>();
            int totalFrequency = 0;

            for (Text val : values) {
                String[] docAndFreq = val.toString().split(":");
                String docId = docAndFreq[0];
                int freq = Integer.parseInt(docAndFreq[1]);
                totalFrequency += freq;
                docFreqMap.put(docId, docFreqMap.getOrDefault(docId, 0) + freq);
                docSet.add(docId);
            }

            double df = docSet.size();
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

    public static class ExcludeStopwordsFilter implements PathFilter {
        @Override
        public boolean accept(Path path) {
            return !path.getName().equals("cn_stopwords.txt");
        }
    }

    public static void main(String[] args) throws Exception {
        Configuration conf = new Configuration();

        Job job = Job.getInstance(conf, "Word Frequency");
        job.setJarByClass(RemoveStopWords.class);
        job.setMapperClass(TokenMapper.class);
        job.setReducerClass(AvgReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));

        // 停用词路径：input_path/cn_stopwords.txt
        Path stopwordsPath = new Path(args[0], "cn_stopwords.txt");
        job.addCacheFile(stopwordsPath.toUri());
        // 设置输入文件过滤器（关键修改）
        FileInputFormat.setInputPathFilter(job, ExcludeStopwordsFilter.class);

        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}