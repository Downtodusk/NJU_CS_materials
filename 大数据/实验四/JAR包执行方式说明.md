jar包task1-1.0-SNAPSHOT.jar下有三个类: `MutualFriends`, `CommonFollowers`, `FriendRecommendation` 分别对应本次三个任务, 执行命令如下:

Task1:

```bash
hadoop jar /path/to/task1-1.0-SNAPSHOT.jar com.example.MutualFriends <input_path> <output_path>
```

Task2:

```bash
hadoop jar /path/to/task1-1.0-SNAPSHOT.jar com.example.CommonFollowers <output_path_of_task1> <input_path (same as task1)> <x> <output1_path> <output2_path>
```

Task3:

```bash
hadoop jar /path/to/task1-1.0-SNAPSHOT.jar com.example.FriendRecommendation <output_path_of_task1> <input_path (same as task1)> <tmp_output_of_job1> <output_path>
```

