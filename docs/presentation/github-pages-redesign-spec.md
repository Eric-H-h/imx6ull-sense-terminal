# GitHub Pages 项目展示页完整设计方案

## 文档状态

- 状态：Design proposal
- 适用页面：`site/`
- 部署环境：GitHub Pages
- 实现状态：尚未实施
- 事实来源：架构文档、ADR、阶段总结和 `docs/verification/`

本文只定义公开项目展示页的内容结构、视觉系统和交互规则，不修改当前页面实现，也不替代项目验证文档。

## 1. 项目定位

页面对外是一份完整、可信的开源项目主页，同时让面试官自然看到项目作者的工程能力。

页面不直接陈述“作者掌握了某项技能”，而是通过真实约束、设计决策、技术取舍和验证证据，让访客自行判断：

- 能完成嵌入式 Linux 端到端闭环；
- 能设计 C11/pthread 并发数据路径；
- 能在资源受限 ARM 平台上进行性能取舍；
- 能定义故障边界、恢复策略和 systemd 服务契约；
- 能用测试、文档、ADR 和证据管理项目。

页面定义为：

> 基于真实板端验证结果构建的交互式工程案例，不是实时开发板控制台。

## 2. 设计目标

### 2.1 首次访问

访客应在 15 秒内回答：

1. 这是什么项目；
2. 它运行在什么平台；
3. 它完成了什么闭环；
4. 为什么值得继续阅读。

### 2.2 深入阅读

访客应在 1 至 3 分钟内理解：

- 系统数据如何从 UVC 摄像头流向浏览器和运动检测；
- 为什么采用 latest-frame、低频灰度抽样和 degraded 恢复；
- 哪些结论经过真实开发板验证；
- 如何进入源码、文档和 Release。

### 2.3 视觉体验

- 页面应有明确焦点，不能让所有内容拥有相同视觉权重；
- 页面应具有现代工程感，但不能做成传统后台仪表盘；
- 颜色、字体和动画必须帮助理解系统；
- 页面必须在桌面和移动端无重叠、无遮挡、无布局跳动；
- 交互模型应增强理解，不能伪装成实时板端数据。

## 3. 非目标

本次页面重设计不包含：

- 实时连接开发板 `/status`；
- 服务端 API、数据库、登录或表单；
- 摄像头直播、验收视频或推流截图；
- React、Vue 等前端框架迁移；
- 修改 daemon、配置协议或测试结论；
- 将主页变成个人简历或营销落地页；
- 使用与系统含义无关的粒子、光球和装饰动画。

## 4. 访客阅读路径

页面使用渐进式信息结构：

```text
项目定位
  ↓
工程目标与关键事实
  ↓
系统架构
  ↓
关键工程决策
  ↓
交互实验室
  ↓
M5 验证结果
  ↓
文档、Release 与源码
```

不同访客可以在任意层级离开，但离开前都能获得一个完整结论：

| 阅读时间 | 获得的信息 |
| --- | --- |
| 15 秒 | 项目定位、平台、技术栈、核心目标 |
| 1 分钟 | 架构、性能和可靠性方向 |
| 3 分钟 | 关键决策、交互模型和验证结果 |
| 深入阅读 | 源码、ADR、测试证据和部署方法 |

## 5. 页面信息架构

### 5.1 顶部导航

只保留一层 sticky 导航，不再同时固定页眉、横切图和章节导航。

```text
Overview · Architecture · Decisions · Lab · Verification · GitHub
```

导航要求：

- 左侧显示项目简称或符号；
- 右侧显示章节入口和 GitHub；
- 当前章节使用底线或文字颜色表示；
- 移动端收敛为菜单按钮；
- 不使用依赖固定高度计算的多层 sticky 偏移。

### 5.2 首屏 Overview

首屏是全宽内容带，不使用左右卡片式营销布局。

建议文案：

```text
i.MX6ULL Sense Terminal

面向资源受限 ARM Linux 平台的
轻量级视觉感知与可靠运行服务。

[查看系统架构]  [GitHub 源码]
```

事实栏采用有标签的工程铭牌：

