## 配置表达式运算支持

US Kit定义的配置中，支持进行表达式运算，表达式所具备的表达能力如下：

### 字面量(literal)

支持以下类型的字面量定义：

| 类型    | 说明                | 样例                              |
| ------- | ------------------- | --------------------------------- |
| int     | C++中的int          | 123/-123                          |
| double  | C++中的double       | 1.23                              |
| string  | 使用' (单引号)包围  | 'hello'                           |
| boolean | 布尔值              | true/false                        |
| array   | 数组                | [ 1, 'hello', true ]              |
| dict    | 对象                | {'a':1, 'b': 'hello', 'c' : true} |
| null    | null值              | null                              |

### 数值四则运算

对数值支持 `+`，`-`，`*`，`/` 四种运算

### 逻辑运算

对boolean类型支持 `&&`，`||`，`! `三种运算

### 比较运算

所有类型支持 `==`，`!=`

数值类型支持` >`，`<`，`>=`，`<=`

### 位运算

对boolean和int类型支持 `&`，`|`两种运算

### 字符串运算

对string类型支持字符串拼接(`+`)运算

### 数组运算

对array类型支持合并(`+`)，交集(`&`)，并集(`|`)和差集(`-`)四种运算

### 条件运算

支持expr1 ? expr2 : expr3格式的条件运算，如果expr1求值为true，那么返回expr2的求值结果，否则返回expr3的求值结果

### 变量求值

通过配置def语法定义的局部变量foo，可以通过$foo的形式获得变量值，未定义值返回null。

此外，具体配置中还有一些特殊系统变量$request、$response、$recall、$rank等，具体说明参见[配置详细说明](config.md)。

### 函数调用

通过func(arg1, arg2,...)的形式进行调用

#### 内置函数

1. json_encode(json)：对json值进行序列化，当 json 对象为字面量（literal）时，等同于 to_string() 方法

   参数：

   * json：json值

   返回：序列化后的json字符串

   样例：`json_encode({'a':1})`

   输出：`'{"a" : 1}'`

2. json_decode(json_str)：对json字符串进行反序列化，是json_encode的逆操作

   参数：

   * json_str：序列化后的json字符串

   返回：json对象/数组

   样例：`json_decode('{"a":1}')`

   输出：`{"a" : 1}`

3. get(json, path[, default_value=null])：从json对象/数组中获取指定路径的值

   参数：

   * json：json对象/数组
   * path：采用Unix文件系统的路径表示形式来唯一指定json中的值(使用/作为分隔符)
   * default_value(optional)：当path对应的路径没有值时，返回的默认值，该参数为可选参数，如不指定则默认返回null

   返回：json值

   样例：`get({'a': {'b' : 1}}, '/a/b', 0)`

   输出：`1`

   TIPS: 
   
   * 获取请求参数中的值：`get($request, 'key')`
   * 获取已发送并且得到返回结果的所有后端结果：`get($backend, '\')`
   * 获取指定service的返回结果：`get($backend, 'service_name')`, service_name 为backend.conf 中为 service 定义的 name，如果 service 尚未发送或返回结果，则返回 null
   * 获取已执行完成的flow node的输出结果：`get($result, '\')`
   * 获取已执行完成的flow node的 output 中 key 为 xxx的值：`get($result, 'xxx')`
   * 在service中配置，一定会导致请求出错（因而请求自动取消）：`get($NOEXIST, 'whateverulike')`， `$NOEXIST` 可以为任意未定义变量名

4. has(json, key)：判断一个json对象/数组是否包含对应的key

   参数：

   * json：json对象
   * key：字符串

   返回：true/false

   样例：`has({'a' : 1}, 'a')`

   输出：`true`

5. len(json)：返回json数组的长度

   参数：

   * json：json数组

   返回：json数组的长度

   样例：`len([1, 2, 3])`

   输出：`3`

