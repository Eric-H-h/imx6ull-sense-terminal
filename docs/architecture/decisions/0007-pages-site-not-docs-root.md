# ADR-0007：公开交互模型放在 `site/`，由 Actions 从 main 发布

## Status

Accepted

## Context

仓库需要一个面向浏览器的两分钟系统模型。GitHub Pages 只能从分支根或 `/docs` 发布，或使用 Actions。现有 `docs/` 是 Docs-as-Code 分类，不能当站点根。`docs/README.md` 曾写网站源码在 `tools/report-sites/`，那是 M2 生成 HTML，不是公开模型。

## Considered Options

- 把 Pages 设为 Deploy from branch `/docs`。
- 使用 `gh-pages` 产物分支。
- 在 `site/` 维护静态页，Actions 从 `main` 发布子集。

## Decision

公开交互模型的源码在仓库 `site/`。GitHub Pages 使用 GitHub Actions，从 `main` 发布 `index.html`、`css/`、`js/`、`svg/`，不含测试。永不把 Pages 设为 Deploy from branch `/docs`。M2 生成 HTML 仍可留在 `docs/reports/`，不是这套公开模型。

## Consequences

### Positive

- `docs/` 继续只当文档。
- 相对路径可发布到 `https://eric-h-h.github.io/imx6ull-sense-terminal/`。

### Negative

- 第一次需要在 GitHub UI 批准 `github-pages` environment，并把 Pages 来源设为 GitHub Actions。

### Neutral / Follow-up

- 根 README 的主页链接等到 Pages 对外 200 再加。

## Verification

- `node --test site/test/*.test.mjs`
- 本地 `python3 -m http.server 8090 --directory site`
- 合入 `main` 后 workflow `pages` 部署成功

## Related

- 相关 ADR：[0005](0005-use-docs-as-code-structure.md)
