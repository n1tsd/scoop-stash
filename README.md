# scoop-stash

个人自用的 [Scoop](https://scoop.sh) bucket，存放自编译工具或官方仓库（main/extras）里没有的软件。

## 使用

```powershell
scoop bucket add stash https://github.com/n1tsd/scoop-stash.git
scoop install stash/<app>
```

## 软件列表

| 应用 | 说明 |
| --- | --- |
| [reclaude](bucket/reclaude.json) | ReClaude CLI 启动器（Claude Code / Codex），提供 `reclaude` / `rec` / `recodex` / `recx` 四个原生命令入口；入口处拦截 `update` / `setup` / `uninstall`，防止绕过 scoop |
| [cursor2api](bucket/cursor2api.json) | [NGLSG/Cursor2API](https://github.com/NGLSG/Cursor2API) 的 Windows 桌面版（Tauri）。上游无 Windows 发布，由本仓库 Actions 每周一拉取 master 自行编译便携版发 Release |

## 目录结构

```
bucket/                  # manifest（*.json）
shim/                    # recshim 源码（reclaude 的原生命令入口）
assets/                  # GitHub Actions 云端编译产物（recshim-*.exe）
.github/workflows/
    excavator.yml        # 每日自动检查 manifest 更新
    build-shim.yml       # shim 源码变更时自动重建 assets
    cursor2api-build.yml # 每周拉取上游 Cursor2API master 编译 Windows 版并发布 Release
```

## 自动更新

每天 UTC 12:00（北京时间 20:00），GitHub Actions 运行
[ScoopInstaller/GithubActions](https://github.com/ScoopInstaller/GithubActions)（Excavator），
对所有带 `checkver` / `autoupdate` 字段的 manifest 自动检查并提交新版本，
也可在 [Actions](../../actions) 页面手动触发。

## recshim（reclaude 命令入口）

`shim/recshim.c` 是一个极小的原生 C 程序（zig 交叉编译 amd64 + arm64），
硬链接为 `reclaude` / `rec` / `recodex` / `recx` 四个命令入口：

- 原生命令行透传（argv[0] 分发，与官方 setup「复制二进制建入口」行为一致），无 BAT 的「终止批处理」提示
- 入口处拦截 `update` / `setup` / `uninstall` 子命令（`recodex app update` 不受影响）

修改 `shim/recshim.c` 后 push 即自动触发 [build-shim](../../actions/workflows/build-shim.yml)
重建 `assets/`；本地复现用 `pwsh shim/build.ps1`（需 zig 0.16）。

## 添加新 manifest

```powershell
scoop new <app>    # 在当前目录生成模板，编辑后移入 bucket/
```
