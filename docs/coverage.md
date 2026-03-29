# C++ 代码覆盖率：原理、Bazel 配置与操作

本仓库用 Bazel 的 **`bazel coverage`** 与 **lcov** 汇总报告。本文说明**统计在算什么**、**Bazel 与 GCC 如何配合**、**沙箱扮演的角色**，以及**日常怎么跑、常见问题怎么排**。

## Attention：GCC 与 gcov 必须配套

**覆盖率采集能否成功，首先取决于工具链是否成对，而不是仅取决于「是否安装了某个 gcov」。**

| 要求 | 说明 |
|------|------|
| **同一主版本** | 生成 **`.gcno`** 的 **`gcc`** 与解读 **`.gcno`/`.gcda`** 的 **`gcov`** 必须来自**同一 GCC 主版本**（例如都用 9.x 或都用 13.x）。二者不是独立程序：`.gcno` 内含版本标签，**错配**会出现 `version 'A95*', prefer 'B33*'`、**`gcov` 段错误**或空报告。 |
| **同一条「安装线」** | 优先使用**与 `gcc` 可执行文件同目录、同名规则配对**的 `gcov`（如 `x86_64-linux-gnu-gcc-9` ↔ `x86_64-linux-gnu-gcov-9`）。仅把「系统里某个较新的 gcov」装好了，但编译仍走 **`/usr/bin/gcc` → gcc-9**，而默认 **`gcov` → gcov-13**，问题依旧。 |
| **必须传到采集动作** | 即便本机已安装正确 `gcov`，也需通过 **`GCOV`** 让 **`collect_cc_coverage.sh`** 实际调用它（本仓库用 **`.bazelrc`** 的 **`--action_env=GCOV`**，并推荐 **`./tools/coverage.sh`** 自动推导）。否则沙箱内仍可能只用 PATH 上的默认 `gcov`。 |

**结论**：先确认 **Bazel 实际用来编译 C++ 的编译器**（与 **`${CC}` / `gcc`** 一致），再选用**与之配套的 `gcov`**，并保证 **`GCOV`** 指向该二进制。下文「必知：`GCOV` 与编译器一致」一节有典型报错与操作示例。

## 1. 覆盖率在统计什么（GCC + gcov + lcov）

对 **C/C++**，常用流程是：

1. **编译期插桩**  
   编译器在生成代码时加入计数器（例如使用 `-fprofile-arcs -ftest-coverage` 一类选项；具体由工具链与 Bazel 封装决定）。  
   同时生成 **`.gcno`（notes）** 文件，记录「基本块 ↔ 源码行」的对应关系。

2. **运行期写入**  
   程序执行时，计数器更新；正常退出时写出 **`.gcda`（data）** 文件，与 `.gcno` 成对使用。

3. **`gcov` 工具**  
   读取 **`.gcno` + `.gcda`**，把二进制计数还原成**逐行/逐分支**的覆盖信息（人类可读或中间格式）。

4. **`lcov` 聚合**  
   把多个源文件、多个目标的结果**合并**成一份报告（本仓库通过 Bazel 的 **`--combined_report=lcov`** 生成合并输出）。

**行覆盖率**常见含义：某行是否被执行过（statement/line coverage）；更细还有分支覆盖等，取决于工具链与报告格式。

**本仓库关注点**：在配置 **`--config=coverage`** 下，仅对 [`instrumentation_filter`](../.bazelrc) 中声明的包做插桩（当前为 **`//src/kv`** 与 **`//tests`**），避免对无关第三方整库插桩、加快构建。

## 2. Bazel 侧：采集与合并

根目录 [`.bazelrc`](../.bazelrc) 中 **`common:coverage`** 启用：

| 选项 | 作用 |
|------|------|
| **`--action_env=GCOV`** | 把当前环境中的 **`GCOV`** 传入测试/采集动作（沙箱内 **`collect_cc_coverage.sh`** 会调用它）；与下面脚本自动配对配合。 |
| **`--collect_code_coverage`** | 要求 Bazel 在覆盖构建/测试中启用覆盖率收集流程。 |
| **`--instrumentation_filter=...`** | 限制**插桩范围**（本仓库为 `//src/kv` 与 `//tests`）。 |
| **`--combined_report=lcov`** | 测试结束后**合并**为 lcov 格式报告；终端日志里会出现 **`LCOV coverage report is located at ...`**，指向生成文件路径。 |

推荐使用仓库脚本 [`tools/coverage.sh`](../tools/coverage.sh)：在**未**设置 **`GCOV`** 时，会按 **`${CC:-gcc}`** 推导配套的 **`gcov`**；在 **`bazel coverage`** 成功后**自动**用 **`genhtml`** 生成 **`build/coverage-html/index.html`**（依赖系统已安装 **lcov**）。可用 **`COVERAGE_HTML_DIR`** 改输出目录，**`SKIP_COVERAGE_HTML=1`** 跳过 HTML（例如无 lcov 的 CI）。

## 3. 沙箱机制（sandbox）与覆盖率的关系

**Bazel 的 Linux 沙箱**（默认策略下）大致做这些事：

- 为**每个动作（action）**准备**隔离的执行目录**，只暴露声明过的输入与工具链，**限制**对宿主机的随意读写。
- 使构建与测试更**可复现**：减少「本机环境碰巧能跑」的情况。

**与覆盖率的关系**：

- 测试二进制仍在沙箱约束下运行；**`.gcda` 的生成位置**由编译选项与工作目录决定，Bazel 在覆盖率流程中负责**收集**这些产物并进入后续 **`gcov`/`lcov`** 管道。
- 若你看到 **`gcov` 崩溃** 或 **版本不匹配**，通常不是沙箱「坏了」，而是 **`gcc` 与 `gcov` 主版本不一致**（见下一节）。

