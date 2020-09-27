# 前言
## 欢迎来到Pyr文档
	此插件使用了CPython的api来实现py解释
	参考http://docs.python.org
	本插件使用了嵌入式扩展
	在初始化py环境时引入mc模块
	所有接口均在mc模块内
***
# 接口说明
## runcmd(字符串)
	执行一段指令
示例:
```py
mc.runcmd('say hello')
```
## log(字符串)
	标准输出一段字符串(中文输出用,无中文用print)
示例:
```py
mc.log('hello')
```
## setListener(整数,函数名)
	设置监听器,执行函数
	执行的函数需要有一个参数
	此参数用来传数据,以字典形式传人
	可print来查看
	返回值决定是否拦截
	1-控制台输入指令
	2-控制台指令输出（无效）
	3-玩家选择表单（无效）
	4-玩家使用物品
	5-玩家放置方块
	6-玩家破坏方块
	7-玩家打开箱子（无效）
	8-玩家打开木桶（无效）
	9-玩家关闭箱子（无效）
	10-玩家关闭木桶（无效）
	11-玩家放入取出物品（无效）
```py
def func(e):
	print(e)
	return False
mc.setListener(1,func)
```
## setTimeout(函数,整数)
	延时执行一个无参函数...单位毫秒
```py
def a()
	print('我是a')
mc.setTimeout(a,500)
```
# 已知问题
	1.控制台编码问题(目前是mbcs)
	2.错误流消失问题
	3.多线程堵塞问题(py内)
