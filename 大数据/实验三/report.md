# 第三次实验报告

| 姓名 | 学号      | 邮箱                    | 院系       |
| ---- | --------- | ----------------------- | ---------- |
| 姜帅 | 221220115 | jsissosix1221@gmail.com | 计算机学院 |

---

## 任务一: HBase 安装及命令行操作

### 0️⃣**安装Zookeeper并完成配置**

**下载Zookeeper：**首先，从 Apache 官方网站下载 ZooKeeper 3.4.12 的 tarball 文件。下载完成后解压

```bash
wget https://archive.apache.org/dist/zookeeper/zookeeper-3.4.12/zookeeper-3.4.12.tar.gz
tar -xzf zookeeper-3.4.12.tar.gz
```

**配置Zookeeper：**进入安装目录并创建一个配置文件

```
cd /home/js/Zookeeper/zookeeper-3.4.12/conf
cp zoo_sample.cfg zoo.cfg
```

编辑`zoo.cfg`文件，将dataDir修改为自己创建的路径，例如：

```bash
# ...
dataDir=/home/js/Zookeeper/zookeeper-3.4.12/zkdata
# ...
```

**启动Zookeeper：**进入 ZooKeeper 的 `bin` 目录并启动服务

```
cd /home/js/Zookeeper/zookeeper-3.4.12/bin
./zkServer.sh start
```

可以看到以下输出：

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250408192738641.png" alt="image-20250408192738641" style="zoom: 80%; margin: 0px;" />

使用 ZooKeeper 客户端连接测试： 输入`./zkCli.sh`,如下图所示可以进入Zookeeper的命令行界面, 运行命令（`ls /`）, 安装成功.

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250408192915383.png" alt="image-20250408192915383" style="zoom:67%; margin:0" />

### 1️⃣安装HBase并完成配置

**下载HBase：**从Apache官网下载HBase-2.4.0的压缩包，解压下载的文件

```bash
wget https://archive.apache.org/dist/hbase/2.4.0/hbase-2.4.0-bin.tar.gz
tar -xzf hbase-2.4.0-bin.tar.gz
```

编辑`~/.bashrc`文件，在文件末尾添加以下内容

```bash
export HBASE_HOME=/home/js/HBase/hbase-2.4.0
export PATH=$PATH:$HBASE_HOME/bin
```

使更改生效：`source ~/.bashrc`, 通过`hbase version` 验证安装是否成功，如图：

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250408220908703.png" alt="image-20250408220908703" style="zoom:80%;margin:0" />

**配置HBase：**

1. 编辑`conf/hbase-env.sh`,在末尾添加如下内容：

   <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250408221308486.png" alt="image-20250408221308486" style="zoom:80%;margin:0" />

2. 编辑hbase-site.xml:

   <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250408222006607.png" alt="image-20250408222006607" style="zoom:67%; margin:0" />

3. 验证HBase是否运行：启动HBase，输入jps检查进程，结果如下（需要先启动HDFS和ZooKeeper）

   <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250408222629831.png" alt="image-20250408222629831" style="zoom:80%; margin:0" />

   可以看到HMaster和HRegionServer, HBase服务正常启动
   
   在浏览器中访问本地的16010端口，成功访问：
   
   <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410114910619.png" alt="image-20250410114910619" style="zoom:80%;" />

### 2️⃣进入HBase shell命令行，熟悉HBase基础操作

启动HBase后输入hbase shell进入命令行并输入`status`查看状态：

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250409193343262.png" alt="image-20250409193343262" style="zoom:80%;" />

- 创建表，列出所有表，获取表的描述：

  <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250409193944185.png" alt="image-20250409193944185" style="zoom:67%; margin:0" />

- 删除列族，删除表：

  <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250409194617124.png" alt="image-20250409194617124" style="zoom:67%; margin:0" />

- 插入、更新、扫描、删除数据：

  <img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250409195126466.png" alt="image-20250409195126466" style="zoom:80%; margin:0" />

- 此外，可以使用 `help` 命令查看所有命令或某个具体命令的帮助

### 3️⃣Hbase与关系型数据库

#### **设计并创建表：**

