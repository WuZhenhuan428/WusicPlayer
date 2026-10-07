# WusicPlayer 排序/分类 DSL 设计

> 状态: 设计稿 v1
> 范围: 媒体库自定义分类 + 歌单列表排序规则
> 关联: 替换现有 `PlaylistViewModel::set_sort_expression()` 的 `%key%` 表达式;
>       统一 `LibraryBrowseModel` 的预设分组与 `SortType`。

## 1. 设计目标与已确认决策

| #   | 决策                     | 说明                                                   |
| --- | ------------------------ | ------------------------------------------------------ |
| 1   | 不做旧格式兼容           | 目前无用户, 引擎全新, 保持架构干净                     |
| 2   | 属性直接引用             | 不需要 `%` 包裹; 属性/关键字大小写无关                 |
| 3   | 显式小节关键字           | `sort:` / `group:` / `bucket:` 用块形式 `sort { ... }` |
| 4   | 结构化而非语义化         | 显式定界符、无隐式规则、无歧义                         |
| 5   | `if / elif / else` 块    | `bucket` 的标准结构                                    |
| 6   | `group` 与 `bucket` 互斥 | 同一 DSL 二选一, 语义清晰                              |
| 7   | 字符串排序用 QCollator   | 语言感知、忽略大小写; 不做自研排序                     |
| 8   | 花括号块定界             | 语法树结构与代码行/缩进完全无关                        |
| 9   | 原语模型                 | AST = 原语树; 块式语法是原语的表面拼写, 可扩展         |

## 2. 语言总览

一个 DSL 由**三个小节**组成, 每个小节以关键字 + `{}` 块呈现, 各出现至多一次:

```
sort   { ... }   多键排序(可全局)
group  { ... }   多级分组(按属性值聚合)     ── 二选一
bucket { ... }   条件分类(按表达式键聚合)   ── 二选一
```

- 小节顺序任意, 块之间只需空白(换行/空格/`;` 均可, 视作空白)。
- `group` 与 `bucket` 互斥; 缺省 `sort` = 保持原顺序; 缺省 `group`/`bucket` = 平铺。
- 分组后: 组间按分组键排序, 组内按 `sort` 排序。

## 3. 语法规范 (BNF)

### 3.1 结构

```
dsl          := section (SEP? section)*
section      := sort_block | group_block | bucket_block
sort_block   := "sort"   "{" sort_item (SEP sort_item)* "}"
group_block  := "group"  "{" group_item (SEP group_item)* "}"
bucket_block := "bucket" "{" branch      (SEP branch)*      "}"
SEP          := ";" | NEWLINE          -- 等价分隔符; 块外为纯空白
```

### 3.2 小节子句

```
sort_item    := property (dir)? (nulls_mode)?
group_item   := property (dir)?
dir          := "asc" | "desc"                  -- 默认 asc
nulls_mode   := "nulls" ("first" | "last")      -- 仅 sort; 默认 nulls last
branch       := "if"   expr "then" key_expr
              | "elif" expr "then" key_expr
              | "else" key_expr
key_expr     := STRING | expr                   -- 分类键: 字面量或表达式
```

### 3.3 表达式

```
expr      := ternary
ternary   := or_expr ("?" expr ":" expr)?
or_expr   := and_expr (("or" | "||") and_expr)*
and_expr  := not_expr (("and" | "&&") not_expr)*
not_expr  := ("not" | "!") not_expr | cmp_expr
cmp_expr  := add_expr (cmp_op add_expr)?
cmp_op    := "==" | "!=" | "<" | "<=" | ">" | ">="
add_expr  := mul_expr (("+" | "-") mul_expr)*
mul_expr  := unary (("*" | "/" | "%") unary)*
unary     := ("-" | "+") unary | postfix
postfix   := primary
primary   := NUMBER | STRING | "true" | "false" | "null"
           | property | func_call | "(" expr ")"
func_call := ident "(" args? ")"
args      := expr ("," expr)*
```

### 3.4 词法

- 标识符: `[A-Za-z_][A-Za-z0-9_]*`, 解析时大小写无关(归一为小写)。
- 数字: 整数(`0`, `-3`, `2026`)。
- 字符串: 双引号 `"..."` 或单引号 `'...'`; 转义 `\" \' \\ \n \t`。
- 注释: `#` 到行尾, 块内块外均可。
- 保留字: `sort group bucket if elif else then asc desc nulls first last and or not true false null`。
- 属性名不得与保留字冲突(注册表构建时检查)。

