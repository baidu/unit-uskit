## 详细配置说明

USKit的配置采用protobuf的text format格式。

注：下面配置项说明中，名字带有*的表示可以同时定义多个

### us.conf

USKit服务的主配置，位于`conf/`目录下，主要用于指定对话中控根目录以及需要加载的对话机器人中控。

一个`us.conf`样例：

```
root_dir : "./conf/us/"

load : "demo"
load : "test"
```

| 配置项       | 类型   | 必须 | 说明                                                         |
| ------------ | ------ | ---- | ------------------------------------------------------------ |
| root_dir     | string | 否   | 机器人中控配置的根目录路径，默认为`"./conf/us/"`，路径相对于_build目录 |
| load*        | string | 否   | 声明需要加载的对话中控id，如果`root_dir`下没有该id对应目录，则USKit服务会启动失败。 |

USKit配置的灵活性在于配置项可以支持表达式运算，提供了一套配置层面的DSL(领域特定语言)，可以根据不同的用户请求、后端远程调用结果、技能排序结果来动态生成相应的配置，并根据生成的配置执行相应的处理得到最终结果。具体的表达式语法可以参见[表达式运算支持](expression.md)

下面依次对`backend.conf`，`rank.conf`和`flow.conf`三个配置文件进行详细说明：

### 全局变量

在`backend.conf`，`rank.conf`，`flow.conf`中，可以访问到以下全局变量：

* $request：用于表示用户的请求参数，比如可以通过`get($request, 'query')`获取用户的query
* $result：输出结果，对应于最终JSON结果中的result字段
* $backend：保存了所有成功处理的后端远程调用的结果，比如可以通过`get($backend, 'get_session')`来获取名为get_session的远程服务调用结果

### KVE配置

KVE用于定义变量名及其对应的取值，取值可以是直接字符串，也可以是表达式(取值为表达式的求值结果)。在`backend.conf`，`rank.conf`，`flow.conf`中均大量使用KVE配置定义配置项，理解该配置有助于后续3个配置文件的配置说明

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| key    | string | 是   | 变量的名称                                                   |
| value  | string | 否   | 表示变量的值为字符串value，不做表达式的求值(evaluate)，与expr二者必须填一个 |
| expr   | string | 否   | 表示变量的值为expr对应表达式的求值结果，与value二者必须填一个，表达式支持的运算详见配置表达式运算支持说明 |

#### 样例

定义名为query，值为用户输入query的http_query配置项

```
http_query {
    key : "query"
    expr : "get($request, 'query')"
}
```

### backend.conf

后端服务管理引擎需要使用的配置，用于声明接入的后端的配置，包括UNIT技能后端、DMKit技能后端，session后端等，backend.conf包括两个层级的概念：

* backend：负责声明服务器的地址、协议、连接超时、重试等，代表服务提供方，比如UNIT平台；
* servcie：负责声明具体远程服务调用的**请求构造**策略和**召回结果解析**策略，一个backend可以包括多个service，比如UNIT平台上不同的技能(skill)；

`backend.conf`中可以同时定义多个不同的backend配置，用于管理多个来源的技能

一个`backend.conf`样例：

```
backend {
    name : "echo_backend"
    server : "127.0.0.1:8000"
    protocol : "http"
    connect_timeout_ms : 100
    timeout_ms : 100
    max_retry : 1

    service {
        name : "echo"
        request {
            http_method : "post"
            http_uri : "/EchoService/Echo"
            http_header {
                key : "Content-Type"
                value : "application/json"
            }
            http_body {
                key : "message"
                expr : "get($request, 'query')"
            }
        }
        response {
        	ensure : "get($response, 'errno') == 0"
            output {
                key : "result"
                expr : "get($response, 'message')"
            }
        }
    }
}
```

#### backend配置

