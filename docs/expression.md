## 配置表达式运算支持

USKit定义的配置中，支持进行表达式运算，表达式所具备的表达能力如下：

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

1. json_encode(json)：对json值进行序列化

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

6. slice(json[, start, end])：从json数组中提取[start, end)指定的元素(不包括end)，比如start=1, end=3, 表示抽取索引为1,2的元素

    参数：

    * json：json数组
    * start：从该索引开始提取json数组的元素，默认为0，如果该参数为负数，表示从json数组的倒数第几个元素开始提取
    * end：在该索引处停止提取json数组的元素，默认为数组长度，如果该参数为负数，表示在json数组的倒数第几个元素停止提取

    返回：json数组

    样例：`slice([1, 2, 3, 4], 2, 3)`

    输出：`[3]`

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

#### 函数自定义

用户可以通过C++编写满足制定签名(详见代码function_manager.h中的FuntionPtr)的函数，并在global.cpp中进行注册添加至function_manager，即可在配置中进行调用

用户可以参考function/builtin.cpp文件中的内置函数定义，实现自定义函数，具体实现方法可以参考[自定义函数和策略](docs/custom.md)

注意：自定义函数需要负责进行调用参数和返回参数的类型、个数检查等

### 嵌套作用域(scope)

表达式进行求值的过程中，需要依赖于上下文(context)，USKit支持变量嵌套作用域求值，在变量求值时，会先在当前配置中定义的局部作用域搜索，如果没有找到，则往上搜索更外层的作用域，直到最外层作用域。具体会产生局部作用域的配置项请参见[详细配置项说明](config.md)