6. slice
    * slice(json[, start, end])：从json数组中提取[start, end)指定的元素(不包括end)，比如start=1, end=3, 表示抽取索引为1,2的元素

        参数：

        * json：json数组
        * start：从该索引开始提取json数组的元素，默认为0，如果该参数为负数，表示从json数组的倒数第几个元素开始提取
        * end：在该索引处停止提取json数组的元素，默认为数组长度，如果该参数为负数，表示在json数组的倒数第几个元素停止提取

        返回：json数组

        样例：`slice([1, 2, 3, 4], 2, 3)`

        输出：`[3]`

    * slice(json, index_set): 从json数组中提取index不在index_set中的元素

        参数：
        * json：json 数组
        * index_set：待删除元素的index

        返回：json 数组

        样例：`slice(['a', 'b', 'c', 'd'], [0, 3])`

        输出：`['b', 'c']`

    * slice(json, slice_args): 从json数组中删除或提取满足条件的元素
        
        参数：

        * json：json 数组
        * slice_args：json 对象，key 必须包含"method","path" 以及"keys"，对应的 value类型分别为：ENUM('AND', 'EXCLUSIVE')， string 以及 json 数组。

        返回：json 数组

        样例：`slice([{'from':'others', 'score':1}, {'from':'unit', 'score':99}, {'from':'default', 'score':0 }], {'method':'AND', 'path': 'from', 'keys': ['others', 'dueros']})`

        输出：`[{'from':'others', 'score':1}]`

        样例：`slice([{'from':'others', 'score':1}, {'from':'unit', 'score':99}, {'from':'default', 'score':0 }], {'method':'EXCLUSIVE', 'path': 'from', 'keys': ['others', 'dueros']})`

        输出：`[{'from':'unit', 'score':99}, {'from':'default', 'score':0 }]`



7. int(str_or_bool)：将数值字符串或者bool值转换为int值, 当输入为true时转换为1, false时为0, 可以作为指示函数使用

    参数：

    * str_or_bool：数值字符串或者bool值(true或者false)

    返回：int数值

    样例：`int(true)`, `int('1234')`

    输出：`1, 1234`

8. bool(json)：判断给定值是否为空，当输入为null，false，0，0.0，''，空数组，空对象时，返回false，否则返回true

    参数：

    * json：json值

    返回：true/false

    样例：`bool('')`, `bool(0)`, `bool('abc')`

    输出：`false, false, true`

9. md5(str)：计算给定字符串的md5签名

    参数：

    * str：需要计算md5签名的字符串

    返回：str对应md5签名的16进制字符串

    样例：`md5('hello')`

    输出：`'5d41402abc4b2a76b9719d911017c592'`

10. sha1(str)：计算给定字符串的sha1签名

    参数：

    * str：需要计算sha1签名的字符串

    返回：str对应sha1签名的16进制字符串

    样例：`sha1('hello')`

    输出：`'aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d'`

11. time()：返回当前时间的Unix时间戳

    返回：当前时间的Unix时间戳

    样例：`time()`

    输出：`1540800540`

12. set(json, path, value): 对 json 的指定路径赋值
    
    参数：

    * json: 需要赋值的对象
    * path：赋值的路径（UNIX-Like）
    * value：输入值

    返回：赋值后的 json 体（参数中的 json 保持不变）

    样例：`set({'test':{'test_key':'test_value'}}, '/test/test_key', {'res_key':'res_value'})`

    输出：`{'test':{'test_key':{'res_key':'res_value'}}}`


13. index_at(json, path, keyvalue): 返回 json 数组中在路径 path 上的值与 keyvalue 相等的元素的位置（并转化为字符串）
    
    参数：

    * json：json 数组
    * path：进行keyvalue对比的路径，根路径为每个元素的根路径
    * keyvalue：对比的值
    
    返回：元素的位置（字符串）, 不存在返回`‘-1’`

    样例：`index_at([{'origin':'12'}, {'origin':'12345'}], '/origin', '12345')`

    输出：`‘1’`


14. foreach_get(json, path[, default_value=null]): 遍历 json 数组，从每个元素中获取指定路径的值并组成新的数组返回
    
    参数：

    * json：json 数组
    * path：指定路径
    * default_value(optional)：当path对应的路径没有值时，返回的默认值，该参数为可选参数，如不指定则默认返回null
    
    返回：json 数组

    样例：`foreach_get([{'origin':'12'}, {'origin':'12345'}, {'key':'value'}], '/origin', '')`

    输出：`['12', '12345', '']`