对于课程表（course），行键采用C_No, 列族为info，列限定符为name和credit。通过create创建表然后使用put插入数据，插入后通过scan扫描，具体操作

```hive
create 'course', 'info'

put 'course', '123001', 'info:name', 'Math'
put 'course', '123001', 'info:credit', '4.0'

put 'course', '123002', 'info:name', 'English'
put 'course', '123002', 'info:credit', '3.0'

put 'course', '123003', 'info:name', 'Big Data'
put 'course', '123003', 'info:credit', '4.0'
```

可以看到该表已成功创建：

![image-20250410120000689](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410120000689.png)

 对于学生表（student），行键为S_No, 列族info下的具体字段为name, sex, age, 创建方式同上，结果如下：

![image-20250410120418990](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410120418990.png)

对于选课表(enrollment), 行键采用`S_No:C_No`的组合, 列族info, info:score 存储成绩, 对应hbase指令以及结果:

```hive
create 'enrollment', 'info'

put 'enrollment', '2025001:123001', 'info:score', '68'
put 'enrollment', '2025001:123002', 'info:score', '90'
put 'enrollment', '2025001:123003', 'info:score', '96'
...
```

![image-20250410121132878](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410121132878.png)

#### 查询选修Big Data的学生的成绩

首先在course表中通过课程名获取Big Data对应的课程号:

```hive
scan 'course', { FILTER => "SingleColumnValueFilter('info', 'name', =, 'binary:Big Data')" }
```

![image-20250410122039903](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410122039903.png)

可以看到Big Data的课程号为 `123003`

再在选课表中通过RowFilter查找行键中包含课程号为123003的记录:

```hive
scan 'enrollment', { FILTER => "RowFilter(=, 'substring::123003')" }
```

![image-20250410122229806](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410122229806.png)

可以看到学生2025001选修了Big Data课程, 成绩为 96

#### 学生表增加联系方式

从表中可以看到学生表新增了email这一字段, 只需要在info列族中增加 `info:email`即可, 直接使用put命令:

```hive
put 'student', '2025001', 'info:email', 'lilei@qq.com'
put 'student', '2025002', 'info:email', 'hmm@qq.com'
put 'student', '2025003', 'info:email', 'zl@qq.com'
put 'student', '2025004', 'info:email', 'lm@qq.com'
```

![image-20250410131402741](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410131402741.png)

#### 查询Zhang Li的联系方式

在student表中使用 SingleColumnValueFilter 过滤器查找 info:name 等于 "Zhang Li" 的记录

```hive
scan 'student', { FILTER => "SingleColumnValueFilter('info', 'name', =, 'binary:Zhang Li')" }
get 'student', '2025003', { COLUMN => 'info:email' }
```

![image-20250410133757420](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410133757420.png)

![image-20250410133944802](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410133944802.png)

从结果中可以看到, 姓名''Zhang Li''对应的联系方式是zl@qq.com

这种方法的缺陷是需要扫描整个表,效率较低,为了更高效的按姓名查询,可以创建一个辅助表 student_by_name，以姓名作为行键，存储学号等信息.

#### 删除创建的表

在 HBase 中，删除表需要先禁用表（disable），然后才能执行删除操作（drop）

首先确认当前有哪些表: `hbase: > list`

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410134153503.png" alt="image-20250410134153503" style="zoom: 67%; margin: 0px;" />

我们需要删除的表是 `course`、`enrollment` 和 `student`, 输入命令:

```hive
disable 'course'
drop 'course'

disable 'student'
drop 'student'

disable 'enrollment'
drop 'enrollment'
```

再次运行 list 命令，确认表已删除：

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410134327186.png" alt="image-20250410134327186" style="zoom:67%; margin:0" />

---

## 任务二: MapReduce编程与Hive外部表管理

### MapReduce编程

我们需要编写一个 MapReduce 程序来处理 IOlog.trace 数据，筛选 op_name=2 的记录，并按 user_namespace 统计 op_count 的总和

在Map阶段, 我们对每行数据按照空格分隔,获取 op_name 和 user_namespace, op_count, 筛选op_name = 2的记录, 以`<userNamespace, opCount>` 的格式输出

在Reduce阶段, 对同一个namespace的opcout进行累加, 按照`<user_namespace, sum(op_count)>`格式输出.

