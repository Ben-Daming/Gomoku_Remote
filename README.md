# 说明文档
## 1. 概述
本项目是国科大24级本科生的c程设大作业五子棋实现。该程序有以下功能：
- 支持pvp,pve两种玩法
- 支持自由无限制和标准禁手（含复杂解禁）两种规则
- 支持悔棋 **2025.12.10 04:00 新增：悔棋对ai智力的影响已修复，但是悔棋后的第一步搜索可能会较慢，为正常现象**
- 支持保存、载入棋谱，若保存失败，请确保可执行文件有写入、修改文件权限。**(2025.12.10 04:00 新增)**
## 2. 目录结构
```
Gomoku/
├── Makefile
├── build/                # 可执行文件目录
├── include/              # 头文件目录
│   ├── ai.h
│   ├── ascii_art.h
│   ├── ascii_art.h
│   ├── board.h
│   ├── bitboard.h
│   ├── evaluate.h
│   ├── history.h
│   ├── rules.h
│   ├── start_helper.h
│   ├── tt.h
│   ├── types.h
│   ├── zobrist.h
│   └── record.h
├── src/                  # 源代码目录
│   ├── ai.c
│   ├── ascii_art.c
│   ├── bitboard.c
│   ├── board.c
│   ├── evaluate.c
│   ├── history.c
│   ├── main.c
│   ├── rules.c
│   ├── start_helper.c
│   ├── tt.c
│   ├── zobrist.c
│   └── record.c
├──Develop_Doc.md
└── README.md
```

## 3. 构建与运行

### 使用 Makefile
- 在根目录执行：
```bash
make
```
- 常用 targets:
  - `make`: 基本构建，默认参数会开启 `-O3 -g -march=native -fopenmp`
  - `make clean`: 清理构建产物
  - `make release`: 产出带有 LTO 的最高优化二进制文件（MAD flags）


### 运行
本人不熟悉windows，以下命令行指令在unix类系统中通用
```bash
make release
./build/gomoku-release
```
无参数时，默认会使用pve+标准禁手


```bash
./build/gomoku-release --mode pvp
```
可以使用`--mode`来显示指明采用pvp/pve模式

若尝试无禁手模式，则可以使用`--rules`来显式指定规则集，例如指定在无禁手规则下pvp对战：
```bash
./build/gomoku-release --mode pvp --rules simple 
```

输入`./build/gomoku-release --help`可以查看相关参数

若有保存棋谱的需求，在退出游戏时根据指示输入`yes`,程序会自动将棋谱以保存时间为文件名保存到`./game_records`目录中，若该目录不存在，会自动创建

加载棋谱：使用命令行参数`--load <文件路径/文件名>`即可，游戏模式、规则等自动和棋谱内容保存一致，先后手可选
```bash
./build/gomoku-release --load ./game_records/2025-12-10_03_53_19.txt
```


## 4.开发者
- 本项目欢迎参考代码及出于学习用途的fork
- 如有进一步开发的意愿，可以使用遗留的开发者模式构建和运行
```bash
make debug
./build/gomoku --debug Renju #测试禁手的debug模式
```
- 如有并发编程的需求，请记得移除`ascii_art`相关的全局变量域
## 5.杂项
本项目的部分代码使用了矢量化指令集（如 AVX2）并对编译器加入向量化提示以提高性能。如果在不支持这些指令集的环境中运行，某些实现（例如向量化的 `evaluateLines4`）可能会表现不佳。仓库历史里在提交 `ef918c3aeda1f8761c8b1d6388297dbe207ec2ca` 提供了一个不依赖矢量化指令的 `evaluateLines4` 实现，若需将 `src/evaluate.c` 恢复到该版本，请运行：
```bash
git restore --source ef918c3aeda1f8761c8b1d6388297dbe207ec2ca -- src/evaluate.c
```
该版本的实现在x86指令集，gcc编译环境下相比向量化实现稍慢了5%左右，但无需矢量化指令集

## 6.鸣谢
- 感谢[@Squareless-XD](https://github.com/Squareless-XD)热心提供五子棋规则和对战细节的指导，给了我很多启发，并引发了我对棋类游戏的兴趣
- 开局定式的硬编码移植自[@Squareless-XD](https://github.com/Squareless-XD)的[gomoku仓库](https://github.com/Squareless-XD/2022Fall-UCAS-C-Gomoku)
- [@LucasLan666666](https://github.com/LucasLan666666/)也热心提供github仓库并给我带来测试ai用例和字符艺术等启发。我在`main.c`中绘制了一幅超天酱的ascii画(\^_\^)
- 此外还有诸多热心帮助我测试程序的国科大同学，感谢你们愿意腾出时间陪我一起玩😭
- 最后感谢我自己，通过实现大学以来第一个真正意义上的个人项目，让自己感到了很久没有体验过的真实快乐。
## 参考资料
- 一本对弈类棋游戏通用的[简明算法教程](https://www.xqbase.com/computer.htm)
- [oi wiki](https://oi-wiki.org/search/alpha-beta/)对基本alpha-beta搜索算法的介绍
- 一个增强算法发明者本人的介绍网页[mtd(f)](https://people.csail.mit.edu/plaat/mtdf.html)
- 助教[@Squareless-XD](https://github.com/Squareless-XD)的[gomoku仓库](https://github.com/Squareless-XD/2022Fall-UCAS-C-Gomoku)