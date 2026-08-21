# 交互说明实验室

静态页，不连接开发板。必须用 HTTP 预览（ES module 不能走 `file://`）。

```sh
# 仓库根。不要用 8080，避免和 daemon 默认端口混淆。
python3 -m http.server 8090 --directory site
# 打开 http://127.0.0.1:8090/
```

测试：

```sh
# 若去掉 site/package.json，Node 20 把 site/js/*.js 当 CommonJS：
node --input-type=module -e "import('./site/js/evidence-facts.js')"
# 期望：Unexpected token 'export'

# 恢复 {"type":"module"} 之后：
node --test site/test/*.test.mjs
```

公开地址（仓库 Public 且 Pages 部署后）：https://eric-h-h.github.io/imx6ull-sense-terminal/

改 `app/daemon/motion_detector.c` 或 M4 故障表时，同步 `site/js/motion-math.js` / `site/js/fault-machine.js` / `site/js/evidence-facts.js`。