## 4. 属性注册表

覆盖 `TrackMetaData` 全部字段 + 派生/上下文属性。类型三类: `string` / `int` / `bool`。

| DSL 标识符     | 类型   | 来源                             | 别名                  |
| -------------- | ------ | -------------------------------- | --------------------- |
| `title`        | string | meta.title                       |                       |
| `artist`       | string | meta.artist                      |                       |
| `album`        | string | meta.album                       |                       |
| `album_artist` | string | meta.album_artist                |                       |
| `genre`        | string | meta.genre                       |                       |
| `composer`     | string | meta.composer                    |                       |
| `comment`      | string | meta.comment                     |                       |
| `lyrics`       | string | meta.lyrics                      |                       |
| `encoder`      | string | meta.encoder                     |                       |
| `date`         | string | meta.date(原始串)                |                       |
| `filename`     | string | meta.filename                    |                       |
| `filepath`     | string | meta.filepath(完整路径)          |                       |
| `directory`    | string | filepath 的目录部分              | 别名 `folder`, `path` |
| `extension`    | string | 派生(扩展名, 无点)               |                       |
| `year`         | int    | meta.year                        |                       |
| `track`        | int    | meta.track_number                | 别名 `track_number`   |
| `disc`         | int    | meta.disc_number                 | 别名 `disc_number`    |
| `disc_total`   | int    | meta.disc_total                  |                       |
| `duration`     | int    | meta.duration_s(秒)              | 别名 `length`         |
| `bitrate`      | int    | meta.bitrate                     |                       |
| `start_at`     | int    | meta.start_at                    |                       |
| `index`        | int    | 上下文(在当前列表中的序号, 0 起) |                       |
| `missing`      | bool   | 派生(文件是否存在)               |                       |

> 注册表 = 引擎唯一数据源。新增属性只需在注册表中登记 `{ 标识符, 类型, 取值函数 }`,
> 排序/分组/分类自动获得能力。

## 5. 类型系统与运算

- **类型**: `string` / `int` / `bool`。字面量类型明确, 属性类型由注册表静态给定。
- **静态检查**(解析期): 未知属性、未知原语、类型不匹配、括号不匹配 → 直接报错, 带 `行:列`。
- **运算约束**:
  - 算术 `+ - * / %`: 仅 `int`。
  - 比较 `== != < <= > >=`: 同类型; `int` 可与数字字面量比较。
  - 逻辑 `and or not`: 仅 `bool`。
  - 字符串函数: 见第 6 节原语表。
- **两套字符串序**(职责分离, 关键):
  - **排序器**(`sort`/`group` 的键序): 统一 `QCollator`, 语言感知、忽略大小写。
  - **比较运算符**(表达式内 `< > <= >=`): 码点序 `QString::compare(..., CaseSensitive)`, 确定性、无歧义。
  - `==` / `!=`: 始终严格(大小写敏感)。

## 6. 原语模型与表面语法

引擎核心是**原语树**(AST)。原语注册表把"语言能力"声明为可配置项:

```
原语 := { 名称(大小写无关), 形式(中缀/前缀), 参数签名, 求值函数 }
```

| 原语                                        | 形式   | 签名 → 返回             |
| ------------------------------------------- | ------ | ----------------------- |
| `sort` / `group` / `bucket`                 | 结构块 | 小节                    |
| `asc` / `desc`                              | 修饰   | 方向                    |
| `nulls first` / `nulls last`                | 修饰   | 空值策略                |
| `if` / `elif` / `else` / `then`             | 分支   | 分类键                  |
| `and` / `or` / `not`                        | 中缀   | (bool, bool…) → bool    |
| `== != < <= > >=`                           | 中缀   | (T, T) → bool           |
| `+ - * / %`                                 | 中缀   | (int, int) → int        |
| `contains(s, sub)`                          | 前缀   | (string, string) → bool |
| `starts_with(s, pre)` / `ends_with(s, suf)` | 前缀   | (string, string) → bool |
| `matches(s, regex)`                         | 前缀   | (string, string) → bool |
| `in(v, item…)`                              | 前缀   | (T, T…) → bool          |
| `len(s)`                                    | 前缀   | (string) → int          |
| `upper(s)` / `lower(s)`                     | 前缀   | (string) → string       |
| `cond ? a : b`                              | 三元   | (bool, T, T) → T        |