```text
平台  i.MX6ULL / ARMv7
核心  C11 + pthread
视频  V4L2 + MJPEG
运行  systemd
```

首屏要求：

- H1 必须是项目名称；
- 项目描述控制在两行左右；
- 事实栏是一个整体，不拆成四张功能卡；
- 背景使用抽象数据流线路，不使用摄像头画面；
- 首屏底部必须露出下一节的一部分；
- “不是实时开发板”不作为首屏主文案。

### 5.3 工程目标 Engineering Goals

将项目价值归纳为三个工程目标，而不是罗列普通功能：

#### 实时性

保持约 30 FPS MJPEG 推流，慢客户端允许跳帧，但不能阻塞采集线程或持续累积延迟。

#### 资源意识

在 i.MX6ULL 上控制 CPU 和内存开销；运动检测使用低频 JPEG 灰度解码和缩小后的灰度帧。

#### 可恢复性

摄像头、日志和进程故障都有可诊断状态与恢复路径，服务能够在无人值守条件下运行。

展示规则：

- 三项内容采用横向栏目或无边框分栏；
- 每项只保留一句目标和一句结果；
- 详细实现放到工程决策区域；
- 真实数值必须由验证事实源提供。

### 5.4 系统架构 Architecture

主架构图：

```text
UVC Camera → V4L2 Capture → Latest JPEG → HTTP MJPEG
                                  ↓
                          Grayscale Decode
                                  ↓
                       Motion → JSONL → Status
                                  ↓
                               systemd
```

交互规则：

- 默认状态必须能直接看懂完整链路；
- 鼠标或键盘选中模块后，显示模块职责、输入、输出和源码位置；
- 数据流动画只沿真实路径移动；
- HTTP 客户端和 motion worker 作为 latest JPEG 的两个消费者显示；
- systemd 包围进程生命周期，不画成普通数据处理节点；
- 摄像头缺失时 capture 进入 degraded，但 daemon 和 HTTP 仍保持存在；
- 图示只说明系统，不模拟未经验证的能力。

架构模块颜色：

| 模块 | 颜色语义 |
| --- | --- |
| Camera / Capture | PCB 绿，表示采集和健康 |
| Latest JPEG / HTTP | 数据蓝，表示帧和网络传输 |
| Motion / JSONL | 运动橙，表示分析和事件 |
| Fault | 故障红，表示异常和恢复过程 |
| Inactive / Missing | 中性灰，表示暂不可用 |

### 5.5 工程决策 Decisions

本区是展示工程能力的核心，不按源码文件逐个介绍。

每项统一使用：

```text
问题 → 约束 → 决策 → 取舍 → 结果 → 证据
```

#### 决策一：latest-frame 与慢客户端

- 问题：浏览器读取速度可能低于摄像头采集速度；
- 约束：不能无限排队，也不能让 HTTP 阻塞 capture；
- 决策：共享区域只保留最新 JPEG 和 sequence；
- 取舍：慢客户端可能跳帧，但不会积累越来越大的延迟；
- 结果：采集与客户端发送解耦；
- 证据：链接 M2 验证和相关源码。

#### 决策二：低频运动检测

- 问题：每帧全分辨率 JPEG 解码会浪费 ARM CPU；
- 约束：不能明显降低 MJPEG 主链路帧率；
- 决策：按约 3 FPS 抽样，并缩放为 160 x 120 灰度帧；
- 取舍：不追求逐帧定位，换取稳定资源开销；
- 结果：推流和运动检测并行运行；
- 证据：链接 M3 benchmark 和验证结果。

#### 决策三：摄像头缺失不退出

- 问题：USB 摄像头可能断开或节点编号变化；
- 约束：硬件短暂不可用不应造成整个服务退出；
- 决策：重新枚举符合条件的 UVC Video Capture 节点，并进入 degraded 状态重试；
- 取舍：状态模型比直接退出更复杂，但恢复不依赖 systemd 重启；
- 结果：同一进程完成摄像头热插恢复；
- 证据：链接 M4 camera missing/recovery 验证。

#### 决策四：systemd 故障契约