| 配置项             | 类型   | 必须 | 说明                                                         |
| ------------------ | ------ | ---- | ------------------------------------------------------------ |
| name               | string | 是   | backend的名称                                                |
| server             | string | 是   | 服务器地址，支持brpc的server声明方式，详见[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/client.md#channel) |
| protocol           | string | 是   | 与服务器的交互协议，支持http和redis两种协议                  |
| connection_type    | string | 否   | 与服务器连接的类型，支持brpc的连接类型，详见[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/client.md#%E8%BF%9E%E6%8E%A5%E6%96%B9%E5%BC%8F) |
| load_balancer      | string | 否   | 负载均衡的算法，支持brpc的负载均衡算法，详见[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/client.md#%E8%B4%9F%E8%BD%BD%E5%9D%87%E8%A1%A1) |
| connect_timeout_ms | int32  | 否   | 连接超时，单位为ms                                           |
| timeout_ms         | int32  | 否   | 请求超时，单位为ms                                           |
| max_retry          | int32  | 否   | 当请求发生网络错误时进行的最大重试次数                       |
| service*           | object | 是   | 该backend下面的service配置，具体参数参见service配置说明<br />backend配置可以包含多个service配置 |
| request_template*  | object | 否   | 当同一个backend下的多个service共用同一个request策略时，可以在backend里定义request_template，并在service的request配置中进行引用，具体参数参见request配置说明<br />backend配置可以包含多个request_template配置 |
| response_template* | object | 否   | 当同一个backend下的多个service共用同一个response策略时，可以在backend里定义response_template，并在service的responset配置中进行引用，具体参数参见response配置说明<br />backend配置可以包含多个response_template配置 |

#### service配置

| 配置项          | 类型   | 必须 | 说明                                                         |
| --------------- | ------ | ---- | ------------------------------------------------------------ |
| name            | string | 是   | service的名称                                                |
| request_policy  | string | 否   | 表示service的请求构造策略，默认为`default`，当默认请求构造策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](docs/custom.md) |
| request         | object | 否   | 该service的请求构造配置，当request_policy为`default`时有效，具体参数参见request配置说明 |
| response_policy | string | 否   | 表示service的结果解析策略，默认为`default`，当默认结果解析策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](docs/custom.md) |
| response        | object | 否   | 该service的请求构造配置，当response_policy为`default`时有效，具体参数参见response配置说明 |

#### request配置

| 配置项       | 类型     | 必须 | 说明                                                         |
| ------------ | -------- | ---- | ------------------------------------------------------------ |
| name         | string   | 否   | request配置的名称，当作为request_template的时候需要声明，后续可以引用该名称 |
| include      | stirng   | 否   | 当引用template的时候，需要声明为对应request_template的名称   |
| def*         | KVE      | 否   | 定义request中使用的局部变量，可在request配置的其他配置项中使用，具体参数见KVE配置说明，request配置中可以包含多个def配置 |
| http_method  | string   | 否   | HTTP请求的方法，支持GET/POST，当backend的协议为http时，该字段有效 |
| http_uri     | string   | 否   | HTTP请求的URI，当backend的协议为http时，该字段有效           |
| http_header* | KVE      | 否   | HTTP请求的Header，需要满足HTTP Header的定义。具体参数见KVE配置说明，request配置中可以包含多个http_header的配置 |
| http_query*  | KVE      | 否   | HTTP请求的Query，具体参数见KVE配置说明，request配置中可以包含多个http_query的配置 |
| http_body*   | KVE      | 否   | HTTP请求的Body，当http_method为POST时生效，目前支持application/json格式的编码，具体参数见KVE配置说明，request配置中可以包含多个http_body的配置 |
| redis_cmd*   | object   | 否   | Redis命令请求配置，当backend的协议为redis时，该字段有效，具体参数参见redis_cmd配置，request配置可以包含多个redis_cmd配置 |

注：每个request配置会生成一个局部作用域

#### redis_cmd配置

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| op     | string | 是   | Redis命令的名称，比如get，set等                              |
| arg*   | string | 否   | 与Redis命令op对应的参数，每个arg是一个表达式，执行Redis命令时使用arg的表达式执行结果。redis_cmd配置中可以包含多个arg |

#### response配置

| 配置项  | 类型   | 必须 | 说明                                                         |
| ------- | ------ | ---- | ------------------------------------------------------------ |
| name    | string | 否   | response配置的名称，当作为response_template的时候需要声明，后续可以引用该名称 |
| include | string | 否   | 当引用template的时候，需要声明为对应response_template的名称  |
| def*    | KVE    | 否   | 定义response中使用的局部变量，可在response配置的其他配置项中使用，具体参数见KVE配置说明，response配置中可以包含多个def配置 |
| if*     | string | 否   | 定义一个条件分支，具体参数见response if配置的说明，一个response节点可以包含多个if条件分支。 |
| output* | object | 否   | 定义结果的字段抽取和适配策略，每个output定义一个抽取的字段的名称及其对应的取值，具体参数见KVE配置说明。response配置中可以包含多个output配置，用于多个字段的抽取。 |

注1：在response配置中，可以访问名为$response的特殊变量，用于表示该service对应的原始召回结果

注2：每个response配置会生成一个局部作用域

##### response if配置项说明

| 配置项 | 类型     | 必须 | 说明                                                         |
| ------ | -------- | ---- | ------------------------------------------------------------ |
| cond*  | string   | 是   | 定义进入分支的判断条件(比如errno为0)，每个cond要求是可以产生boolean结果的表达式。response if配置中可以包含多个cond配置，多个cond之间是and关系 |
| def*   | KVE      | 否   | 定义response if配置中使用的局部变量，可在response if配置的其他配置项中使用，具体参数见KVE配置说明，response if配置中可以包含多个def配置 |
| output | KVE      | 否   | 表示进入该分支后，定义结果的字段抽取和适配策略，每个output定义一个抽取的字段的名称及其对应的取值，具体参数见KVE配置说明。response if配置中可以包含多个output配置，用于多个字段的抽取。 |

注：每个response if配置会生成一个局部作用域

### rank.conf

RankEngine需要使用的配置文件，用于声明不同skill结果之间的排序规则。

rank.conf中可以同时定义多个rank规则配置，交由RankEngine进行管理

一个`rank.conf`样例，定义了优先级分组，3个对话技能为1组，聊天自己为1组，对于3个对话技能，使用score打分按照从大到小排序：

```
rank {
    name : "rank_rule"

    order : "unit_skill_1,unit_skill_2,unit_skill_3"
    order : "talk"

    sort_by {
        expr : "get($item, 'score')"
        desc : 0
    }
}
```

#### rank规则配置

| 配置项      | 类型   | 必须 | 说明                                                         |
| ----------- | ------ | ---- | ------------------------------------------------------------ |
| name        | string | 是   | 定义一个排序规则的名称，用于在flow.conf中进行引用            |
| rank_policy | string | 否   | 表示排序使用的策略，默认为`default`，当默认排序策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](docs/custom.md) |
| order*      | string | 否   | 技能之间的优先级顺序定义，支持定义多个order，按照定义从上到下的顺序，技能优先级逐步下降；每个order支持通过逗号(,)定义多个同等优先级的技能，该配置项在排序结果中优先级最高。该配置项当rank_policy为`default`时有效 |
| sort_by*    | object | 否   | 定义排序的规则和对应的计算方式，具体参数参见sort_by配置说明，一个rank配置中可以支持多个sort_by配置，表示按照多个规则依次进行排序。该配置项当rank_policy为`default`时有效 |

