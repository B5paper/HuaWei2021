1. 实现了 baseline，思想是 first fitting，如果现有的服务器都放不下，就只买 74 号服务器。没有迁移策略。
2. 重构了底层的数据结构，以 server 为主导，挂载虚拟机 vms。不再服务器状态和虚拟机状态并列存储。