若需调试沙箱相关问题，可查 Bazel 文档中的 **sandboxing** 与 **`--sandbox_debug`** 等选项（本仓库文档不展开所有 flag）。

## 4. 操作步骤（推荐）

在**仓库根目录**执行。

**首选：脚本（自动匹配 `gcc` / `gcov`）**

```bash
chmod +x tools/coverage.sh   # 仅需一次
./tools/coverage.sh //tests:kv_store_test
```

**直接调用 Bazel（请自行保证 `GCOV` 与编译器一致，见下节）：**

```bash
bazel coverage --config=coverage //tests:kv_store_test
```

使用 **[`tools/coverage.sh`](../tools/coverage.sh)** 时，成功结束后会在 **`build/coverage-html/`**（或 **`COVERAGE_HTML_DIR`**）生成 **`index.html`**，无需再跑其它脚本。若**直接**调用 **`bazel coverage`**，则只会得到 lcov **`.dat`**，见下文 **「HTML 报告（index.html）」**。

### 日志里的两行 `LCOV coverage report` 与 `Executed 0 out of 1`

- 终端里先后出现 **`_baseline_report.dat`** 与 **`_coverage_report.dat`** 的路径是**正常现象**：前者为基线，后者为合并后的覆盖率数据；**`genhtml`** 使用的是 **`_coverage_report.dat`**。
- **`Executed 0 out of 1 test`** 且测试行带 **`(cached)`** 表示：**本次没有重新执行测试进程**，Bazel 复用了**上一次**的测试结果（以及与之关联的 coverage 缓存）。改代码后若希望**重新跑测试并刷新**覆盖率/HTML，请任选其一：
  - **`COVERAGE_NOCACHE=1 ./tools/coverage.sh //tests:kv_store_test`**（脚本会加上 **`--nocache_test_results`**），或
  - **`./tools/coverage.sh --nocache_test_results //tests:kv_store_test`**

## 5. 必知：`GCOV` 与编译器一致

收集阶段会调用 **`gcov`** 处理 **`gcno`/`gcda`**。`.gcno` 由 **某个主版本的 `gcc`** 生成，**必须**用**同主版本**的 **`gcov`** 解读；否则会看到类似：

```text
... file.gcno: version 'A95*', prefer 'B33*'
... Segmentation fault ... "${GCOV}" -i -b ...
```

这通常表示：**默认的 `/usr/bin/gcov`（例如指向 gcov-13）** 与 **实际用来编译的 gcc（例如 gcc-9）** 不一致——常见于 **`update-alternatives`** 或 PATH 上混装多版 GCC。

**处理办法：**

1. **用 [`tools/coverage.sh`](../tools/coverage.sh)**（按 **`${CC:-gcc}`** 推导配套 `gcov`，并已通过 **`.bazelrc`** 中的 **`--action_env=GCOV`** 传入沙箱）。
2. 或**手动**指定与 **`realpath "$(command -v gcc)"`** 同套件的 `gcov`，例如：

```bash
GCOV=/usr/bin/x86_64-linux-gnu-gcov-9 bazel coverage --config=coverage //tests:kv_store_test
```

使用 **Clang** 等其它编译器时，需自行选用对应的覆盖工具（如 **`llvm-cov gcov`**），并显式设置 **`GCOV`**。如何确认 Bazel 用的编译器：可看构建子命令，或参考 [`compile_commands.json`](ide_indexing.md) 中的编译器路径。

## 6. 与 `bazel test` 的区别

- **`bazel test`**：只关心测试通过与否，**不**生成合并覆盖率报告（除非你自己叠加大致相同的 coverage 配置）。
- **`bazel coverage`**：在覆盖率配置下跑测试并**产出覆盖率工件**；本仓库期望与 **`--config=coverage`** 一起使用。

更短的命令表仍见 [build.md](build.md)。

## 7. HTML 报告（`index.html`）在哪里

**Bazel 本身不生成 `index.html`。**  
`--combined_report=lcov` 产出的是 **lcov tracefile**（**`_coverage_report.dat`**）；**[`tools/coverage.sh`](../tools/coverage.sh)** 在 **`bazel coverage`** 成功后，会调用 **`genhtml`** 写入默认目录 **`build/coverage-html/`**（需已安装 **lcov**，Ubuntu：`sudo apt install lcov`）。

| 环境变量 | 含义 |
|----------|------|
| **`COVERAGE_HTML_DIR`** | HTML 输出目录（默认 **`build/coverage-html`**） |
| **`SKIP_COVERAGE_HTML=1`** | 只跑 **`bazel coverage`**，不执行 **`genhtml`**（例如 CI 未装 lcov） |
| **`COVERAGE_NOCACHE=1`** | 传入 **`--nocache_test_results`**，强制重跑测试（避免「Executed 0 out of 1」时沿用旧覆盖率） |

**若只用手写 `bazel coverage`**，则需自行对合并后的 **`.dat`** 执行 **`genhtml`**。合并文件的典型路径为：

```text
$(bazel info execution_root)/bazel-out/_coverage/_coverage_report.dat
```

示例：

```bash
exec_root="$(bazel info execution_root)"
genhtml -o build/coverage-html "${exec_root}/bazel-out/_coverage/_coverage_report.dat"
```

**说明**：若从未成功跑过 coverage，**`_coverage_report.dat`** 不存在，脚本会报错。若只有 **`_baseline_report.dat`** 而没有合并后的 **`_coverage_report.dat`**，请先确认测试与 coverage 已成功完成（见日志中的 **`INFO: LCOV coverage report`**）。
