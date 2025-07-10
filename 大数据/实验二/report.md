# 第二次实验报告

| 姓名 | 学号      | 邮箱                  | 院系       |
| ---- | --------- | --------------------- | ---------- |
| 姜帅 | 221220115 | js13156223725@163.com | 计算机学院 |

---

## 任务一：带词频属性的文档倒排算法

该任务创建了两个Mapreduce job（分别对应java类`InvertedIndex`和`SortByFreuqency`）

### InvertedIndex

- **Map阶段的设计思路**
  
  -  将输入的文本行拆分为单词，并为每个单词生成 `<词语, 文档ID:1>` 的键值对，表示该词语在某个文档中出现一次。
  - Key ：`Text`类型，表示词语（例如"哈利"）
  - Value：`Text`类型，表示文档ID和词频的组合 例如（“第二部-密室:1”）
  - 伪代码：
  
    ```pseudocode
    Function Begin Map(key: Object, value: Text, context: Context)
        // get file name
        docName = get file name from input split
    
        // 将输入行拆分为单词
        words = tokenize(value)
        wordFreqMap = new HashMap<String, Integer>
        
        // Output <word, docName:freq> for each word
        for each (word, freq) in wordFreqMap:
            set word = word
            set docInfo = docName + ":" + freq
            context.write(word, docInfo)
    End Function
    ```
- **Reduce阶段的设计思路**

  - 合某个词语在所有文档中的出现信息，计算平均词频，并输出每个文档的词频。

  - Key：`Text` 类型，表示词语（例如 "哈利"）

  - Value：`Iterable<Text>` 类型，表示该词语在不同文档中的出现记录（例如 ["第一部-魔法石:1", "第一部-魔法石:1", "第二部-密室:1"]）

  - 伪代码：

    ```pseudocode
    Function Reduce(key: Text, values: Iterable<Text>, context: Context)
        初始化统计数据结构 HashMap docFreqMap and docSet
        totalFrequency = 0
        for each value in values:
            (docId, freq) = 分割 value 为 [文档ID, 词频]
            totalFrequency = totalFrequency + freq
            docFreqMap[docId] = docFreqMap[docId] + freq // 累加词频
            docSet.add(docId) // 记录出现过的文档
        df = docSet的大小 // 文档频率
        averageFrequency = totalFrequency / df if df > 0 else 0
    
        output = format("%.2f", averageFrequency) + ", "
        for each (docId, freq) in docFreqMap:
            output = output + docId + ":" + freq + "; "
    
        outputKey = "[" + key + "]"
        context.write(outputKey, output)
    End Function
    ```

- **输出示例**

  ![image-20250327103739329](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327103739329.png)

​	完整的输出文件位于`/user/221220115stu/lab2/output/task1/step1`

### SortByFrequency

 该MapReduce job的目的是对上一个倒排索引任务的输出按平均词频进行**降序排序**，同时应用于任务一和接下来的任务三

- **Map阶段的设计思路**

  - 从输入行中提取平均词频`avgFrequency`，将其取负值作为排序键（因为 MapReduce 默认按键升序排序，取负值可实现降序），并将整行作为值输出

  - Key：`DoubleWritable` 类型，表示负的平均词频（例如 -2.50）

  - Value：`Text` 类型，表示原始输入行（例如 "[word]    2.50, doc1:2; doc2:1"）

  - 伪代码：

    ```pseudocode
    Function Map(key: Object, value: Text, context: Context)
        parts = split(line, "\t", 2)
        // 提取词频部分
        secondPart = parts[1]
        commaIndex = find index of "," in secondPart
        frequencyStr = substring(secondPart, 0, commaIndex)
        
        try:
            // 按照词频，取负，实现降序排序
            frequency = parseDouble(frequencyStr)
            negFrequency = -frequency
            set negAvgFrequency = negFrequency
            set outputValue = line
            // 输出 <negFrequency, original line>
            context.write(negAvgFrequency, outputValue)
        catch NumberFormatException:
            print error "Failed to parse frequency from: " + line
    End Function
    ```

- **Reduce阶段的设计思路**

  - 按键（负平均词频）排序后的值（原始行）直接输出，完成降序排序。

  - 输入<key, value>：`<DoubleWrite, Iterable<Text>>`表示平均词频和对应的原式行

  - 输出<key, value>：`<Text, NullWritable>`, 表示原式行和无附加值

  - 伪代码：

    ```pseudocode
    Function Reduce(key: DoubleWritable, values: Iterable<Text>, context: Context)
        for each value in values:
            context.write(value, NullWritable)
    End Function
    ```

- **输出示例**

  ![image-20250327135801476](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327135801476.png)
  
  完整的输出文件位于`/user/221220115stu/lab2/output/task1/step2`

---

## 任务二：计算TF-IDF

该任务创建了一个MapReduce job，对应java类`CalculateTFIDF`, 目标是计算文本集合中每个词语在每个文档中的 **TF-IDF** 值

