# 实验五：中间代码优化 实验报告

| 成员   | 学号      | 院系       | 邮箱                    |
| ------ | --------- | ---------- | ----------------------- |
| 张路远 | 221220144 | 计算机学院 | craly199@foxmail.com    |
| 姜帅   | 221220115 | 计算机学院 | jsissosix1221@gmail.com |

## 一. 任务描述

通过完成数据流分析中的`Reaching Definition` / `Live Variables` / `Available Expressions` / `Const Propagation`以及迭代算法和`worklist`算法，对中间代码进行优化，包括：全局公共子表达式消除，全局常量传播和全局无用代码消除（选作1）

框架代码中需要我们实现的有：

- 可用表达式分析
- 活跃变量分析
- 复制传播
- 常量传播
- 迭代算法和`worklist`算法驱动的`backword`求解器

## 二. 任务实现

根据实验手册中的全局优化管道：常量传播$\rarr$公共子表达式消除$\rarr$无用代码消除$\rarr$循环不变代码外提$\rarr$归纳变量强度消减，我们需要完成的为前三步，需要基于任务描述中的几种数据流分析的模式进行优化。

### 可用表达式/活跃变量/复制传播

对于前三项任务，即可用表达式分析，活跃变量分析，复制传播，对应着数据流分析中的`Available Expressions`, `Live Variables`和`Reaching Definition`，可以参照以下表格：

<img src="C:\Users\Jsissosix\AppData\Roaming\Typora\typora-user-images\image-20250529145038640.png" alt="image-20250529145038640" style="zoom: 67%; margin:0" />

- 在复制传播中，需要维护`use_def`链和`def_use`链，这体现在`transferStmt`中：

  - **copy_kill：**当一个变量被重新定义时，移除与该变量相关的旧的复制关系

    ```c
    if(VCALL(fact->def_to_use, exist, new_def)) {
        IR_var use = VCALL(fact->def_to_use, get, new_def);
        VCALL(fact->def_to_use, delete, new_def);
        VCALL(fact->use_to_def, delete, use);
    }
    if(VCALL(fact->use_to_def, exist, new_def)) {
        IR_var def = VCALL(fact->use_to_def, get, new_def);
        VCALL(fact->def_to_use, delete, def);
        VCALL(fact->use_to_def, delete, new_def);
    }
    ```

  - **copy_gen：**对于赋值语句（`IR_ASSIGN_STMT`），如果右边是变量（非常量），生成新的复制关系。

    ```c
    VCALL(fact->def_to_use, set, def, use);
    VCALL(fact->use_to_def, set, use, def);
    ```

- 在活跃变量分析中，Optimize阶段，针对操作语句或赋值语句，如果该语句定义的变量的definition在语句的出口处不活跃（当前程序点之后不会被任何语句所使用），则可以将该语句标记为死代码。

  ```c
  if(VCALL(*new_out_fact, exist, def) == false) {
      stmt->dead = true;
      updated = true;
  }
  ```

### 常量传播

按照文档和框架代码的提示填写即可

需要注意的是，在计算二元表达式的值时，对于输入的`CPValue`类型的`v1`和`v2`，如果`v1`和v2任意一方为`NAC`，**不可以**直接返回`NAC`，而是先判断该二元运算为除法:heavy_division_sign:且`v2`为`0`的情况，在这种情况下应该优先返回**UNDEF**而不是`NAC`

此外，在`transferStmt`中，如果一个产生新的definition的语句并非`IR_ASSIGN_STMT`也并非`IR_OP_STMT`，例如： 

```c
x = f(...)
```

那么x在格上的value应为**NAC**，这是出于soundness的考虑

### 后向分析求解器

仿照前向分析迭代求解器和worklist求解器的结构来写，需要注意的是：后向分析meet的是当前节点所有后继节点的InFact，相应的，当一个节点的InFact发生变化时，需要将该节点的所有前驱加入到worklist中。

