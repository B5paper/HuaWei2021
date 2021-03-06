1. 精心地维护了一个散列表，保证它不出问题，实现了完全用散列表进行迁移和调度。效率提升明显。但是不够稳定，对于某些天，处理的速度会慢很多很多。原因可能是在那些天里，找不到可迁移的虚拟机，几乎所有的虚拟机都很难迁移。降低了迁移次数就好了。


想法：
迁移策略：
用于存放单节点虚拟机的服务器：
首先考虑老的服务器：（满足阈值 thresh_1 的服务器就是老服务器）
设定两个阈值 thresh_1 和 thresh_2。其中 thresh_1 为核心数和内存数的最小限度，若其中有一个数小于 thresh_1，那么计算这两个数的差值，若差值大于 thresh_2，那么说明这台服务器的节点的资源分配不均衡。可以从这台服务器中迁出去一个虚拟机，再迁进来一个新的，使得核心数和内存数满足两个阈值要求。迁出去的虚拟机满足：若节点的核心数小于 thresh_1，那么虚拟机的核心数要大于内存数；若节点的内存数小于 thresh_1，那么虚拟机的内存数要大于核心数。迁出去的虚拟机可以先放到新的服务器上。
然后考虑新的服务器：
采用二次迁移策略。
1. 将能迁移的虚拟机迁移到老节点上，若虚拟机的核心数较大，那么首先考虑匹配核心数的节点，然后再考虑使得服务器节点剩余的内存数尽量得少。
2. 将第一次迁移剩下的虚拟机，再次尽量归并到同一个服务器上。

用于存放双节点虚拟机的服务器：
如果其总是存放双节点的虚拟机，那么就可以将这个服务器简化为单节点。所以它也必须满足两个阈值限制的平衡需求。此时它的迁移策略与单节点的迁移策略相同。
但是专门存放双节点虚拟机的服务器，也可以用于存放单节点虚拟机。
在双服务器迁移之前，需要先将单节点的虚拟机迁移出去；然后完成自己的迁移策略；最后在单服务器迁移之后，再将新服务器中的虚拟机尽量迁移到双服务器上。
迁移顺序是：双服务器将单虚拟机迁出，双服务器内部迁移，单服务器内部迁移。注意这里不需要将单虚拟机迁入到双服务器上，因为新的一天已经开始，即将处理 add 请求。

空的服务器：
如果一个服务器是空的，那么它即可以是单服务器，也可以是双服务器。这取决于它接受的第一台虚拟机是什么样的。


调度策略：
单节点的虚拟机优先考虑单服务器，对于老服务器，直接使用 best fit；若老服务器放不下，则对新服务器使用 best fit。若新服务器也放不下，则对空服务器使用 first fit。否则就买新服务器。
双节点的虚拟机优先考虑双服务器，策略同上。若探测到当天剩余的 add 请求中不再有双节点的虚拟机，那么可以把单节点虚拟机放到双服务器上。

理想结果：
1. 老的服务器都处于平衡状态，不会有核心数/内存数一方资源占尽，而另一方还剩很多。
2. 空的服务器尽量多。
