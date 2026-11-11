# scoop-stash

个人自用的 [Scoop](https://scoop.sh) bucket，存放自编译工具或官方仓库（main/extras）里没有的软件。

## 使用

```powershell
scoop bucket add stash https://github.com/n1tsd/scoop-stash.git
scoop install stash/<app>
```

## 目录结构

```
bucket/                  # manifest（*.json）
.github/workflows/
    excavator.yml        # 每日自动更新
```

## 自动更新

每天 UTC 12:00（北京时间 20:00），GitHub Actions 运行
[ScoopInstaller/GithubActions](https://github.com/ScoopInstaller/GithubActions)（Excavator），
对所有带 `checkver` / `autoupdate` 字段的 manifest 自动检查并提交新版本，
也可在 [Actions](../../actions) 页面手动触发。

## 添加新 manifest

```powershell
scoop new <app>    # 在当前目录生成模板，编辑后移入 bucket/
```