#### sort_by配置

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| expr   | string | 是   | 定义该规则的排序指标的计算表达式，表达式的计算结果要求是数值型 |
| desc   | int32  | 否   | 表示该规则是否按照排序指标的数值从大到小进行排序，默认为1，如果需要按照从小到大排序，则该项置为0 |

注：在sort_by配置中，可以访问名为$item的特殊变量，用于表示参与排序的每一个技能的结果

### flow.conf

FlowEngine需要使用的配置，用于定义整个中控US的执行流程。

一个flow.conf中可以包含多个flow配置，用于声明多个处理流程的节点，处理流程节点之间通过指向来串起整个执行流程。执行流程从配置的第一个flow配置节点开始。

一个`flow.conf`样例，声明了先获取session结果，然后并行请求3个对话技能，并且按照排序规则进行排序并生成最终结果：

```
flow {
    name : "read_session"
    recall : "session"
    next : "skill_recall_rank"
}
flow {
    name : "skill_recall_rank"
    recall : "unit_skill_1"
    recall : "unit_skill_2"
    recall : "unit_skill_3"
 
    # 排序
    rank {
        rank : "rank_rule"
        top_k : 1
    }
 
    if {
        cond : "len($rank) == 1"
        output {
            key : "result"
            expr : "get($rank, '0')"
        }
    }
}
```

