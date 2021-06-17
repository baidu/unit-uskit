## 详细配置说明

USKit 的配置采用 protobuf 的 text format 格式。

> 注：下面配置项说明中，名字带有 `*` 的表示可以同时定义多个

### us.conf

USKit服务的主配置，位于 `conf/` 目录下，主要用于指定对话中控根目录以及需要加载的对话机器人中控。

一个 `us.conf` 样例：

```
root_dir : "./conf/us/"

load : "demo"
load : "test"

equired_params {
    param_name : "user_id"
    default_value : "USER_1"
}
equired_params {
    param_name : "query"
    param_expr : "get($request, 'request/query', '')"
}

input_config_path : "config_json"
```

| 配置项       | 类型   | 必须 | 说明                                                         |
| ------------ | ------ | ---- | :----------------------------------------------------------- |
| root_dir     | string | 否   | 机器人中控配置的根目录路径，默认为 `"./conf/us/"`，路径相对于 `_build` 目录 |
| load*        | string | 否   | 声明需要加载的对话中控id，如果 `root_dir` 下没有该 id 对应目录，则 USKit 服务会启动失败。 |
| required_params* | object | 否 | 用户请求必传参数的配置，默认为 `logid`, `uuid`, `usid`, `query`。具体参数参见 required_params 配置说明<br />`us.conf` 可以包含多个 required_params 配置 |
| editable_response | bool | 否 | 默认为 false。表示是否直接输出 flow 的 output 结果，不添加 `error_code` 与 `error_msg` |
| input_config_path | string | 否 | 无状态请求的 json 配置路径。未设置时表示不启用无状态请求 |

#### required_params 配置
| 配置项       | 类型   | 必须 | 说明                                                         |
| ------------ | ------ | ---- | ------------------------------------------------------------ |
| param_name | string | 是   | 参数名称 |
| default_value | string | 否 | 默认参数值 |
| param_path | string | 否 | 参数路径 |
| param_expr | string | 否 | 参数表达式 |

> 注：`default_value`, `param_path`, `param_expr` 中只有一个会生效，优先级从高到低。

USKit 配置的灵活性在于配置项可以支持表达式运算，提供了一套配置层面的 DSL (领域特定语言)，可以根据不同的用户请求、后端远程调用结果、技能排序结果来动态生成相应的配置，并根据生成的配置执行相应的处理得到最终结果。具体的表达式语法可以参见[表达式运算支持](expression.md)

下面依次对 `backend.conf`，`rank.conf` 和 `flow.conf` 三个配置文件进行详细说明。

### 全局变量

在 `backend.conf`，`rank.conf`，`flow.conf` 中，可以访问到以下全局变量：

* `$request`：用于表示用户的请求参数，比如可以通过 `get($request, 'query')` 获取用户的 query
* `$result`：输出结果，对应于最终 JSON 结果中的 result 字段
* `$backend`：保存了所有成功处理的后端远程调用的结果，比如可以通过 `get($backend, 'get_session')` 来获取名为 get_session 的远程服务调用结果

### KVE 配置

KVE 用于定义变量名及其对应的取值，取值可以是直接字符串，也可以是表达式(取值为表达式的求值结果)。在 `backend.conf`，`rank.conf`，`flow.conf` 中均大量使用 KVE 配置定义配置项，理解该配置有助于后续 3 个配置文件的配置说明。

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| key    | string | 是   | 变量的名称                                                   |
| value  | string | 否   | 表示变量的值为字符串 value，不做表达式的求值 (evaluate)，与 expr 二者必须填一个 |
| expr   | string | 否   | 表示变量的值为 expr 对应表达式的求值结果，与 value 二者必须填一个，表达式支持的运算详见配置表达式运算支持说明 |

#### 样例

定义名为 query，值为用户输入 query 的 http_query 配置项

```
http_query {
    key : "query"
    expr : "get($request, 'query')"
}
```

### backend.conf

Backend Engine需要使用的配置文件，，用于声明接入的后端的配置，包括 UNIT 技能后端、DMKit 技能后端，session 后端等，`backend.conf` 包括两个层级的概念：