- 问题：所有退出都自动重启会产生配置错误重启风暴；
- 约束：崩溃应恢复，正常关闭和永久配置错误不应循环重启；
- 决策：SIGTERM 正常退出 0，配置错误使用专用退出码，unit 使用 `Restart=on-failure`；
- 取舍：需要明确进程退出语义和 unit 策略；
- 结果：不同故障拥有可预测行为；
- 证据：链接 M4 fault matrix。

版式规则：

- 每个决策是一个完整横向内容带，不使用卡片套卡片；
- 左侧是问题和约束，右侧是决策、结果与证据；
- 移动端改为纵向顺序；
- 证据链接使用统一图标和文字格式。

### 5.6 交互实验室 Lab

保留现有三个模型，但合并为一个实验区域：

```text
帧流与慢客户端 | 运动检测 | 故障与恢复
```

统一标识：

```text
Interactive model · Based on board-verified M5 evidence
```

#### 帧流与慢客户端

- 展示 frame sequence 如何更新；
- 调低客户端速度时显示被跳过的旧帧；
- 强调“跳帧但不积压”的结果；
- 节点选择说明 UVC capture 与 metadata 节点的区别。

#### 运动检测

- 保留挥手、静止、像素差、比例阈值、cooldown 和 sample FPS；
- 同时显示输入图、灰度图、changed ratio 和状态机；
- motion 触发时使用橙色，不使用通用红色；
- JSONL 预览只展示最近事件。

#### 故障与恢复

- 使用 M5 已验证场景进行回放；
- 展示 process、`/status`、JSONL 和 systemd 状态如何共同变化；
- 故障期间使用红色，degraded 使用橙色，恢复使用绿色；
- 回放结束后显示对应验证文档链接。

### 5.7 M5 验证 Verification

M5 已完成，主页不显示 `Pending`。所有结论必须链接到当前验证文档，不在页面中复制完整日志。

建议矩阵：

| 场景 | 系统行为 | 页面结果 |
| --- | --- | --- |
| 摄像头缺失 | daemon 保持运行并进入 degraded | Verified |
| 摄像头恢复 | 同一进程恢复采集 | Verified |
| SIGTERM | 正常退出，不触发失败重启 | Verified |
| `kill -9` | systemd 按策略恢复服务 | Verified |
| 非法配置 | 明确失败且不产生重启风暴 | Verified |
| 事件日志失败 | 推流继续，状态可诊断并可恢复 | Verified |
| 开发板 reboot | 无需登录即可自动启动 | Verified |
| JSONL 持久化 | 重启后保留并继续追加 | Verified |

展示规则：

- 使用单张清晰表格或时间线，不拆成八张卡片；
- `Verified` 使用绿色文本和检查图标；
- 恢复时间只有在 evidence 中存在真实数据时才显示；
- 点击证据链接进入对应 Markdown 文档；
- 页面不得伪造日志或补齐未记录的数字。

### 5.8 开源入口 Open Source

页面末尾为真正想运行项目的人提供入口：

- Quick Start；
- ARM 交叉编译；
- 板端部署；
- HTTP API；
- 配置参考；
- 架构与 ADR；
- 测试报告；
- Release；
- GitHub 仓库。

主页只展示入口和一行说明，具体步骤继续由 `docs/` 管理。

页尾建议：

```text
Designed and implemented by Eric Hou
GitHub · Architecture · Verification · License
```

作者信息保持克制，不在首屏加入求职或个人简历文案。

## 6. 视觉设计系统

### 6.1 整体风格

风格定义为“现代嵌入式工程作品”：

- 精确，但不冰冷；
- 技术密度较高，但阅读层级明确；
- 有硬件和数据流特征，但不是后台仪表盘；
- 使用多种语义颜色，但不做无意义装饰；
- 通过深浅区域交替建立页面节奏。

页面节奏：

```text
深色首屏
→ 明亮工程目标
→ 明亮架构与决策
→ 深色交互实验室
→ 明亮验证与开源入口
```

### 6.2 配色