**扩展性**: 块式语法只是原语的表面拼写; `sort { artist asc }` 解析后即原语树
`(sort (asc artist))`。未来若需要 S 表达式表面语法, 只需新增一个 reader 把
`( … )` 映射到同一棵原语树, 引擎零改动(见 §9)。

## 7. 空值与边界语义

- **排序**: 空值默认 `nulls last`, 可由 `nulls first` 覆盖。
- **表达式**: 任何二元运算遇 `null` → `null`; `bool` 上下文 `null` = `false`;
  仅 `x == null` / `x != null` 用于判空。
- **分组**: `group` 的键为空/缺失 → `"unknown"`(保持现有行为)。
- **分类**: `bucket` 分支按书写顺序求值, 第一个命中者胜出;
  `else` 缺省时未命中 → 固定键 `"未分类"`。

## 8. 错误模型

- **解析错误**: 未知属性 / 未知原语 / 括号不匹配 / 缺 `then`/`else` / 类型不匹配 /
  保留字冲突。错误信息含 `行:列` 与 token 上下文, UI 内联展示。
- **运行错误**(极少): 正则编译失败(`matches`)等, 归为规则级错误。
- **失败策略**: 规则整体无效 → 回退"无规则"(保持原顺序), 不崩溃、不静默错排。

## 9. 实现架构

```
src/core/dsl/
    lexer.h/cpp        # token 化(标识符/数字/字符串/注释/定界符)
    ast.h              # 原语树节点(原语名 + 参数列表)
    parser.h/cpp       # 递归下降 → AST; 含静态类型/名称检查
    registry.h         # 属性注册表 + 原语注册表
    evaluator.h/cpp    # AST → 闭包(排序比较器 / 分组键函数 / 分类键函数)
```

- **数据流**: `表达式字符串 → lexer → parser(→ AST) → registry 校验 → evaluator 编译 → 比较器/键函数`。
- 解析一次、编译一次, 之后对每条记录 O(1) 求值, 不重复解析。
- **集成**:
  - `PlaylistViewModel::set_sort_expression()` → 改为接收 DSL 字符串, 交给引擎。
  - `LibraryBrowseModel` 预设分组(artist/album/genre/folder/year)→ 映射为 DSL 的 `group { ... }`, 与自定义统一。
  - `SortType` 枚举保留为"快速预设/列头点击排序"入口, 与 DSL 共享属性注册表; 自定义规则以 DSL 字符串持久化(JSON)。
- **持久化**: 保存 DSL 源字符串(可读、可迁移), 加载时解析 + 缓存; 解析失败回退并提示。

## 10. 示例

**歌单排序: 艺术家(空值置底) → 年份降序 → 标题**

```
sort {
    artist asc nulls last
    year desc
    title asc
}
```

**库分类: 多级分组 + 组内排序**

```
group {
    genre asc
    album
}
sort {
    disc asc
    track asc
    title asc
}
```

**库分类: 按年代分桶(条件分类)**

```
bucket {
    if year < 1990 then "90前"
    elif year < 2010 then "90-00s"
    else "10后"
}
sort {
    album_artist asc
    album asc
    disc asc
    track asc
}
```

**流派归一化 + 逻辑运算**

```
bucket {
    if in(genre, "摇滚", "金属", "硬摇") then "摇滚"
    elif in(genre, "爵士", "蓝调") then "爵士/蓝调"
    elif matches(title, "live") then "现场"
    else "其他"
}
```

**按十年分组(表达式作分类键)**

```
bucket {
    if year > 0 then year / 10 * 10
    else "未知年代"
}
sort {
    album asc
    track asc
}
```

**三元表达式(作为键表达式的一部分)**

```
bucket {
    if duration > 0 then duration < 180 ? "短" : "长"
    else "未知"
}
```

## 11. 开放问题 / 未来工作

- `date` / `added_at` 是否进入真实日期运算(暂作字符串/上下文属性)。
- 是否提供 GUI 构建器(可视化为下拉 + 条件编辑器, 而非纯手写文本)。
- S 表达式表面语法(仅当有脚本化需求时再实现)。
