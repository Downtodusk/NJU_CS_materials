package com.example;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

public class CalculateTFIDF {

    public static class TFIDFMapper extends Mapper<Object, Text, Text, Text> {
        private Text word = new Text();
        private Text docInfo = new Text();
        private String docName;

        @Override
        protected void setup(Context context) throws IOException, InterruptedException {
            FileSplit fileSplit = (FileSplit) context.getInputSplit();
            docName = fileSplit.getPath().getName();
        }

        @Override
        public void map(Object key, Text value, Context context) throws IOException, InterruptedException {
            // skip cn_stopwords.txt
            if (docName.contains("cn_stopwords.txt")) {
                return;
            }

            StringTokenizer itr = new StringTokenizer(value.toString());
            Map<String, Integer> wordFreqMap = new HashMap<>();

            while (itr.hasMoreTokens()) {
                String w = itr.nextToken();
                wordFreqMap.put(w, wordFreqMap.getOrDefault(w, 0) + 1);
            }

            for (Map.Entry<String, Integer> entry : wordFreqMap.entrySet()) {
                word.set(entry.getKey());
                docInfo.set(docName + ":" + entry.getValue());
                context.write(word, docInfo);
            }
        }
    }

    public static class TFIDFReducer extends Reducer<Text, Text, Text, NullWritable> {
        private int totalDocs;

        @Override
        protected void setup(Context context) throws IOException, InterruptedException {
            totalDocs = context.getConfiguration().getInt("totalDocs", 1);
        }

        @Override
        public void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            Map<String, Integer> docFreqMap = new HashMap<>();
            Set<String> docSet = new HashSet<>();

            for (Text val : values) {
                String[] parts = val.toString().split(":");
                String docName = parts[0];
                int freq = Integer.parseInt(parts[1]);
                docFreqMap.put(docName, freq);
                docSet.add(docName);
            }

            double df = docSet.size();
            double idf = Math.log((double) totalDocs / (df + 1)) / Math.log(2);

            for (Map.Entry<String, Integer> entry : docFreqMap.entrySet()) {
                String docName = entry.getKey();
                int tf = entry.getValue();
                double tfidf = tf * idf;
                String outputKey = docName + ", " +key.toString() + ", " + String.format("%.3f", tfidf);
                context.write(new Text(outputKey), NullWritable.get());
            }
        }
    }

    public static void main(String[] args) throws Exception {
        Configuration conf = new Configuration();

        // 获取输入路径下的文件数量（书本总数）
        Path inputPath = new Path(args[0]);
        FileSystem fs = FileSystem.get(conf);
        FileStatus[] fileStatuses = fs.listStatus(inputPath);
        int totalDocs = 0;
        for (FileStatus status : fileStatuses) {
            if (status.isFile()) { // 只统计文件，不包括子目录
                String fileName = status.getPath().getName();  // 获取文件名
                if (!fileName.contains("cn_stopwords.txt")) {  // 跳过 cn_stopwords.txt
                    totalDocs++;
                }
            }
        }
        if (totalDocs == 0) {
            System.err.println("No files found in input path: " + inputPath);
            System.exit(1);
        }

        // 设置书本总数到配置中
        conf.setInt("totalDocs", totalDocs);

        Job job = Job.getInstance(conf, "Calculate TF-IDF Step 1");
        job.setJarByClass(CalculateTFIDF.class);
        job.setMapperClass(TFIDFMapper.class);
        job.setReducerClass(TFIDFReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);

        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}

//第一部-魔法石	一切,1.170
//[一家]	4.00, 第二部-密室:2; , 第一部-魔法石:7; , 第三部-阿兹卡班的囚徒:3; 
