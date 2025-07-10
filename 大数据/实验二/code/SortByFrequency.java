package com.example;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.DoubleWritable;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

import java.io.IOException;

public class SortByFrequency {
    public static class SorterMapper extends Mapper<Object, Text, DoubleWritable, Text> {
        private DoubleWritable negAvgFrequency = new DoubleWritable();
        private Text outputValue = new Text();

        @Override
        public void map(Object key, Text value, Context context) throws IOException, InterruptedException {
            // 获取输入行并去除首尾空格
            String line = value.toString().trim();
            if (line.isEmpty()) return;

            // 分割词语部分和后续内容
            String[] parts = line.split("\t", 2);
            if (parts.length < 2) {
                // 输入格式错误，跳过
                return;
            }

            String secondPart = parts[1];

            // 从 secondPart 中提取频率（在第一个逗号之前）
            int commaIndex = secondPart.indexOf(',');
            if (commaIndex == -1) {
                return;
            }

            String frequencyStr = secondPart.substring(0, commaIndex).trim();

            try {
                // 解析频率并取负值
                double frequency = Double.parseDouble(frequencyStr);
                double negFrequency = -frequency; // 取负值以实现降序排序
                negAvgFrequency.set(negFrequency);
                outputValue.set(line);
                context.write(negAvgFrequency, outputValue);
            } catch (NumberFormatException e) {
                // 频率解析失败，跳过此行
                System.err.println("Failed to parse frequency from: " + line);
            }
        }
    }
    public static class SorterReducer extends Reducer<DoubleWritable, Text, Text, NullWritable> {
        public void reduce(DoubleWritable key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            // 遍历与当前键关联的所有值
            for (Text value : values) {
                // 直接输出整行文本，值为空
                context.write(value, NullWritable.get());
            }
        }
    }

    public static void main(String[] args) throws Exception{
        Configuration conf = new Configuration();

        Job job = Job.getInstance(conf, "Sorter");
        job.setJarByClass(SortByFrequency.class);
        job.setMapperClass(SortByFrequency.SorterMapper.class);
        job.setReducerClass(SortByFrequency.SorterReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        job.setMapOutputKeyClass(DoubleWritable.class);
        job.setMapOutputValueClass(Text.class);

        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}