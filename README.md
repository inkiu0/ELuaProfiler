# ELuaProfiler
Easy LuaProfiler
## Supported Solutions
| Solution | Supported |
| ---      | ---       |
| *unlua-ue4* | √ |
| *slua-ue4* | × |
| *slua-unity* | × |
## Build
### UE4
1. configure Setup.bat/Setup.command
    - ```
      set src=E:\repo\ELuaProfiler\UE4\UnLua\
      set dst=E:\YourProject\Plugins\
      ```
2. Run Setup.bat/Setup.command
3. Build YourProject
### Unity
__待开发__
## Usage
### ELuaMonitor
ELuaMonitor主要用于剖析Lua的CPU性能，以及内存频繁开辟引起GC的问题，编辑器界面如图：
![](Samples/ELuaMonitor.png)
#### MonitorMode
右上角为模式选择控件，目前支持三种模式
1. PerFrame
    - 逐帧采样，并且可以按帧数前后回溯
2. Total
    - 累计采样，统计一段时间内的开销情况，最常用。
3. Statistics
    - 统计模式，在Total的基础上将所有节点平铺开，统计单函数的开销。
#### MonitorController
1. Play
    - 中间的按钮为Play，点击Play后，如果LuaVM启动了，会立刻开始Profile。否则等待LuaVM启动，自动开始Profiler
    - 开始Profile后按钮变为暂停键，可以暂停和恢复Profile
2. Clear
    - 最右侧为Clear按钮，点击Clear后会立刻停止Profile，并清空当前Profile数据
3. NextFrame/PrevFrame
    - 逐帧回溯，分别为上一帧和下一帧。
4. CurFrameIdx/TotalFrame
    - 当前帧数/总帧数
    - CurFrameIdx支持手动输入
    - 当0 < CurFrameIdx < TotalFrame，会停在CurFrameIdx这一帧
    - 否则CurFrameIdx会等于TotalFrame，并跟随TotalFrame一起增长
#### MonitorData
1. TotalTIme(ms)
    - 函数从Call到Return总共消耗的时间，单位为毫秒
    - 该值会受Profiler本身开销影响，主要是Profiler取Time的开销（因为Profiler取Time必须是同步的，但比如自增函数的开销比取Time开销还小，势必会造成误差，可以控制ProfileDepth来消除）
2. TotalTime(%)
    - 该函数调用时间开销占父节点的百分比
    - TotalTime / Parent.TotalTime
3. SelfTime(ms)
    - 代表该函数自身的开销
    - TotalTime减去各子节点的TotalTime
4. SelfTime(%)
    - 自身开销占父节点的比重
    - SelfTime / Parent.SelfTime
5. Average(ms)
    - 平均每次调用耗时
    - TotalTime / Calls
6. Alloc(kb)
    - 函数调用期间开辟的内存，单位为kb
7. Alloc(%)
    - 该函数开辟的内存占父节点的比重
    - Alloc / Parent.Alloc
8. GC(kb)
    - 函数调用期间释放的内存，因为Lua的GC为步进式，所以不能准确形容当前函数释放的内存。
    - 但一级节点的Alloc - GC可以作为整个Lua的内存增量，可以用来观察代码是否有明显泄露，但具体定位还应该使用ELuaMemAnalyzer。
9. GC(%)
    - 该函数调用期间发生的GC占父节点的比重
    - GC / Parent.GC
10. Calls
    - 该函数被调用次数
    
#### Max Depth
Max Depth控制Profile的最大深度，最小值为1，最大值为1000(可在代码中更改)，Max Depth可以有效消除Profiler的误差。从我的经验来说，Profiler的误差主要来源于Profiler的GetTime函数。而且GetTime必须是同步进行的，所以这部分误差会一直存在。考虑以下代码
```lua
function EmptyFunction()

end

function Counting()
    for i = 1, 1000 do
        EmptyFunction()
    end
end
```
当我们统计到`Counting`函数的时候，会统计到1000次`EmptyFunction`函数的开销。但由于`EmptyFunction`函数的开销过小，甚至比Profiler的GetTime函数的开销还小。所以`Counting`的统计势必存在很大的误差，这个时候我们可以将MaxDepth设定在`Counting`这一层，不再继续展开，不统计过细的分支(在这个例子中指`EmptyFunction`)，我们就能正确地观察到`Counting`的性能开销。

一般我们在实战中，从1开始慢慢增加Depth，直到我们停在一个合适的地方。
#### 排序
目前是按照`TotalTime`降序排列，后续会支持点选Title进行不同数据的排序。

### ELuaMemAnalyzer
_UI界面开发中..._
#### Snapshot
在ELuaMemAnalyzer中点击一次采样，即可生成一个`Snapshot`。`Snapshot`中包含了当前时刻的内存情况，以`_G`为根节点。
同时`Snapshot`也支持逻辑运算，以方便剖析内存的泄露和增长。
#### Snapshot Logic Operation
以`Snapshot`为单元进行逻辑运算，目前支持`&与`和`|或`运算，这两个运算配合强制GC可以很好地查找泄露。
1. `&` Operation
    - 两颗`Snapshot`进行与运算，得到两个时刻相同的内存部分。
2. `|` Operation
    - 两颗`Snapshot`进行或运算，得到两个时刻总的内存部分，相当于A + B
3. `^` Operation
    - 两颗`Snapshot`进行异或预算，得到两个时刻的内存差异部分。
3. Memory Leak
    - 在刚启动游戏时，或强制GC后，采样得到`SnapshotA`
    - 在游戏内进行游玩时，每隔5分钟采样，得到`SnapshotB`、`SnapshotC`、`SnapshotD`
    - `常驻内存` = `SnapshotA` & `SnapshotB` & `SnapshotC` & `SnapshotD`
#### Deviation
由于种种原因，内存统计的误差都不可避免。误差主要来自于这几方面：
1. Lua的`Intern机制`
    - 由于Lua对短字符串和`number`采用了`Intern机制`，所以这部分存在重复统计。
    - 后续优化考虑做一个去除。
2. 多重引用，比如A:{B: {D, E}, C: {D, F}}，这样一颗三层的树。
    - 为了保证单独查看节点B和节点C的正确性，`B = D + E`，`C = D + F`
    - 所以`A = D + E + D + F`，存在一定的误差。
## Roadmap
### 1. ELuaMemAnalyzer
为ELuaMemAnalyzer编写EditorUI界面
### 2. Support slua-ue4
接入slua-ue4
### 3. Remote Profile
将ELuaProfiler分为Server和Client，支持真机远程Profile
### 4. Serialize & Deserialize
支持Profile数据的序列化和反序列化
### 5. Support slua-unity
接入slua-unity，并编写Unity版本的EditorUI
### 6. Support unlua-unity
接入unlua-unity
### 7. Support xlua-unity
接入xulua-unity