| Token | 色值 | 用途 |
| --- | --- | --- |
| Graphite | `#121815` | 首屏、深色实验室、重要标题 |
| Cold White | `#F5F7F6` | 页面基础背景 |
| Surface | `#FFFFFF` | 阅读区域和工具表面 |
| PCB Green | `#197255` | 品牌、采集、健康状态 |
| Data Blue | `#2F6FDB` | HTTP、帧和数据流 |
| Motion Amber | `#D98218` | Motion、阈值和事件 |
| Fault Red | `#C74646` | 故障和不可恢复错误 |
| Muted Ink | `#65716B` | 次要说明 |
| Divider | `#DCE4E0` | 分隔线和静态网格 |

规则：

- 正文不得使用低对比度浅灰；
- 大面积背景只使用 Graphite、Cold White 和 Surface；
- 蓝、橙、红只作为语义强调色；
- 渐变只能用于表现数据流方向，不作为大面积装饰背景；
- 状态不能只依靠颜色区分，必须同时有文字或图标。

### 6.3 字体

页面最多使用三种字体角色：

```css
--font-sans: Inter, "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
--font-mono: "JetBrains Mono", "Cascadia Code", "SFMono-Regular", monospace;
--font-display: Inter, "Segoe UI", "PingFang SC", sans-serif;
```

静态页面默认不依赖网络字体。若仓库不提交本地 `woff2`，浏览器直接使用系统回退字体。

字号建议：

| 场景 | 桌面 | 移动端 |
| --- | --- | --- |
| Hero H1 | 56-68 px | 38-46 px |
| Section H2 | 34-42 px | 28-32 px |
| Subheading | 20-24 px | 18-20 px |
| Body | 16-18 px | 16 px |
| Caption | 13-14 px | 13-14 px |
| Code / status | 12-14 px | 12-13 px |

字体大小不随 viewport 连续缩放，使用固定断点和有限字号。

### 6.4 布局

- 内容最大宽度建议 1180-1240 px；
- 正文阅读列宽建议 680-760 px；
- 使用 12 列桌面网格和 4 列移动端网格；
- 页面区段使用全宽背景带，内容在内部约束宽度；
- 卡片圆角不超过 8 px；
- 不使用卡片嵌套卡片；
- 重复项目才使用卡片，架构和决策使用无框布局；
- 控件、Canvas 和状态栏使用稳定尺寸，交互状态不能推动页面跳动。

### 6.5 图标与视觉元素

- 命令按钮使用清晰文字或图标加文字；
- GitHub、源码、文档和证据链接使用统一图标；
- 架构模块使用简单几何符号，不使用写实照片；
- 不用装饰性圆球、发光斑点或漂浮渐变；
- 当前抽象板卡图标不再承担首屏主视觉，可保留为 favicon 或导航标记。

## 7. 动画与交互规范

### 7.1 允许的动画

1. 帧沿架构路径移动；
2. latest-frame sequence 更新；
3. 慢客户端导致旧帧淡出；
4. changed ratio 跨越阈值；
5. 故障模块从正常变为故障，再恢复；
6. 页面区段进入 viewport 时进行轻微透明度过渡。

### 7.2 禁止的动画

- 无限粒子背景；
- 与内容无关的漂浮、旋转和脉冲；
- 每个标题分别飞入；
- 鼠标移动触发大面积视差；
- 动画改变固定控件尺寸；
- 自动播放导致用户无法阅读静态内容。

### 7.3 性能约束

- 使用 `requestAnimationFrame`；
- Canvas 离开视口后暂停；
- 页面隐藏时暂停循环；
- 优先动画 `transform` 和 `opacity`；
- 避免在每帧进行 DOM 大量重排；
- `prefers-reduced-motion` 下关闭非必要运动；
- 所有模型必须有暂停或重置能力。

## 8. GitHub Pages 静态约束

页面继续使用原生 HTML、CSS、JavaScript 和 Canvas。

### 8.1 允许使用

- ES Modules；
- CSS Custom Properties；
- Canvas 2D；
- `IntersectionObserver`；
- `requestAnimationFrame`；
- 本地 SVG、字体和数据模块；
- Node 内置测试运行纯逻辑测试。

### 8.2 不允许依赖

