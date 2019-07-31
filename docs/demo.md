# US Kit 使用示例 (demo)

## 差旅场景

该示例演示如何使用US Kit实现一个差旅场景下的对话机器人，需要同时支持天气查询、机票预定、酒店预定和闲聊共4个技能的满足。

在进行对话中控的搭建之前，首先我们需要准备好以下4个技能：

### 添加UNIT预置技能
其中，天气、机票和闲聊均为UNIT平台的预置技能，可以在平台上直接进行添加。

1. 进入[百度理解与交互平台(UNIT)>我的技能](http://ai.baidu.com/unit/v2#/sceneliblist)
2. 点击获取技能，依次获取天气、机票和闲聊3个技能，获取成功后可以在我的技能中查看每个技能的技能ID，如下图红框圈出：
![UNIT网站查看技能id](imgs/skill_id.png)

### 添加自定义技能
酒店预定技能可以通过UNIT平台进行自定义技能配置，具体的配置方式参考DM Kit[示例文档](https://github.com/baidu/unit-dmkit/blob/master/docs/demo_skills.md)

### 使用US Kit搭建对话机器人

在我们开始之前，梳理一下机器人中控需要完成的工作和策略：

1. 接入天气、机票、酒店和闲聊4个技能，完成与[UNIT 2.0协议](https://ai.baidu.com/docs#/UNIT-v2-dialogue-API/top)的对接(backend.conf)
2. 技能的并行召回和解析抽取，根据远程服务通信是否成功，错误码是否为0判断是否成功召回技能(backend.conf)
3. 多技能结果的排序，在这个教程中，我们按照`酒店>机票>天气>聊天`的优先级对技能结果进行排序，这是比较简单的排序策略，如需更加复杂的排序策略可以参考[详细配置文档](config.md)(rank.conf)
4. 管理多轮上下文session，记录用户多轮交互结果，对于涉及多轮对话的技能来说这很重要，本教程使用Redis作为session的存储介质，对于每个用户记录对应的session，并将session下发给上一轮被选为最终结果的技能(backend.conf，flow.conf)
5. Access Token的管理，由于访问UNIT平台需要根据access token，并且该token一般是一个月的有效期，因此需要在内存里cache该token，并设置有效期，本教程使用Redis的SETEX命令存储access token并设置超时，并在过期后自动刷新token(backend.conf，flow.conf)

整体的流程图如下：
![demo机器人流程图](imgs/demo_flow.png)

按照上面的梳理，我们通过编写`backend.conf`，`rank.conf`和`flow.conf`3个配置，即可实现一个多技能满足的差旅机器人:

#### backend.conf
以添加UNIT天气技能为例：

策略：定义请求UNIT平台服务的地址、通信协议、超时等选项，以及UNIT协议请求模板和结果解析模板策略，完成后引用模板进行天气技能的定义(只需额外定义技能对应的id)

```
backend {
    name : "unit_backend"
    # 将server修改为你的服务器地址
    server : "https://aip.baidubce.com"
    protocol : "http"
    connect_timeout_ms : 500
    timeout_ms : 3000
    max_retry : 1

    request_template {
        name : "unit_request_template"

        http_method : "post"
        http_uri : "/rpc/2.0/unit/bot/chat"

        http_header {
            key : "Content-Type"
            value : "application/json"
        }

        ...

        http_body {
            key : "bot_id"
            expr : "$bot_id"
        }
    }

    response_template {
        name : "unit_response_template"
        
        if {
            cond : "get($response, 'error_code') == 0"
            cond : "get($response, 'result/response/action_list/0/type') == 'event'"

            ...

            output {
                key : "session/bot_session"
                expr : "get($response, 'result/bot_session', '')"
            }

            output {
                key : "session/bot_id"
                expr : "$bot_id"
            }
        }
    }

    service {
        name : "weather_11111"
        request {
            def {
                key : "bot_id"
                value : "11111"
            }
            include : "unit_request_template"
        }

        response {
            def {
                key : "bot_id"
                value : "11111"
            }
            include : "unit_response_template"
        }
    }
}
```

#### rank.conf
按照上文提到的排序规则进行定义：

策略：酒店>机票>天气>聊天>聊天

```
rank {
    name : "skill_rank"
    order: "hotel_44444"
    order: "flight_22222"
    order: "weather_11111"
    order: "talk_33333"
}
```

#### flow.conf
按照流程图里依次定义流程节点，这里以召回对话技能节点为例：

策略：并发召回上文提及的4个技能，并按照`rank.conf`中定义的排序规则进行排序，选择top1技能作为结果输出，如果4个技能均未召回，那么调整到兜底回复节点

```
flow {
    name : "recall_skill"
    recall: "hotel_44444"
    recall: "flight_22222"
    recall: "weather_11111"
    recall: "talk_33333"

    rank {
        rule : "skill_rank"
        top_k : 1
    }

    if {
        cond : "len($rank) == 1"
        output {
            key : "skill"
            expr : "get($rank, '0')"
        }
        output {
            key : "value"
            expr : "get($backend, get($rank, '0') + '/data')"
        }
        next : "set_session"
    }

    next : "default"
}
```

为了方便开发者快速体验，本教程还提供了配置生成脚本`conf_generator.py`(位于_build/conf/us/demo目录下)，开发者只需修改同目录下的options.py，将对应的服务器地址、技能id、API Key和Secret Key等进行替换：

```
# -*- coding: utf-8 -*-

options = {
    # server address
    # 服务器地址
    'redis_server' : '127.0.0.1:6379',
    'dmkit_server' : '127.0.0.1:8010',
    'unit_server' : 'https://aip.baidubce.com',

    # skill ids
    # 填写每个技能对应的技能id
    'unit_skill_id' : {
        'weather' : '11111',
        'flight' : '22222',
        'talk' : '33333'
    },

    'dmkit_skill_id' : {
        'hotel' : '44444'
    },


    # rank order
    # 按照数组从左到右的优先级进行排序
    'rank' : ['hotel', 'flight', 'weather', 'talk'],

    # access token
    # 查看百度AI开放平台文档了解如何获取API Key和Secret Key: http://ai.baidu.com/docs#/Begin/top
    'api_key' : 'your_api_key',
    'secret_key' : 'your_secret_key'
}
```

然后运行`python conf_generator.py`即可生成US Kit所需的`backend.conf`，`rank.conf`和`flow.conf`3个配置文件：

```
cd _build/conf/us/demo
# edit opitons.py...
python conf_generator.py
cd ../../../
./uskit
```

另外，由于我们使用了Redis作为存储介质，因此还需要在你的机器上安装Redis并启动服务，可以通过搜索引擎查找你的系统相对应的安装方法，这里不再赘述。安装并启动后，可以通过以下方式验证Redis服务是否正常工作：

```
> redis-cli ping
PONG
```

如果返回PONG说明正常。

发起请求：

```
curl -H "Content-type: application/json" -d '{"usid":"demo","logid":"123456","query":"我想订酒店","uuid":"123"}' http://127.0.0.1:8888/us
```

一切正常的话，你应该可以看到以下结果：

```
{"error_code":0,"error_msg":"OK","result":{"value":"请问您要预订哪一天的酒店？","skill":"hotel_44444"}}
```

到此为此，我们就实现了一个差旅场景下的对话机器人，更深入学习US Kit的配置和其他用法可以参考[详细配置说明](config.md)和[配置表达式运算](expression.md)

## 异步并发模式场景
现在我们思考这样的一种场景：用户通过语音访问服务时环境嘈杂导致输入 query 缺少字词，从而召回率低于预期，我们可以使用 query 访问泛化/改写服务。如果使用同步并发的模式，第一次同时请求泛化服务和差旅场景的4个技能，待所有请求全部返回后第二次使用泛化结果再一次请求4个技能。总体上看如果相对易技能的请求时间，泛化服务的耗时短的多的话，那么使用异步模式，在泛化服务返回结果后马上再一次请求4个技能，可以将整体的服务耗时缩减到原来的50%。

泛化服务只是一个比较常见的例子，当不同服务与服务之间的时延差别明显时，异步并发模式都可能是更好的一种尝试。

异步模式与同步模式在配置上主要有以下区别：
### backend.conf
在本例中会发送两次天气技能请求，一次是使用用户请求的参数，一次是使用泛化结果作为请求参数。

每一个结果返回后需要再次发送的其他后端请求，都需要在 backend 节点下配置一个新的service，同一个 backend 节点下的不同 service 可以共用 backend的 name, server, protocol 等配置，也可以共享同样的 template 以减少配置量。

不同service的配置在整体上是基本相同的，在本例中，weather_request_query 和 weather_rewrite_query 两个 service 的区别仅在于 request 中的 query 变量，其他都相同，所以使用 request_template。

weather_request_query 使用用户请求的query作为参数，与基本配置一致，不需要修改。

weather_rewrite_query 使用 semantic_rewrite 的返回结果作为参数，需要修改request中的 query 变量。 通过从$backend变量中取得 semantic_rewrite 的返回结果定义 query。

```
backend {
    name : "unit_backend"
    ...
    #backend 的配置共用
    ...

    # 使用用户输入query请求天气服务
    service {
        name : "weather_request_query"
        request {
            def {
                key : "query"
                expr : "get($request, 'query')"
            }
            include : "unit_request_template"
        }

        response {
            ...
        }
    }

    service {
        name : "weather_rewrite_query"
        request {
            def {
                key : "query"
                expr : "get($backend, 'semantic_rewrite/data/result')"
                #以上语句表达，在所有后端服务返回结果中，取 semantic_rewrite 这个服务的output中的 data/result 作为 http_query中key：“query"所对应的value
            }
            include : "unit_request_template"
        }
    }
}
```

### flow.conf
使用异步模式时的 flow 配置与同步模式有较大的区别：

* `flow_policy`需要明确指定为`recurrent` 或 `global_cancel`（global cancel policy 会在后文中详细介绍）
* 请求服务使用`recall_config`对象定义
* flow 节点的 rank 配置需要明确配置 `$input` 变量为所有需要排序的服务名（包括可能不在该节点使用的服务）
* 通过 `recall_config.recall_service.next_flow` 指定的flow节点中的 rank 和 output 定义无效。
```
flow_policy : "recurrent"
flow {
    name : "recall_skill"
    recall_config {
        cancel_order : "NONE"
        recall_service {
            recall_name : "flight_22222"
        }
        recall_service {
            recall_name : "semantic_rewrite"
            next_flow : "rewrite_next_flow"
        }
        recall_service {
            recall_name : "weather_request_query"
        }
    }

    #定义rank的输入数组, weather_rewrite_query 服务并不在此节点请求，而是在 rewrite_next_flow 中请求
    def {
        key : "input_array"
        expr : "['weather_request_query', 'flight_22222', 'weather_rewrite_query']"
    }

    rank {
        rule : "skill_rank"
        input : "$input_array"
        top_k : 1
    }

    if {
        ...
    }

    next : "default"
}

flow {
    name : "rewrite_next_flow"
    recall_config {
        recall_service : "weather_rewrite_query"
    }
    # 由 next_flow 指定的flow node中的rank和（if）output 不生效
}
```

### rank.conf
rank 配置基本上没有区别，仅需注意 order 中要填写所有参与排序的后端服务名。

## 全局异步取消模式
在普通的异步并发模式中，所有服务都会发出请求，等待结果返回，只是发送的顺序以及是否同步上的区别。那么如果有下面这样一种使用场景：仍然是需要`酒店、天气、机票和聊天`服务，但是由于某种原因导致除了`天气`服务外其他技能耗时相对更久，而当`天气`结果满足某种条件时，差旅需求取消，导致`酒店和机票`技能的结果不再重要，而原本的并发配置导致USKit需要等待所有结果之后才能返回用户，耗费了大量不必要的时间，因此我们需要可以**全局取消请求**的异步模式。

全局异步取消模式的使用场景：

* 多个服务之间延时的差异性很大
* 一定条件下仅需要部分请求结果即可以满足返回请求的内容

### backend.conf
backend 配置要求与普通异步并发模式一致

### rank.conf
rank 配置要求与普通异步并发模式一致

### flow.conf
全局异步取消模式的 flow 配置大体上基于普通异步并发模式的配置方式。再次基础上，有以下区别：

* flow_policy 只能使用 globalcancel
* recall_config 的 cancel_order 指定为 GLOBAL_CANCEL时，每一个recall_service结果返回后都会判断配置中的**退出**条件，符合条件则取消所有还没有返回结果的请求，并进入 flow 节点的 rank 和（if) output block
* 需要在整个`flow.conf`文件中的第一个flow节点定义 `global_cancel_config`， 使用方式见 flow if 的配置方法

```
flow_policy : "globalcancel" #全局取消异步并发模式 flow pollicy
flow {
    name : "cancel_flow_test"
    recall_config {
        cancel_order : "GLOBAL_CANCEL" #只有 globalcancel 可以使用这个cancel order
        #cancel_order : "NONE"
        #cancel_order : "ALL"
        recall_service {
            recall_name : "flight_22222"
        }
        recall_service {
            recall_name : "semantic_rewrite"
            next_flow : "rewrite_next_flow"
        }
        recall_service {
            recall_name : "weather_request_query"
        }
    }
 
    global_cancel_config { #flow conf内的所有flow会copy第一个flow node的global_cancel_config, cond用于判断是否结束所有请求并返回结果
        cond : "(get($backend, 'weather_request_query') != null) && (get($backend, 'weather_request_query/weather_text') == 'storm')"
        # 得到天气返回结果并且发现是暴风雨则取消其他请求并返回结果
    }
 
    def {
        key : "input_array"
        expr : "['weather_request_query', 'flight_22222', 'weather_rewrite_query']"
    }
 
    rank {
        rule : "robokit_rank"
        top_k : 10
        input : "$input_array"
    }
 
    if {
        cond : "len($rank) >= 1"
        def {
            key : "best_result"
            expr : "get($rank, '0')"
        }
        output {
            key : "result"
            expr : "$backend"
        }
    }
}
 
flow {
    name : "rewrite_next_flow"
    recall_config {
        cancel_order : "GLOBAL_CANCEL"
        #会自动从第一个flow配置中复制全局取消配置（global_cancel_config）
        recall_service : "weather_rewrite_query"
    }
}
```

## 动态HTTP请求
使用场景：用户输入请求体中指定某个参数为数组，根据其输入数组长度发送与其相同数量的下游 HTTP 请求，每个请求中的指定参数对应数组中的每个元素。

### backend.conf
一个 backend service中的所有动态请求结果最后会合并成数组存储在$response变量中用于后续配置上的操作。
* service 内多个 HTTP 请求之间为同步并发，所有请求返回或失败或取消后进入 response 处理
* 动态 service 的返回结果根据 output 结构体的定义方式在最终排序中由两种存在方式
    1. response 仅有一个 output，key 为 `""` 或 `"/"` 且输出为每个元素都可以取得排序所需数值的**数组**，则每一个动态的 HTTP 请求作为单独的结果（可视作不同的service）在 rank 配置中进行排序
    2. response 的 output 为其他情况时，则所有该 service 内的动态请求最终以output定义的输出结果作为一个整体在 rank 配置中进行排序
```
backend {
    name : "test_poisearch_backend"
    server : "127.0.0.1:8156"
    protocol : "http"
    connect_timeout_ms : 100
    timeout_ms : 1500
    max_retry : 0
    # 设定 backend 的 is_dynamic 为 true 将使用动态HTTP请求，默认为 false
    is_dynamic : true
    service {
        name : "dynamic_poisearch"
        request {
            http_method : "get"
            http_uri : "/place/v2/search"
            http_query {
                key : "ak"
                expr : "get($request, 'ak')"
            }
            http_query {
                key : "output"
                expr : "'json'"
            }
            http_query {
                key : "callback"
                expr : "null"
            }
            http_query {
                key : "page_size"
                expr : "3"
            }
            dynamic_args_node : "query" #修改http_query中的参数
            #dynamic_args_path : "request/query" #修改http_body中的参数时，需要使用dynamic_args_path指定修改节点路径（支持UNIX-Like path）
            dynamic_args {
                key : "search_query" #仅在dynamic_args_node为query和uri时生效，body时无效，不可包含"/"
                expr : "get($request, 'poi_list')"
            } #输入中poi_list为数组，其中的元素会通过key赋值给对应参数，第一个动态请求等同于：
            #http_query {
            #    key : "query"
            #    expr : "get($request, 'poi_list/0')"
            #}
        }
        response {
            def {
                key : "poi1"
                expr : "json_decode(get($response, '0'))"
            }
            def {
                key : "poi2"
                expr : "json_decode(get($response, '1'))"
            }
            # 使用该output配置方法，dyanmic_poi_search的结果会作为一个整体在flow和rank中进行排序
            output {
                key : "/result"
                expr : "{'poi_list' : [$poi1, $poi2]}"
            }

            # 假设：rank.conf中配置排序依据为get($item, 'score')，使用以下output配置方式，则每个动态请求返回的结果都会作为单独的服务结果与其他后端结果进行排序：
            # output {
            #    key : ""
            #    expr : "[{'result': $poi1, 'score': 1}, {'result': $poi2 'score': 10}]"
            #}
        }
    }
}
```