编写完成后输入 `mvn clean install` 打包成jar包, 将IOlog.trace上传到HDFS, 输入以下命令运行:

```bash
hadoop jar op-count-1.0-SNAPSHOT.jar com.example.IOlogTraceProcessor /user/js/opcount/input/IOlog.trace /user/js/opcount/output
```

任务运行结束后, 输入 `hdfs dfs -cat /user/js/opcount/output/part-r-00000` 查看输出结果(部分):

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410145107475.png" alt="image-20250410145107475" style="zoom:80%; margin:0" />

Yarn监控截图

![image-20250411153415998](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411153415998.png)

### 在 Hive 中创建表映射 MapReduce 输出

#### 安装并配置Hive

安装,解压并配置bashrc:

```bash
wget https://archive.apache.org/dist/hive/hive-3.1.2/apache-hive-3.1.2-bin.tar.gz
tar -xzf apache-hive-3.1.2-bin.tar.gz
sudo mv apache-hive-3.1.2-bin /home/js/hive

nano ~/.bashrc

# 在文件末尾添加以下内容
export HIVE_HOME=/home/js/hive
export PATH=$PATH:$HIVE_HOME/bin

# 使更改生效
source ~/.bashrc
```

验证环境变量和hive版本: 

```bash
echo $HIVE_HOME
hive --version
```

![image-20250410150212422](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410150212422.png)

配置hive: 进入hive的conf目录, 创建`hive-site.xml`文件, 添加如下的配置信息, 并在HDFS上创建目录: `hdfs dfs -mkdir -p /user/hive/warehouse`

```xml
<configuration>
    <property>
        <name>hive.metastore.warehouse.dir</name>
        <value>hdfs://localhost:9000/user/hive/warehouse</value>
    </property>
    <property>
        <name>javax.jdo.option.ConnectionURL</name>
        <value>jdbc:derby:;databaseName=metastore_db;create=true</value>
    </property>
    ...
</configuration>
```

初始化元数据: `$HIVE_HOME/bin/schematool -initSchema -dbType derby`

启动Hive CLI: `hive`

在Hive Shell中运行以下命令:

```hive
show databases;
create table test (id int, name string);
show tables;
```

结果如下图所示, 可以看到Hive正常运行, 安装成功.

![image-20250410154046268](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250410154046268.png)

#### 在Hive中创建表管理 MapReduce 的输出数据

MapReduce 输出存储在 HDFS 的 `/user/js/opcount/output` 目录中, 我们需要通过 Hive 创建一个外部表映射这个数据, 进入 Hive Shell, 输入以下指令创建表:

```hive
CREATE EXTERNAL TABLE IOlog_output (
    user_namespace STRING,
    sum_op_count BIGINT
)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY ','
STORED AS TEXTFILE
LOCATION 'hdfs://localhost:9000//user/js/opcount/output';
```

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411124324804.png" alt="image-20250411124324804" style="zoom:67%; margin:0" />

输入以下命令查看所有数据

```hive
SELECT * FROM IOlog_output;
```

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411124450913.png" alt="image-20250411124450913" style="zoom:67%; margin:0" />

## 任务三:Hive分区分桶表与HQL实践

### 1️⃣Hive中创建分区表

在Hive中创建一张分区表 `IOlog_part_221220115`, 以命名空间(`user_namespace`)为分区条件

```hive
CREATE TABLE iolog_part_221220115 (
    block_id STRING,
    io_offset STRING,
    io_size STRING,
    op_time STRING,
    op_name STRING,
    user_name STRING,
    rs_shard_id STRING,
    op_count BIGINT,
    host_name STRING
)
PARTITIONED BY (user_namespace STRING)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY ' '
STORED AS TEXTFILE;
```

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411140019685.png" alt="image-20250411140019685" style="zoom:50%; margin:0" />

### :two:Hive中创建分桶表

创建分桶表 `IOlog_trace_221220115`

```hive
CREATE TABLE IOlog_trace_221220115 (
    block_id STRING,
    io_offset STRING,
    io_size STRING,
    op_time STRING,
    op_name STRING,
    user_namespace STRING,
    user_name STRING,
    rs_shard_id STRING,
    op_count BIGINT,
    host_name STRING
)
CLUSTERED BY (block_id) INTO 3 BUCKETS
ROW FORMAT DELIMITED
FIELDS TERMINATED BY ' '
STORED AS TEXTFILE;
```