- 服务端渲染；
- 数据库；
- 隐藏密钥；
- GitHub Pages 运行时环境变量；
- 实时开发板 API；
- 外部 CDN 和第三方脚本；
- 必须联网下载才能工作的字体或图标。

### 8.3 路径规则

GitHub Pages 位于项目子路径：

```text
/imx6ull-sense-terminal/
```

所有资源继续使用相对路径：

```html
./css/site.css
./js/main.js
./assets/...
```

不得使用指向域名根目录的 `/css/...` 或 `/js/...`。

### 8.4 CSP

继续保持严格 Content Security Policy：

- 脚本、样式、图片和字体只允许 `'self'`；
- 默认不连接外部服务；
- 验证事实通过本地 ES Module 导入；
- 若继续使用 `connect-src 'none'`，不得使用 `fetch()` 加载本地 JSON；
- 外部 GitHub 链接使用普通导航，不需要放宽 CSP。

## 9. 数据与事实管理

公开页面不是事实源。所有数字和结论必须能追溯到项目文档。

建议继续使用 `site/js/evidence-facts.js` 保存页面需要的最小事实：

```js
export const evidence = {
  release: "current verified release",
  captureFps: "from verification evidence",
  motionSampleFps: "from verification evidence",
  rssKb: "from verification evidence",
  checks: {
    cameraRecovery: true,
    sigterm: true,
    kill9Recovery: true,
    badConfig: true,
    eventLogRecovery: true,
    rebootAutostart: true,
    jsonlPersistence: true
  }
};
```

规则：

- 不在多个 JavaScript 文件重复硬编码同一数字；
- 页面只展示 evidence 已记录的数值；
- 没有保留精确值时只显示 Verified，不推测恢复时间；
- 更新 daemon 行为时同步更新模型测试和事实数据；
- 页面中的事实链接回 Markdown evidence。

## 10. 无障碍与可理解性

- 所有交互控件可通过键盘访问；
- Canvas 必须有同等信息的文字说明；
- 当前标签页使用 `aria-selected`；
- 状态变化使用 `aria-live` 的短文本摘要；
- 按钮必须有可理解名称；
- 链接文本不能全部写“了解更多”；
- 正文对比度至少满足 WCAG AA；
- hover 信息必须也能通过 focus 获得；
- JavaScript 失败时，项目介绍、架构、决策和验证仍可阅读；
- `noscript` 提示只影响实验室，不把整个页面描述为不可用。

## 11. 响应式设计

建议验证视口：

```text
1440 x 900
1280 x 720
1024 x 768
768 x 1024
390 x 844
360 x 800
```

移动端规则：

- 只保留一层 sticky 导航；
- 首屏事实栏改为两列，不挤成单行；
- 架构图允许纵向布局，但保持数据流方向清楚；
- 决策内容从左右布局改为顺序阅读；
- 实验标签页可横向滚动或使用等宽三段控制；
- 故障矩阵在窄屏使用场景列表，不强行压缩三列表格；
- 文本、按钮和 Canvas 不得互相遮挡；
- 不用假定高度的 CSS 变量定位后续 sticky 元素。

## 12. 现有页面迁移策略

### 保留

- 三个交互模型的纯逻辑；
- motion 参数和状态机演示；
- fault machine 的已验证状态变化；
- evidence facts 与单元测试；
- 严格 CSP；
- 原生 ES Module 架构；
- GitHub Pages Actions 发布方式。

### 重构

- `site/index.html` 的章节顺序和语义结构；
- `site/css/site.css` 的颜色、字体、布局和响应式规则；
- 顶部 cross-section 改为架构区域；
- 三个独立 bench 改为统一实验室；
- 重复的 model chip 改为单一模型说明；
- 验证结果更新为完整 M5 状态；
- 页尾增加克制的作者和文档入口。

### 删除或降级

- 三层 sticky 区域；
- 首屏中的防御性说明；
- 多次重复的“未连接开发板”；
- 当前米色、棕色单一主题；
- 过小的 H1 和紧凑正文；
- 在首页展示完整日志；
- 容易被理解为实时状态的措辞。

## 13. 建议文件影响范围

实施时预计修改：