* backend：负责声明服务器的地址、协议、连接超时、重试等，代表服务提供方，比如 UNIT 平台；
* servcie：负责声明具体远程服务调用的**请求构造**策略和**召回结果解析**策略，一个 backend 可以包括多个 service，比如 UNIT 平台上不同的技能 (skill)；

`backend.conf` 中可以同时定义多个不同的 backend 配置，用于管理多个来源的技能

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
            if {
                cond : "get($response, 'errno') == 0"
                output {
                    key : "result"
                    expr : "get($response, 'message')"
                }
            }
        }
    }
}
```

#### backend 配置

| 配置项             | 类型   | 必须 | 说明                                                         |
| ------------------ | ------ | ---- | ------------------------------------------------------------ |
| name               | string | 是   | backend 的名称                                               |
| server             | string | 是   | 服务器地址，支持 brpc 的 server 声明方式，详见[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/client.md#channel) |
| protocol           | string | 是   | 与服务器的交互协议，支持 http 和 redis 两种协议              |
| connection_type    | string | 否   | 与服务器连接的类型，支持 brpc 的连接类型，详见[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/client.md#%E8%BF%9E%E6%8E%A5%E6%96%B9%E5%BC%8F) |
| load_balancer      | string | 否   | 负载均衡的算法，支持 brpc 的负载均衡算法，详见[brpc文档](https://github.com/brpc/brpc/blob/master/docs/cn/client.md#%E8%B4%9F%E8%BD%BD%E5%9D%87%E8%A1%A1) |
| connect_timeout_ms | int32  | 否   | 连接超时，单位为 ms                                          |
| timeout_ms         | int32  | 否   | 请求超时，单位为 ms                                          |
| max_retry          | int32  | 否   | 当请求发生网络错误时进行的最大重试次数                       |
| service*           | object | 是   | 该 backend 下面的 service 配置，具体参数参见 service 配置说明<br />backend 配置可以包含多个 service 配置 |
| request_template*  | object | 否   | 当同一个 backend 下的多个 service 共用同一个 request 策略时，可以在 backend 里定义request_template，并在 service 的 reques t配置中进行引用，具体参数参见 request 配置说明<br />backend 配置可以包含多个 request_template 配置 |
| response_template* | object | 否   | 当同一个 backend 下的多个 service 共用同一个 response 策略时，可以在 backend 里定义 response_template，并在 service 的 responset 配置中进行引用，具体参数参见 response 配置说明<br />backend 配置可以包含多个 response_template 配置 |
| is_dynamic | bool	| 否 | 默认为 false。设置为 true 时 service 中配置的 dynamic 相关参数生效 |

#### service 配置

| 配置项          | 类型   | 必须 | 说明                                                         |
| --------------- | ------ | ---- | ------------------------------------------------------------ |
| name            | string | 是   | service 的名称                                               |
| request_policy  | string | 否   | 表示 service 的请求构造策略，默认为 `default`，当默认请求构造策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](custom.md) |
| request         | object | 否   | 该 service 的请求构造配置，当 request_policy 为 `default` 时有效，具体参数参见 request 配置说明 |
| response_policy | string | 否   | 表示 service 的结果解析策略，默认为 `default`，当默认结果解析策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](custom.md) |
| response        | object | 否   | 该 service 的请求构造配置，当 response_policy 为 `default` 时有效，具体参数参见 response 配置说明 |
| success_flag    | string | 否   | 用于检查当前 service 是否被成功调用                          |

#### request 配置

| 配置项       | 类型     | 必须 | 说明                                                         |
| ------------ | -------- | ---- | ------------------------------------------------------------ |
| name         | string   | 否   | request 配置的名称，当作为 request_template 的时候需要声明，后续可以引用该名称 |
| include      | stirng   | 否   | 当引用 template 的时候，需要声明为对应 request_template 的名称 |
| def*         | KVE      | 否   | 定义 request 中使用的局部变量，可在 request 配置的其他配置项中使用，具体参数见 KVE 配置说明<br />request 配置中可以包含多个 def 配置 |
| http_method  | string   | 否   | HTTP 请求的方法，支持 GET/POST，当 backend 的协议为 http 时，该字段有效 |
| http_uri     | string   | 否   | HTTP 请求的 URI，当 backend 的协议为 http 时，该字段有效     |
| http_header* | KVE      | 否   | HTTP 请求的 Header，需要满足 HTTP Header 的定义。具体参数见 KVE 配置说明，request 配置中可以包含多个 http_header 的配置 |
| http_query*  | KVE      | 否   | HTTP 请求的 Query，具体参数见 KVE 配置说明，request 配置中可以包含多个 http_query 的配置 |
| http_body*   | KVE      | 否   | HTTP 请求的 Body，当 http_method 为 POST 时生效，目前支持 application/json 格式的编码，具体参数见 KVE 配置说明，request 配置中可以包含多个 http_body 的配置 |
| redis_cmd*   | object   | 否   | Redis 命令请求配置，当 backend 的协议为 redis 时，该字段有效，具体参数参见 redis_cmd 配置，request 配置可以包含多个 redis_cmd 配置 |
|dynamic_args_node |string | 否 |动态参数对 request 进行修改的参数类型，现在支持："uri", "query", "body", 分别对应修改 “http_uri", "http_query", "http_body"|
|dynamic_args*|KVE | 否 | key 为动态输入修改的参数类型中的 path（仅在 dynamic_args_node 为 query 和 uri 时生效，body 时忽略），expr 解析后应得到一个数组 |
|dynamic_args_path|string | 否 |修改 http_body 中的参数时，需要使用 dynamic_args_path 指定修改节点路径（支持UNIX-Like path）|
|host_ip_port|string | 否 |使用局部的 IP 和 port 而非 backend 层的 server|

> 注：
>
> - 每个 request 配置会生成一个局部作用域
> - 动态模式下（backend 配置 is_dynamic 为 true），dynamic_args_node 和 dynamic_args 为必须参数

#### redis_cmd 配置

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| op     | string | 是   | Redis 命令的名称，比如 get，set 等                           |
| arg*   | string | 否   | 与 Redis 命令 op 对应的参数，每个 arg 是一个表达式，执行Redis命令时使用arg的表达式执行结果<br/>redis_cmd 配置中可以包含多个 arg |

#### response 配置

| 配置项  | 类型   | 必须 | 说明                                                         |
| ------- | ------ | ---- | ------------------------------------------------------------ |
| name    | string | 否   | response 配置的名称，当作为 response_template 的时候需要声明，后续可以引用该名称 |
| include | string | 否   | 当引用 template 的时候，需要声明为对应 response_template 的名称 |
| def*    | KVE    | 否   | 定义 response 中使用的局部变量，可在 response 配置的其他配置项中使用，具体参数见 KVE 配置说明，response 配置中可以包含多个 def 配置 |
| if*     | string | 否   | 定义一个条件分支，具体参数见 response if 配置的说明，一个 response 节点可以包含多个 if 条件分支。 |
| output* | object | 否   | 定义结果的字段抽取和适配策略，每个 output 定义一个抽取的字段的名称及其对应的取值，具体参数见 KVE 配置说明。response 配置中可以包含多个 output 配置，用于多个字段的抽取。 |

> 注：
>
> - 在 response 配置中，可以访问名为 `$response` 的特殊变量，用于表示该 service 对应的原始召回结果
>
> - 每个 response 配置会生成一个局部作用域

##### response if 配置项说明

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| cond*  | string | 是   | 定义进入分支的判断条件 (比如 errno 为 0)，每个 cond 要求是可以产生 boolean 结果的表达式。response if 配置中可以包含多个 cond 配置，多个 cond 之间是 and 关系 |
| def*   | KVE    | 否   | 定义 response if 配置中使用的局部变量，可在 response if 配置的其他配置项中使用，具体参数见 KVE 配置说明，response if 配置中可以包含多个 def 配置 |
| output | KVE    | 否   | 表示进入该分支后，定义结果的字段抽取和适配策略，每个 output 定义一个抽取的字段的名称及其对应的取值，具体参数见 KVE 配置说明。response if 配置中可以包含多个 output 配置，用于多个字段的抽取。 |

> 注：每个 response if 配置会生成一个局部作用域

### rank.conf

Rank Engine 需要使用的配置文件，用于声明不同 skill 结果之间的排序规则。

`rank.conf` 中可以同时定义多个rank规则配置，交由 Rank Engine 进行管理

一个 `rank.conf` 样例，定义了优先级分组，3个对话技能为1组，聊天自己为1组，对于3个对话技能，使用 score 打分按照从大到小排序：

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

#### rank 规则配置

| 配置项      | 类型   | 必须 | 说明                                                         |
| ----------- | ------ | ---- | ------------------------------------------------------------ |
| name        | string | 是   | 定义一个排序规则的名称，用于在 `flow.conf` 中进行引用        |
| rank_policy | string | 否   | 表示排序使用的策略，默认为`default`，当默认排序策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](custom.md) |
| order*      | string | 否   | 技能之间的优先级顺序定义，支持定义多个 order，按照定义从上到下的顺序，技能优先级逐步下降；每个 order 支持通过逗号(,)定义多个同等优先级的技能，该配置项在排序结果中优先级最高。该配置项当 rank_policy 为`default`时有效 |
| sort_by*    | object | 否   | 定义排序的规则和对应的计算方式，具体参数参见 sort_by 配置说明，一个 rank 配置中可以支持多个 sort_by 配置，表示按照多个规则依次进行排序。该配置项当 rank_policy 为`default`时有效 |

#### sort_by 配置

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| expr   | string | 是   | 定义该规则的排序指标的计算表达式，表达式的计算结果要求是数值型 |
| desc   | int32  | 否   | 表示该规则是否按照排序指标的数值从大到小进行排序，默认为1，如果需要按照从小到大排序，则该项置为0 |

> 注：在 sort_by 配置中，可以访问名为 `$item` 的特殊变量，用于表示参与排序的每一个技能的结果

### flow.conf

Flow Engine需要使用的配置，用于定义整个中控US的执行流程。

一个 `flow.conf` 中可以包含多个 flow 配置，用于声明多个处理流程的节点，处理流程节点之间通过指向来串起整个执行流程。执行流程从配置的第一个 flow 配置节点开始。

一个 `flow.conf` 样例，声明了先获取 session 结果，然后并行请求3个对话技能，并且按照排序规则进行排序并生成最终结果：

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

#### flow 配置

| 配置项      | 类型   | 必须 | 说明                                                         |
| ----------- | ------ | ---- | ------------------------------------------------------------ |
| flow_policy | string | 否   | 表示流程控制使用的策略，现有支持策略有 `default`、`recurrent`、`globalcancel` 和 `leveldeliver`，当默认流程控制策略没法满足使用方需求时，可以进行策略自定义，详见[自定义函数和策略](custom.md) |
| flow*       | string | 否   | 定义中控的流程节点和跳转关系，该配置项当 flow_policy 为`default`、`recurrent`、`global_cancel` 和 `leveldeliver`时有效，具体参数见 flow 节点配置 |

> 注：
>
> - `default` 为默认策略，按用户在 flow 节点中指定的 `next` 按顺序执行 flow 节点。
>
> - `recurrent` 实现了基本的服务跳转和异步并发配置。USKit 接收到用户请求后，向指定后端发送请求。
>   当制定请求返回结果并经过条件判断符合时，将返回结果的一部分作为请求参数发送新的后端请求（两次请求的后端不需要一致）。所有结果返回（或取消）后，进行排序，返回结果。
>
> - `globalcancel` 在异步并发的基础上，支持在任意请求结果返回后检查全局数据判断条件，满足条件则向客户端返回结果。
> - `leveldeliver` 在全局异步并发的基础上，支持分发能力。分发服务返回结果满足配置条件后执行目标服务。

#### flow 节点配置

| 配置项  | 类型     | 必须 | 说明                                                         |
| ------- | -------- | ---- | ------------------------------------------------------------ |
| name    | string   | 是   | 定义 flow 节点的名字，可以被其他 flow 节点的 next 配置项使用 |
| def*    | KVE      | 否   | 定义 flow 节点中使用的局部变量，可在 flow 节点配置的其他配置项中使用，具体参数见 KVE 配置说明，flow 节点配置中可以包含多个 def 配置 |
| recall* | string   | 否   | 定义在该 flow 节点召回的后端远程服务，声明的值必须是在 `backend.conf` 中定义的 service 的名称，否则会提示找不到对应的 backend。一个 flow 节点配置中可以包含多个 recall，表示并行召回这些后端远程服务 |
| rank*   | object   | 否   | 定义该 flow 节点使用的排序规则，具体参数见 flow rank 配置的说明，一个 flow 节点配置可以包含多个 rank 规则，比如先粗排后再精排 |
| if*     | object   | 否   | 定义一个条件分支，具体参数见 flow if 配置的说明，一个 flow 节点可以包含多个 if 条件分支，用于多种情况判断 |
| output* | KVE      | 否   | 表示默认情况下(即所有 if 都不满足)，定义结果的字段抽取和适配策略，每个 output 定义一个抽取的字段的名称及其对应的取值，具体参数见 KVE 配置说明。flow 节点配置中可以包含多个 output 配置，用于多个字段的抽取。 |
| next    | string   | 否   | 表示默认情况下(即所有 if 都不满足)，下一个跳转的 flow 节点，如果没有定义，则执行流程到此结束 |
| recall_config | object | 否 | recall 的升级配置，recall 仍然可用。使用异步并发模式时在 `flow.conf` 中后端请求的配置项，作用等同于同步（默认）模式下的 recall 配置项；在异步模式中，不可以配置 recall 项，否则会覆盖 recall_config，使用同步模式一个 flow 节点只需要一个 recall_config，recall_config 中包含多个 recall_service 和一个 cancel_order |
| global_cancel_config | object	| 否 | 定义一个全局退出的条件，具体参数见 response if 配置的说明（仅 cond 参数生效），在全局异步模式下为必须参数 |
| deliver_config* | object	| 否 | 分发配置，具体参数见 deliver_config 配置说明。一个 flow 节点配置可以包含多个 deliver_config 配置 |
| intervene_config | object	| 否 | 干预配置，具体参数见 intervene_config 配置说明 |

> 注：
>
> - 在 flow 节点配置中，可以访问名为 `$recall` 的变量，该变量保存了该节点成功召回的技能的名称列表，可以作为排序的输入
>
> - 在 flow 节点配置中，可以访问名为 `$rank` 的变量，该变量保存了该节点对指定技能排序后的名称列表
>
> - 每个 flow 节点配置会生成一个局部作用域

#### flow rank 配置

| 配置项 | 类型   | 必须 | 说明                                                        |
| ------ | ------ | ---- | ----------------------------------------------------------- |
| rule   | string | 是   | 指定使用的排序规则名称，声明的值必须在 `rank.conf` 中有定义 |
| input  | string | 否   | 排序使用的技能资源列表，默认为 `$recall`                    |
| top_k  | int32  | 否   | 只保留排序后的 top k 结果，没有指定时，保留全部结果         |

#### flow if 配置

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| cond*  | string | 是   | 定义进入分支的判断条件，每个 cond 要求是可以产生 boolean 结果的表达式。flow if 配置中可以包含多个 cond 配置，多个 cond 之间是 and 关系 |
| def*   | KVE    | 否   | 定义 flow if 配置中使用的局部变量，可在 flow if 配置的其他配置项中使用，具体参数见 KVE 配置说明，flow if 配置中可以包含多个 def 配置 |
| output | KVE    | 否   | 表示进入该分支后，定义结果的字段抽取和适配策略，每个 output 定义一个抽取的字段的名称及其对应的取值，具体参数见 KVE 配置说明。flow if 配置中可以包含多个 output 配置，用于多个字段的抽取。 |
| next   | string | 否   | 表示进入该分支后，下一个跳转的 flow 节点                     |

> 注：每个 flow if 配置会生成一个局部作用域

#### flow recall_config 配置

| 配置项 | 类型     | 必须 | 说明                                                         |
| ------ | -------- | ---- | ------------------------------------------------------------ |
| cancel_order | string | 异步模式下必须 | 定义异步并发请求取消的方式，支持：ALL, PRIORITY, HIERACHY, NONE (大小写敏感) |
| recall_service* | object | 异步模式下必须 | 定义在该 flow 节点召回的后端远程服务 |

#### flow recall_service 配置
| 配置项 | 类型     | 必须 | 说明                                                         |
| ------ | -------- | ---- | ------------------------------------------------------------ |
| name | string | 是 | 定义在该 flow 节点召回的后端远程服务，声明的值必须是在 `backend.conf` 中定义的 service 的名称，否则会提示找不到对应的 backend |
| priority | int | 是 | 定义 recall_service 的优先级，具体使用见注解 |

> 注：
>
>  * ALL: 该 recall_service 请求返回结果后，取消所有其他请求
>  * PRIORITY: 该 recall_service 请求返回结果后，取消所有 priority 参数**不大于**其 priority 的请求
>  * HIERACHY: 该 recall_service 请求返回结果后，取消所有与其 priority 参数**相同**的请求
>  * NONE: 该 recall_service 请求返回结果后，不取消任何请求
>  * GLOBAL_CANCEL：仅支持 flow policy 设置为 globalcancel 的 flow 配置；当**任意**请求返回结果后，检查全局数据是否符合配置中指定的条件，满足条件则退出 recall 阶段进入 rank 和 output 流程。
>
> 以上配置中的 “请求返回结果” 判定条件为：后端请求返回状态码为 200（OK），
> 简单异步模式配置中暂时不支持对返回结果解析后进行条件判断并取消请求

#### flow deliver_config 配置

| 配置项           | 类型   | 必须 | 说明                                                         |
| ---------------- | ------ | ---- | ------------------------------------------------------------ |
| class_num        | int32  | 是   | 分类服务的类别数目                                           |
| clasify_service  | string | 是   | 分类服务，对应 `backend.conf` 中的一个 service               |
| deliver_service* | object | 是   | 分发目标服务配置。具体参数见 deliver_service 配置说明。一个 deliver_config 支持配置多个 deliver_service |

##### deliver_service 配置

| 配置项       | 类型   | 必须 | 说明                                               |
| ------------ | ------ | ---- | -------------------------------------------------- |
| service_name | string | 是   | 分发服务名称，对应 `backend.conf` 中的一个 service |
| threshold    | string | 是   | 分类服务阈值                                       |

#### flow intervene_config 配置

| 配置项         | 类型   | 必须 | 说明                                                         |
| -------------- | ------ | ---- | ------------------------------------------------------------ |
| source         | string | 是   | 需要用来干预的对象，一般是一个表达式，例如最常用的 query 干预可写为 "get($request, 'query', '')" |
| intervene_file | string | 否   | 干预模板文件路径，文件中包含了所有的干预规则。具体参数见 intervene_file 文件配置。 |

##### intervene_file 文件配置

| 配置项 | 类型   | 必须 | 说明                                                         |
| ------ | ------ | ---- | ------------------------------------------------------------ |
| rule*  | object | 否   | 干预规则配置，包含了正则表达式、干预目标和干预设置。具体参数见下方。一个 intervene_file 可以包含多个 rule 配置。 |

每个 rule 包含以下信息：

| 配置项            | 类型   | 必须 | 说明                                                         |
| ----------------- | ------ | ---- | ------------------------------------------------------------ |
| pattern           | string | 是   | 正则表达式                                                   |
| target            | string | 是   | 干预目标，可以为 backend 某个 service，或 flow 中某个 flow 节点 |
| entire_match      | bool   | 否   | 是否完整匹配，对应正则表达式的 match 和 search，默认为 true 表示 match |
| service_intervene | bool   | 否   | 是否为 service 干预，默认为 true                             |
