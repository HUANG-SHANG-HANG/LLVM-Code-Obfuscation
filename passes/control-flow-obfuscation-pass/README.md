# 混合不透明谓词插入策略设计文档

## 一、整体思路

在 LLVM IR 层面，对函数中的每个基本块进行控制流重构：
将原始基本块拆分为 **调度器块(scheduler)** + **真实块(real)** + **虚假块(fake)**，
在调度器中插入混合谓词决定跳转目标。

## 二、IR 修改前后对比

### 修改前（原始 IR）

entry:
%cond = icmp slt i32 %a, 10
br i1 %cond, label %if.then, label %if.else

if.then:
; ... 真实逻辑 ...
br label %if.end

if.else:
; ... 真实逻辑 ...
br label %if.end


### 修改后（混淆后 IR）
entry:
br label %scheduler ; 先跳到调度器

scheduler: ; 新增：调度器块
%chaos_val = call i1 @__obf_chaos_predicate()
%thread_val = call i1 @__obf_thread_predicate()
%mixed = or i1 %chaos_val, %thread_val ; OR 混合
br i1 %mixed, label %real_entry, label %fake_block

real_entry: ; 原始逻辑（语义不变）
%cond = icmp slt i32 %a, 10
br i1 %cond, label %if.then, label %if.else

fake_block: ; 新增：虚假块
; 包含迷惑性代码（异或运算、无意义调用等）
br label %real_entry ; 最终仍跳回真实块（保证语义正确）

if.then:
; ... 原始逻辑不变 ...
br label %if.end

if.else:
; ... 原始逻辑不变 ...
br label %if.end


## 三、控制流图对比

### 修改前 CFG
entry
/    \
if.then if.else
\ /
if.end


### 修改后 CFG
entry
  |
scheduler ──────► fake_block
| |
real_entry ◄───────────┘
/
if.then if.else
\ /
if.end


## 四、插入点选择策略

1. **遍历函数中的每个基本块**
2. **跳过特殊块**：入口块的第一个分支不处理（避免破坏函数签名）
3. **识别 BranchInst**：只处理包含条件分支或无条件分支的块
4. **插入调度器**：在原始块之前插入 scheduler 块
5. **插入虚假块**：在 scheduler 和 real_entry 之间插入 fake_block

## 五、运行时函数设计

Pass 插入的是对外部函数的 **调用指令**（call），实际实现在运行时库中：

| 函数 | 签名 | 功能 |
|------|------|------|
| `__obf_chaos_predicate` | `i1 ()` | 执行帐篷映射，返回 chaos_ok |
| `__obf_thread_predicate` | `i1 ()` | 读取 shared_flag，返回 thread_ok |
| `__obf_runtime_init` | `void ()` | 启动守护线程、初始化预计算值 |

## 六、语义保证

- **mixed = true（正常情况）**：跳转到 real_entry，执行原始逻辑 → 语义正确
- **mixed = false（不可能出现）**：跳转到 fake_block，fake_block 最终也跳回 real_entry → 语义仍然正确
- **关键保证**：无论谓词结果如何，程序最终都会执行原始逻辑
