## 自定义函数和策略

在US Kit内置的表达式函数和策略无法满足你的需求时，你可以通过编写C++代码来对系统进行自定义扩展，一般分为以下三步：

1. 实现相应的接口
2. 将新的函数/策略注册到系统中
3. 重新编译US Kit

### 自定义函数
函数接口定义(src/function/function_manager.h)：

```
typedef int (*FunctionPtr)(rapidjson::Value& args,
                           rapidjson::Document& return_value);
```

参数：

* args：参数列表，以rapidjson数组传递
* return_value：返回值，以rapidjson文档格式返回

返回： 0 表示成功，-1表示失败

样例：

下面实现一个将int值加1的自定义函数，该函数接收一个int值，并加1返回
```
int int_add_one(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1 || !args.IsInt()) {
        return -1;
    }

    return_value.SetInt(args[0].GetInt() + 1);

    return 0;
}
```

#### 注册自定义函数
在src/global.cpp文件中的`register_function`中添加下面一行：

```
void register_function() {
    REGISTER_FUNCTION("get", function::get_value_by_path);
    ...
    REGISTER_FUNCTION("time", function::time);

    // 自定义函数
    REGISTER_FUNCTION("int_add_one", int_add_one);
```
成功编译US Kit后，就可以在配置表达式中使用名为`int_add_one`的函数了

### 自定义backend request策略
策略基类接口定义(src/policy/backend_policy.h)：

```
class BackendRequestPolicy {
public:
    BackendRequestPolicy() {}
    virtual ~BackendRequestPolicy() {}

    virtual int init(const RequestConfig& config, const Backend* backend) { return 0; }
    virtual int run(BackendController* cntl) const = 0;
};
```

子类需要继承BackendRequestPolicy并实现run方法，自定义请求构造策略，如果需要做一些初始化相关的逻辑，可以选择实现init方法

样例：

```
class MyRequestPolicy : public BackendRequestPolicy {
public:
    int init(const RequestConfig& config, const Backend* backend) { return 0; }
    int run(BackendController* cntl) { 
        // My request policy
        ...
        return 0;
    }
};
```

#### 注册backend request自定义策略
在src/global.cpp文件中的`register_policy`中添加下面一行：

```
void register_policy() {
    // request policy
    REGISTER_REQUEST_POLICY("http_default", policy::backend::HttpRequestPolicy);
    REGISTER_REQUEST_POLICY("redis_default", policy::backend::RedisRequestPolicy);

    // 自定义request policy
    REGISTER_REQUEST_POLICY("my_request_policy", MyRequestPolicy);
}
```

成功编译US Kit后，就可以在`backend.conf`配置中将请求构造策略指定为`my_request_policy`：
```
backend {
    service {
        ...
        request_policy : "my_request_policy"
        ...
    }
}
```

### 自定义backend response策略
策略基类接口定义(src/policy/backend_policy.h)：

```
class BackendResponsePolicy {
public:
    BackendResponsePolicy() {}
    virtual ~BackendResponsePolicy() {}

    virtual int init(const ResponseConfig& config, const Backend* backend) { return 0; }
    virtual int run(BackendController* cntl) const = 0;
};
```

子类需要继承BackendResponsePolicy并实现run方法，自定义结果解析策略，如果需要做一些初始化相关的逻辑，可以选择实现init方法

样例：

```
class MyResponsePolicy : public BackendResponsePolicy {
public:
    int init(const ResponseConfig& config, const Backend* backend) { return 0; }
    int run(BackendController* cntl) { 
        // My response policy
        ...
        return 0;
    }
};
```

#### 注册backend response自定义策略
在src/global.cpp文件中的`register_policy`中添加下面一行：

```
void register_policy() {
    // response policy
    REGISTER_RESPONSE_POLICY("http_default", policy::backend::HttpResponsePolicy);
    REGISTER_RESPONSE_POLICY("redis_default", policy::backend::RedisResponsePolicy);

    // 自定义response policy
    REGISTER_RESPONSE_POLICY("my_response_policy", MyResponsePolicy);
}
```

成功编译US Kit后，就可以在`backend.conf`配置中将结果解析策略指定为`my_response_policy`：
```
backend {
    service {
        ...
        response_policy : "my_response_policy"
        ...
    }
}
```

### 自定义rank策略
策略基类接口定义(src/policy/rank_policy.h)：

```
class RankPolicy {
public:
    RankPolicy() {}
    virtual ~RankPolicy() {}

    virtual int init(const RankNodeConfig& config) { return 0; }
    virtual int run(RankCandidate& rank_candidate, RankResult& rank_result,
                    expression::ExpressionContext& context) const = 0;
};
```

子类需要继承RankPolicy并实现run方法，自定义排序策略，如果需要做一些初始化相关的逻辑，可以选择实现init方法

样例：

```
class MyRankPolicy : public RankPolicy {
public:
    int init(const RankNodeConfig& config) { return 0; }
    int run(RankCandidate& rank_candidate, RankResult& rank_result,
            expression::ExpressionContext& context) {
        // My rank policy
        ...
        return 0;
    }
};
```

#### 注册rank自定义策略
在src/global.cpp文件中的`register_policy`中添加下面一行：

```
void register_policy() {
    // rank policy
    REGISTER_RANK_POLICY("default", policy::rank::DefaultPolicy);

    // 自定义response policy
    REGISTER_RANK_POLICY("my_rank_policy", MyRankPolicy);
}
```

成功编译US Kit后，就可以在`rank.conf`配置中将排序策略指定为`my_rank_policy`：
```
rank {
    ...
    rank_policy : "my_rank_policy"
    ...
}
```

### 自定义flow策略
策略基类接口定义(src/policy/flow_policy.h)：

```
class FlowPolicy {
public:
    FlowPolicy() {}
    virtual ~FlowPolicy() {}

    virtual int init(const google::protobuf::RepeatedPtrField<FlowNodeConfig> &config) { return 0; }
    virtual int run(USRequest& request, USResponse& response,
                    const BackendEngine* backend_engine,
                    const RankEngine* rank_engine) const = 0;
};
```

子类需要继承FlowPolicy并实现run方法，自定义流程控制策略，如果需要做一些初始化相关的逻辑，可以选择实现init方法

样例：

```
class MyFlowPolicy : public FlowPolicy {
public:
    int init(const google::protobuf::RepeatedPtrField<FlowNodeConfig> &config) { return 0; }
    int run(USRequest& request, USResponse& response,
            const BackendEngine* backend_engine,
            const RankEngine* rank_engine) {
        // My flow policy
        ...
        return 0;
    }
};
```

#### 注册flow自定义策略
在src/global.cpp文件中的`register_policy`中添加下面一行：

```
void register_policy() {
    // flow policy
    REGISTER_FLOW_POLICY("default", policy::flow::DefaultPolicy);

    // 自定义response policy
    REGISTER_FLOW_POLICY("my_flow_policy", MyFlowPolicy);
}
```

成功编译US Kit后，就可以在`flow.conf`配置中将排序策略指定为`my_flow_policy`：
```
flow_policy : "my_flow_policy"

flow {
    ...
}
```