#### flow配置

| 配置项      | 类型   | 必须 | 说明                                                         |
| ----------- | ------ | ---- | ------------------------------------------------------------ |
| flow_policy | string | 否   | 表示流程控制使用的策略，默认为`default`，当默认流程控制策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](docs/custom.md) |
| flow*       | string | 否   | 定义中控的流程节点和跳转关系，该配置项当flow_policy为`default`时有效，具体参数见flow节点配置 |

#### flow节点配置

| 配置项  | 类型     | 必须 | 说明                                                         |
| ------- | -------- | ---- | ------------------------------------------------------------ |
| name    | string   | 是   | 定义flow节点的名字，可以被其他flow节点的next配置项使用       |
| def*    | KVE      | 否   | 定义flow节点中使用的局部变量，可在flow节点配置的其他配置项中使用，具体参数见KVE配置说明，flow节点配置中可以包含多个def配置 |
| recall* | string   | 否   | 定义在该flow节点召回的后端远程服务，声明的值必须是在backend.conf中定义的service的名称，否则会提示找不到对应的b。一个flow节点配置中可以包含多个recall，表示并行召回这些后端远程服务 |
| rank*   | object   | 否   | 定义该flow节点使用的排序规则，具体参数见flow rank配置的说明，一个flow节点配置可以包含多个rank规则，比如先粗排后再精排 |
| if*     | object   | 否   | 定义一个条件分支，具体参数见flow if配置的说明，一个flow节点可以包含多个if条件分支，用于多种情况判断 |
| output* | KVE      | 否   | 表示默认情况下(即所有if都不满足)，定义结果的字段抽取和适配策略，每个output定义一个抽取的字段的名称及其对应的取值，具体参数见KVE配置说明。flow节点配置中可以包含多个output配置，用于多个字段的抽取。 |
| next    | string   | 否   | 表示默认情况下(即所有if都不满足)，下一个跳转的flow节点，如果没有定义，则执行流程到此结束 |

注1：在flow节点配置中，可以访问名为$recall的变量，该变量保存了该节点成功召回的技能的名称列表，可以作为排序的输入

注2：在flow节点配置中，可以访问名为$rank的变量，该变量保存了该节点对指定技能排序后的名称列表

注3：每个flow节点配置会生成一个局部作用域

#### flow rank配置

| 配置项 | 类型   | 必须 | 说明                                                    |
| ------ | ------ | ---- | ------------------------------------------------------- |
| rule   | string | 是   | 指定使用的排序规则名称，声明的值必须在rank.conf中有定义 |
| input  | string | 否   | 排序使用的技能资源列表，默认为$recall                   |
| top_k  | int32  | 否   | 只保留排序后的top k结果，没有指定时，保留全部结果       |

#### flow if配置

| 配置项 | 类型     | 必须 | 说明                                                         |
| ------ | -------- | ---- | ------------------------------------------------------------ |
| cond*  | string   | 是   | 定义进入分支的判断条件，每个cond要求是可以产生boolean结果的表达式。flow if配置中可以包含多个cond配置，多个cond之间是and关系 |
| def*   | KVE      | 否   | 定义flow if配置中使用的局部变量，可在flow if配置的其他配置项中使用，具体参数见KVE配置说明，flow if配置中可以包含多个def配置 |
| output | KVE      | 否   | 表示进入该分支后，定义结果的字段抽取和适配策略，每个output定义一个抽取的字段的名称及其对应的取值，具体参数见KVE配置说明。flow if配置中可以包含多个output配置，用于多个字段的抽取。 |
| next   | string   | 否   | 表示进入该分支后，下一个跳转的flow节点                       |

注：每个flow if配置会生成一个局部作用域