15. foreach_set(json, path, value): 遍历 json 数组，对每个元素在指定路径赋值并组成新的数组返回
    
    参数：

    * json：json 数组
    * path：指定路径
    * value：赋值，value 为数组类型时，需要与json 数组长度一致，并一一对应赋值
    
    返回：新的 json 数组（原数组不变）

    样例：`foreach_set([{'origin':'12'}, {'origin':'12345'}, {'key':'value'}],'/origin', '55555')`

    输出：`[{'origin':'55555'}, {'origin':'55555'}, {'key':'value', 'origin':'55555'}]`

    样例：`foreach_set([{'origin':'12'}, {'origin':'12345'}, {'key':'value'}],'/origin', ['1', '2', '3'])`

    输出：`[{'origin':'1'}, {'origin':'2'}, {'key':'value', 'origin':'3'}]`

16. translate(json, path, dict): 遍历 json 数组，将每个元素在指定路径的值根据词典 dict 替换成新的值（dict的key中不存在元素指定位置的值或指定路径不存在则不替换），组成新的数组返回
    
    参数：

    * json：json 数组
    * path：指定路径
    * dict: 替换词典
    
    返回：新的 josn 数组（原数组不变）

    样例：`translate([{'intent':'SYS_OTHER'}, {'intent':'SUCCESS'}, {'origin':'12345'}], 'intent', {'SYS_OTHER': 'FAILED'})`

    输出：`[{'intent':'FAILED'}, {'intetn':'SUCCESS'}, {'origin':'12345'}]`

17. strhas(source, target): 判断target字符串是否是source字符串的字串
    
    参数：

    * source：字符串
    * target：字符串
    
    返回：bool 值

    样例：`strhas('小度你好', '小度')`

    输出：`true`

18. substr(source, target[, is_contain=false, is_reverse=false]): 提取source中从target开始到末尾的子字符串
    
    参数：

    * source：字符串
    * target：字符串
    * is_contain(optional)：返回子字符串是否包含 target
    * is_reverse(optional)：是否从右向左开始查找
    
    返回：字符串

    样例：`substr('this is a test string', 'is', true, true)`

    输出：`'is a test string'`

19. replace_all(str, from_str, to_str): 将str中所有的from_str替换成to_str（非递归，以替换部分不会进行第二次替换）。
    
    参数：
    
    * str, from_str, to_str: 字符串
    
    返回：替换后的字符串

    样例：`replace_all('小度小度在吗','小度','')`

    输出：`'在吗'`

20. array_func(json, args): 遍历数组，将每一个元素作为由args指定的自定义函数的第一个参数，执行该函数并将结果按顺序加入到返回的新数组中
    
    参数：

    * json：json 数组
    * args：数组，第一个元素为需要执行的函数名，第二个元素为数组，定义需要执行函数用到的第二到第 N 个参数
    
    返回：每个元素执行指定函数返回的结果所组成的数组

    样例：`array_func(['小度是谁'，'小度小度你好呀'，'小度'], ['replace_all', '小度', '百度'])`

    输出：`['百度是谁'， '百度百度你好呀'， '百度']`
  
#### 函数自定义

用户可以通过C++编写满足制定签名(详见代码function_manager.h中的FuntionPtr)的函数，并在global.cpp中进行注册添加至function_manager，即可在配置中进行调用

用户可以参考function/builtin.cpp文件中的内置函数定义，实现自定义函数，具体实现方法可以参考[自定义函数和策略](docs/custom.md)

注意：自定义函数需要负责进行调用参数和返回参数的类型、个数检查等

### 嵌套作用域(scope)

表达式进行求值的过程中，需要依赖于上下文(context)，US Kit支持变量嵌套作用域求值，在变量求值时，会先在当前配置中定义的局部作用域搜索，如果没有找到，则往上搜索更外层的作用域，直到最外层作用域。具体会产生局部作用域的配置项请参见[详细配置项说明](config.md)
