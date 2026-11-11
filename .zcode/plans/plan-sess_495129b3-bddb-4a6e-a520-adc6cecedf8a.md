# reclaude 原生命令入口：GitHub Actions 云端构建方案

## Context

BAT 包装在 Ctrl+C 时弹 `Terminate batch job (Y/N)?`，对交互式 TUI 不可接受。改为**原生 C 入口程序 recshim**（KB 级，硬链接成 reclaude/rec/recodex/recx 四个名字），实现：argv[0] 分发转发（与官方 setup「复制二进制建入口」行为一致）+ 入口处拦截 `update`/`setup`/`uninstall`（`recodex app update` 的 argv[1] 是 `app`，自动放行）。

**构建方式（按你的要求）**：本地只写源码，**GitHub Actions 用 zig 云端交叉编译** amd64 + arm64，产物自动 commit 回仓库 `assets/`；scoop 安装时 pre_install 从 `$bucketdir\assets\` 直接复制（bucket 本身就是 clone 下来的，零额外网络下载，CN 友好）。以后改拦截逻辑 = 改 `shim/recshim.c` + push，Actions 自动重建。

## 设计

### 1. `shim/recshim.c`（~150 行，无 CRT）

1. `GetModuleFileNameW` 取自身基名 → 入口身份
2. `CommandLineToArgvW`：`argv[1]` ∈ {update, setup, uninstall} → ASCII 提示 + exit 1
3. `FindFirstFileW` 定位同目录 `reclaude-windows-*.exe` → `CreateProcessW`（`lpApplicationName`=真实二进制，命令行首 token=自身入口名，参数原样，继承 stdio）
4. `SetConsoleCtrlHandler(NULL, TRUE)` 入口忽略 Ctrl+C（子进程自己处理）→ 无批处理终止提示
5. `WaitForSingleObject` + `GetExitCodeProcess` 透传退出码
6. 仅依赖 kernel32 + shell32

### 2. `.github/workflows/build-shim.yml`（新增）

- 触发：`workflow_dispatch` + `push` 限定 `paths: ['shim/**']`（产物提交到 `assets/`，不触发自身，无循环）
- runner: ubuntu-latest；下载 pinned zig 0.16.0（与本地一致）；`zig cc -target x86_64-windows-gnu / aarch64-windows-gnu -Os -nostdlib -Wl,-e,entry,--subsystem,console -lkernel32 -lshell32`
- `permissions: contents: write`；产物 strip 后 commit 到 `assets/recshim-amd64.exe`、`assets/recshim-arm64.exe` 并 push（github-actions[bot] 身份）

### 3. `bucket/reclaude.json` 修改

- `pre_install`（删除全部 .cmd 逻辑）：
  - `@{'64bit'='amd64';'arm64'='arm64'}[$architecture]` 选架构
  - `Copy-Item "$bucketdir\assets\recshim-<arch>.exe" "$dir\recshim.exe"`（bucket 是 git clone，完整性由 git 保证）
  - 硬链接 recshim.exe → `reclaude.exe`/`rec.exe`/`recodex.exe`/`recx.exe`（失败退化 Copy-Item）
- `bin`：四入口全指 exe（scoop 原生 shim → recshim → reclaude，三层全原生）
- notes 更新（入口说明 + 拦截说明 + 重建方式）

### 4. 其它文件

- 新增：`shim/recshim.c`、`shim/build.ps1`（本地复现用，与 CI 同参数）、workflow
- 修改：`README.md`（reclaude 表行 + 「改 shim 源码后 push 自动重建」说明）

## 验证

1. 本地 zig 同参数编译一次做冒烟（产物不入库，入库版以 CI 为准）：把 recshim 放到临时目录挨着已下载的 v1.8.3 二进制，直跑：`reclaude.exe version` → v1.8.3；**`recodex.exe version` → 有输出**（关键：argv[0] 分发穿透；失败则改兜底方案回报）；`reclaude update` → 拦截 + exit 1
2. `scoop uninstall reclaude` 清掉旧 BAT 版测试安装
3. 提交 shim 源码 + workflow + manifest + README，push
4. `gh run watch` 等 Actions 构建完成、assets 入库
5. 生产路径全链路：`scoop bucket add stash https://github.com/n1tsd/scoop-stash.git` → `scoop install stash/reclaude` → PATH 上 4 命令复测（version 转发、update 拦截、`recodex app update` 代码审查确认放行）
6. Ctrl+C 无「终止批处理」提示 → 你手动开 `reclaude` 交互会话确认
7. 之后 reclaude 新版由 excavator 每日自动更新 manifest（version/url/hash），与 shim 构建互不干扰