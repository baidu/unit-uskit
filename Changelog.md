# Change Log

## [3.0.0] - 2021-06-16
### Added
* 新增 leveldeliver 模式 flow policy，支持 service 分发能力
* 新增规则干预机制，支持 service 干预和 flow 干预
* 支持无状态请求
* 新增默认启动参数配置 `conf/gflags.conf`
* 新增 `us.conf` 中相关配置
    * `required_param`：用户请求必传参数的配置，不配置时默认为 `logid`, `uuid`, `usid`, `query`
    * `root_dir`：USKit 配置的根目录，默认为 `./conf/us`
    * `input_config_path`：无状态请求的 json 配置路径
    * `editable_response`：是否直接输出 flow 的结果，不添加 `error_code` 与 `error_msg`，默认为 `false`
* 支持 flow node 中 output 配置名称包含 `__TMP__` 的变量作为全局变量，可在全局在 `$result` 中使用，最终输出时会统一删除
* 新增内置函数
    * `hmac_sha1`：获取 HMAC-SHA1
    * `base64_encode`：获取 base64 加密结果
    * `nonce`：生成指定长度的随机字符串
    * `query_encode`：对请求字符串进行编码
    * `split`：对字符串进行切分得到数组
    * `str_slice`：字符串切片
    * `join`：使用字符串连接数组
    * `str_length`：获取字符串长度
    * `str_find`：获取字符串的子串索引
* Backend service 配置新增 `success_flag`，用于检查当前 service 是否被成功调用
* 新增函数链式调用接口 fluent interface
### Changed
* `Closure` 机制解耦至 `src/controller_closure.h`
* hash 相关内置函数直接使用 OpenSSL EVP 库实现
### Fixed
* 修复不同 flow node 之间 output 时非正常 merge 的问题

## [2.0.2] - 2019-08-08
### Added
* 新增 us.conf 默认参数配置
* 新增 dynamic_host_default 模式 request/response policy, 支持运行时定义下游服务 ip:port
### Changed
### Fixed
* 修复 flow 中多个 rank 不生效的问题 [issue#14](https://github.com/baidu/unit-uskit/issues/14)

## [2.0.1] - 2019-07-24
### Added
* 新增异步并发模式
* 新增异步全局退出模式
* 新增动态模式（根据输入请求决定发送请求的数量和参数值）
* 新增多个自定义函数
### Changed
* 修改 butil 和 brpc 模块在代码中的 include 路径（对 binary 无影响）
* flow 输出结果中的 error_code 和 error_msg 会向上透传到最终返回的结果中
### Fixed
* 修复 flow 缺少禁止自循环跳转的问题

## [1.0.1] - 2019-05-14
初版发布
