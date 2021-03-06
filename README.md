# 约定
为了方便测试，请你做实验时，遵守以下约定。

| 文件/目录 | 说明 |
| --- | --- |
| `README.md` | 项目说明。随便改。 |
| `.gitlab-ci.yml` | 自动测试配置， **不能改动！** |
| `prepare.sh` | 运行测试前你要运行的命令，例如安装额外依赖、选择其他编译手段。可以改，只要保证能被 bash 运行即可。**但请保证编译完之后可执行文件路径是 `build/MiniDecaf`，它接受一个命令行参数作为源文件名，并通过标准输出输出编译结果。** |
| `step-until.txt` | 告诉自动测试，你做到哪个 step 了。可以改，须保证内容是 1 到 12 的一个整数。 **做完每个 step 后请及时修改，避免影响评分！** |
| `reports/` | 实验报告，使用 pdf 或 md 格式，命名格式如 `step1.pdf`、`step2.md` 等。 |
| `CMakeLists.txt` | CMake 脚本，可以改，你也可以改用别的编译手段，只要能被 `prepare.sh` 里面的脚本编译即可。里面默认是一个能链接 ANTLR 的脚本。如果不想用 ANTLR，删掉即可。 |
| `src/` 和 `generate` | 放你的编译器代码。你也可以把代码放到别的目录，只要能被 `prepare.sh` 里面的脚本编译即可。 |
| `third_party/` | 第三方依赖，可以改。如果不想用 ANTLR，删掉即可。 |


# 评分
MiniDecaf 有 6 个阶段，每个阶段的 ddl 截止时，我们会检查你最后一次通过 CI 的 commit。

* 如果 `.gitlab-ci.yml` 没有改动，并且 `step-until.txt` 中的数字大于等于那个阶段的最后一个 step 编号，我们就认为你按时完成了该阶段任务。

* 否则，我们会等待你通过该阶段任务，并且按照指导书所说折算晚交扣分。

# 文件
montLexer 词法分析 montParser 语法分析生成AST montConceiver 生成中间代码 montAssembler 生成汇编代码