- `CLUSTERED BY (block_id) INTO 3 BUCKETS`：指定以 `block_id` 作为分桶字段，分桶数量为 **3**。

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411140146585.png" alt="image-20250411140146585" style="zoom:50%; margin:0" />

### :three:导入数据到分区表分桶表

- 导入任意5条数据到分区表, 在向分区表中插入数据时，需要指定分区列的值。（首先需要启用动态分区）

  ```hive
  SET hive.exec.dynamic.partition=true;
  SET hive.exec.dynamic.partition.mode=nonstrict;
  ```

  ```hive
  INSERT INTO iolog_part_221220115
  PARTITION (user_namespace='xxx')
  VALUES (1, 'data1'), (2, 'data2'), (3, 'data3'), (4, 'data4'), (5, 'data5'), (6, 'data6'), (7, 'data7'), (8, 'data8'), (9, 'data9');
  ```

  我们将以下五行数据按上述格式插入到分区表中: (2203126710 * 1, 2203126711 * 3, 2203126715* 1)

  ```
  2714921523 0 8388608 1679209092 2 2203126710 918076232 5 1 3550113875
  2714826509 0 4038842 1679209093 5 2203126711 918076230 8 1 3550112758
  2714815266 0 256 1679209094 2 2203126711 918076231 0 1 3550112742
  2714980205 0 8388608 1679209095 5 2203126711 918076230 10 1 3550114154
  2714911581 1563335 4948276 1679209100 2 2203126715 918076229 8 1 3550113732
  ```

  以其中第一条数据为例, 运行过程如下:

  ![image-20250411145528620](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411145528620.png)

​	Hive 默认使用 MapReduce 作为执行引擎, 会将操作转换为 MapReduce 任务, 日志中的 Total jobs = 3 表示 Hive 将插入操作	分解为 3 个 MapReduce 作业. 按照同样的操作插入剩余四条数据.

- 验证导入是否成功： 

  ```hive
  SHOW PARTITIONS iolog_part_221220115;
  SELECT * FROM iolog_part_221220115;
  ```

  ![image-20250411145958424](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411145958424.png)

​	可以看到5条数据成功插入

- 从HDFS将IOlog.trace导入到分桶表中 （需要启动分桶写入`SET hive.enforce.bucketing=true;`）
  首先创建一个临时表IOlog_temp加载IOlog.trace数据:
  
  ```hive
  CREATE TABLE IOlog_temp (
      block_id STRING,
      io_offset STRING,
      ...
  )
  ROW FORMAT DELIMITED
  FIELDS TERMINATED BY ','
  STORED AS TEXTFILE
  LOCATION 'hdfs://localhost:9000//user/IOlog/';
  ```
  
  ```hive
  INSERT OVERWRITE TABLE iolog_trace_221220115
  SELECT * FROM iolog_temp;
  ```
  
  ![image-20250411150925322](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411150925322.png)

### :four:HQL查询

- 查询分区表中某个分区下的所有数据

  这里选择查询`user_namespace='2203126711'`的所有数据, 应该出现三条. 结果如下:

  ```hive
  SELECT * FROM iolog_part_221220115 WHERE user_namespace='2203126711';
  ```

  ![image-20250411151327045](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411151327045.png)

- 查询分桶表中每个用户有几个不同的主机地址(host_name)

  使用查询语句：

  ```hive
  SELECT user_name, COUNT(DISTINCT host_name) AS host_count
  FROM iolog_trace_221220115
  GROUP BY user_name;
  ```

  验证结果：

  ![image-20250411152013858](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411152013858.png)

- 查询分桶表中每个命名空间下所有用户的写操作(op_name=2)的总次数

  使用HQL查询语句：

  ```hive
  SELECT user_namespace, SUM(op_count) AS total_write_count
  FROM iolog_trace_221220115
  WHERE op_name = '2'
  GROUP BY user_namespace;
  ```

  验证结果：

  ![image-20250411153019009](C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250411153019009.png)