- **Map阶段的设计思路**

  - 同任务一，统计每个文档中每个词语的词频（TF），并为每个词语生成 `<词语, 文档名:词频>` 的键值对
  - Key ：`Text` 类型，表示词语（例如"哈利"）
  - Value：`Text` 类型，表示文档名和词频的组合 例如（“第一部-魔法石:1”）
  - 伪代码：同实验一

- **Reduce阶段的设计思路**

  - 根据词频（TF）和IDF计算每个词语在每个文档中的 TF-IDF 值，并输出结果

  - 输入<key, value>：`<Text, Iterable<Text>>` 类型， 表示词语及其在文档中的出现记录

  - 输出<key, value>：`<Text, Text>`类型，表示文档名和它其中的词语及其TF-IDF

  - 伪代码：

    ```pseudocode
    Function Reduce(key: Text, values: Iterable<Text>, context: Context)
        // 初始化数据结构
        docFreqMap = new HashMap<String, Integer>
        docSet = new HashSet<String>
        // 将每个来自Map阶段的单个输入聚合
        for each val in values:
            (docName, freq) = split(val, ":")
            docFreqMap[docName] = parseInt(freq)
            docSet.add(docName)
    
        // Calculate IDF
        df = size of docSet
        idf = log(totalDocs / (df + 1)) / log(2)
    
        // 计算TF-IDF并输出
        for each (docName, tf) in docFreqMap:
            tfidf = tf * idf
            outputValue = key + "," + format(tfidf, 3 decimals)
            context write
    End Function
    ```

- **输出示例**

  <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327140105403.png" alt="image-20250327140105403" style="zoom:67%; margin:0" />

  完整的输出文件位于`/user/221220115stu/lab2/output/task2`

---

## 任务三：去除停用词

该任务的目标是处理文本数据，去除停用词后统计每个词语在每个文档中的词频，按照任务一的格式进行排序后输出，同样创建了两个MapReduce job，第一个对应java类`RemoveStopWords`，第二个java类同任务一，为`SortByFrequency`

### RemoveStopWords

- **Map阶段的设计思路**

  - 从输入文档中提取词语，读取停用词表以过滤停用词，并为每个过滤后的词语生成 <词语, 文档ID:词频> 的键值对。

  - <Key, Value>类型：`<Text, Text>`,表示词语，文档ID和词频的组合

  - 伪代码：

    ```pseudocode
    Function Setup(context: Context)
        // Load stop words from distributed cache
        cacheFiles = get cache files from context
        for each file in cacheFiles:
            if file contains "cn_stopwords.txt":
                open file and read each item into stopwords
        get document id from input
    End Function
    
    Function Map(key: LongWritable, value: Text, context: Context)
        // Split input line into words
        words = split(value, " ")
        for each word in words:
            if word is not empty and not in stopWords:
                wordCounts[word] = wordCounts.getOrDefault(word, 0) + 1
    End Function
    
    Function Cleanup(context: Context)
        // Output word counts for the document
        for each (word, count) in wordCounts:
            context.write(word, docId + ":" + count)
    End Function
    ```

- **Reduce阶段的设计思路**

  - 聚合某个词语在所有文档中的出现信息，计算平均词频，并输出每个文档的词频。

  - 输入<key, value>：`<Text, Iterable<Text>>` 类型， 表示词语及其在文档中的出现记录

  - 输出<key, value>：`<Text, Text>`类型, 对应[词语]（Key），平均词频和每个文档的词频（Value）

  - 伪代码：

    ```pseudocode
    Function Reduce(key: Text, values: Iterable<Text>, context: Context)
        初始化统计数据结构 HashMap: docFreqMap and docSet
    	
    	// 统计词频
        for each val in values:
            (docId, freq) = split(val, ":")
            freqValue = parseInt(freq)
            totalFrequency = totalFrequency + freqValue
            docFreqMap[docId] = docFreqMap.getOrDefault(docId, 0) + freqValue
            docSet.add(docId)
    
        // 计算平均词频
        df = size of docSet
        averageFrequency = totalFrequency / df if df > 0 else 0
    
        // 构建输出
        output = format(averageFrequency, 2 decimals) + ", "
        for each (docId, freq) in docFreqMap:
            output = output + docId + ":" + freq + "; "
        if size of docFreqMap > 0:
            remove last "; " from output
    
        // 输出结果
        outputKey = "[" + key + "]"
        context.write(outputKey, output)
    End Function
    ```

- **输出示例**

  <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327141132156.png" alt="image-20250327141132156" style="zoom:120%;" />

	> [!NOTE]
	>
	> *作为对比：左侧为去除停用词前，右侧为去除停用词后，可以看到停用词“的”，“的确”均被去除*	

​	完整输出文件见`/user/221220115stu/lab2/output/task3/step1`

- **排序后的输出示例**

  ![image-20250327141423975](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327141423975.png)

​	完整输出文件见`/user/221220115stu/lab2/output/task3/step2`

---

## 执行报告

- **任务一：InvertedIndex & SortByFrequency**

  ![image-20250327142805481](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327142805481.png)

- **任务二：CalculateTFIDF**

  ![image-20250327142827238](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327142827238.png)

- **任务三：RemoveStopWord & SortByFrequency**

  ![image-20250327142858872](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250327142858872.png)