```text
site/index.html
site/css/site.css
site/js/main.js
site/js/cross-section.js
site/js/evidence-facts.js
site/js/bench-frame.js
site/js/bench-motion.js
site/js/bench-fault.js
site/test/*.test.mjs
```

可选新增：

```text
site/js/architecture-flow.js
site/svg/architecture-static.svg
```

不建议为当前规模拆分多个 CSS 文件，也不建议引入打包器或前端框架。

## 14. 后续实施阶段

本文不执行以下阶段，仅定义后续顺序。

### Phase A：低保真骨架

- 重排 HTML 章节；
- 建立单层导航；
- 确定首屏、架构、决策、实验室和验证区尺寸；
- 不加入最终颜色和动画；
- 桌面与移动端先验证无重叠。

### Phase B：视觉系统

- 落地颜色 token；
- 建立字体和字号层级；
- 调整间距、分隔线、按钮和状态；
- 完成深浅页面节奏；
- 检查颜色对比度。

### Phase C：架构与实验交互

- 将 cross-section 迁移为主架构图；
- 将三个 bench 合并为标签页；
- 保留现有模型逻辑；
- 增加动画暂停、重置和 reduced-motion；
- 确保模型和真实证据边界清楚。

### Phase D：事实与证据

- 从当前 evidence 文件核对所有 M5 结果；
- 更新 `evidence-facts.js`；
- 增加验证文档链接；
- 删除所有 Pending 和无来源数字；
- 检查源码、Release 和 License 链接。

### Phase E：验收与发布

- 运行现有 Node 测试；
- 增加必要的 DOM/纯逻辑测试；
- 检查 GitHub Pages 子路径；
- 检查 CSP 控制台错误；
- 在桌面和移动端截图验收；
- 检查 Canvas 非空和动画运行；
- 检查 reduced-motion；
- 合并后验证线上 URL。

## 15. 验收标准

### 内容

- 首屏 15 秒内能理解项目定位；
- H1 是项目名称；
- 事实栏使用平台、核心、视频、运行四个自然标签；
- 工程决策按约束和取舍组织；
- M5 场景全部显示真实 Verified；
- 页面没有无来源数字；
- 页面没有伪实时措辞。

### 视觉

- 页面具有明确的一级、二级和辅助信息层级；
- 语义颜色在全页保持一致；
- 深色和明亮区段形成清晰节奏；
- 没有卡片嵌套和大面积装饰渐变；
- 标题、正文、代码字体角色清楚；
- 360 px 至 1440 px 视口无重叠和截断。

### 交互

- 三个实验可通过鼠标和键盘操作；
- Canvas 有文字替代信息；
- 动画离开视口后暂停；
- reduced-motion 模式可用；
- 模型不会被误认为实时开发板；
- JavaScript 失败时核心项目内容仍然可读。

### 静态部署

- 不需要服务端和数据库；
- 不需要第三方 CDN；
- 所有资源使用相对路径；
- 严格 CSP 下无报错；
- GitHub Pages 项目子路径可正常加载；
- Node 测试全部通过；
- 线上页面与本地静态预览一致。

## 16. 参考方向

本方案只提取展示原则，不复制具体视觉：

- Meshtastic：硬件项目的直接定位、事实指标和交互入口；
- Bun：用真实数字、命令和 benchmark 建立可信度；
- Rust：将复杂技术归纳为少量核心价值；
- Home Assistant：先解释价值，再逐层展开产品与开源入口；
- Tauri：清晰定位、技术优势和 Quick Start 的组合。

参考链接：

- <https://meshtastic.org/>
- <https://bun.com/>
- <https://rust-lang.org/>
- <https://www.home-assistant.io/>
- <https://tauri.app/>

## 17. 最终设计结论

页面最终采用：

```text
深色项目首屏
+ 有标签的事实栏
+ 明亮工程目标和架构
+ 约束驱动的工程决策
+ 深色交互实验室
+ 完整 M5 验证矩阵
+ 开源文档和源码入口
```

核心原则保持不变：

> 每一种颜色、字体、动画和交互，都必须帮助访客更快理解项目、设计决策或验证结果。
