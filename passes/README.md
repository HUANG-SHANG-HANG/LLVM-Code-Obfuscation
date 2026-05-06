# passes/ — LLVM IR 多策略混淆模块

## 输出目录说明

| 目录 | 内容 | 用途 |
|------|------|------|
| `tests-outputs/non-obf-ir/` | 原始 LLVM IR | 论文对照组 |
| `tests-outputs/non-obf-elf/` | 原始可执行文件 | 论文对照组 |
| `tests-outputs/obf-ir/` | **多策略混淆后 IR** | 论文实验数据 |
| `tests-outputs/obf-elf/` | **多策略混淆后可执行文件** | 论文实验数据 |
| `tests-outputs/unit-test-output/` | 单独混淆产物 | 调试用，不入论文 |

## 编译
```bash
bash scripts/build_all.sh
