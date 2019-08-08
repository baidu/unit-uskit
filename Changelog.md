# Change Log
## [2.0.2] - 2019-08-08
### Added
* 新增us.conf 默认参数配置
* 新增 dynamic_host_default 模式request/response policy, 支持运行时定义下游服务 ip:port
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