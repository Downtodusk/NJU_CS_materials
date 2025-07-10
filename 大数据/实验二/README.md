JAR包执行方式说明：

本实验的jar包位于jar/文件夹下，执行方式如下：

- inverted-index-1.0-SNAPSHOT.jar（对应任务一）

  输入命令：

  ```
  yarn jar /path/to/inverted-index-1.0-SNAPSHOT.jar com.example.InvertedIndex /input_path /output_path
  ```

  ```
  yarn jar /path/to/inverted-index-1.0-SNAPSHOT.jar com.example.SortByFrequency /input_path /output_path
  ```

  第一个命令用于生成词频统计，第二个命令用于对第一个文档生成的结果进行排序

- compute-tf-idf-1.0-SNAPSHOT.jar（对应任务二）

  输入命令：

  ```
  yarn jar /path/to/compute-tf-idf-1.0-SNAPSHOT.jar com.example.CalculateTFIDF /input_path /output_path
  ```

- stop-words-1.0-SNAPSHOT.jar（对应任务三）

  输入命令：

  ```
  yarn jar /path/to/stop-words-1.0-SNAPSHOT.jar com.example.RemoveStopWords /input_path /output_path
  ```

  ```
  yarn jar /path/to/stop-words-1.0-SNAPSHOT.jar com.example.SortByFrequency /input_path /output_path
  ```
  
  第一个命令用于生成去处停用词后的词频统计，第二个命令用于对第一个文档生成的结果进行排序

