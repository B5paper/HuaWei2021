# README

1. 修改了选型策略。
1. 将处理方式又改回了顺序处理
1. 维护了一个非空服务器的状态表，大大减少了查询时间
1. 调整了一些参数，达到了目前的最优



经验：

1. 在选型时有一个整型回绕的 bug。因为核心数和内存数加起来的资源数超过了`int`型的最大值。而且`int`型在 windows 上和 linux 上的长度似乎不一致。

    目前遇到的线下和线上不一致的情况的原因：

    1. 数组越界，本地中越界处内存的值与线上越界处内存的值不一样
    1. `INT_MAX`与`INT32_MAX`的区别
    1. 因为整型长度不一致导致的回绕
    1. 有一个变量忘了加`static`修饰，从而使用了未初始化的变量，使得线上和线下的结果不同

1. 若出现红黑树的 bug，基本就两个原因。要么是迭代器失效，要么是创建迭代器时`find()`返回了`end()`，但是仍然使用了这个迭代器。
