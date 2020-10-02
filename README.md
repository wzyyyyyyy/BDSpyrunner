# 前言
## 欢迎来到Pyr文档
	此插件使用了CPython的api来实现py解释
	参考
	http://docs.python.org
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
	相关事件数据和序号参考
	http://game.xiafox.com/jsrdevdoc.htm#reg_t2
	返回值决定是否拦截（True继续，False拦截）
* [x] 1-控制台输入指令
* [ ] 2-控制台指令输出
* [x] 3-玩家选择表单
* [x] 4-玩家使用物品
* [x] 5-玩家放置方块
* [x] 6-玩家破坏方块
* [x] 7-玩家打开箱子
* [x] 8-玩家打开木桶
* [x] 9-玩家关闭箱子
* [x] 10-玩家关闭木桶
* [x] 11-玩家放入取出物品
* [x] 12-玩家切换纬度
* [x] 13-生物死亡
* [ ] 14-生物受伤
* [x] 15-玩家重生
* [x] 16-聊天监听
* [x] 17-玩家输入文本
* [ ] 18-玩家更新命令方块
* [x] 19-玩家输入指令
* [ ] 20-命令方块指令
* [ ] 21-NPC指令
* [x] 22-玩家加载名字
* [x] 23-玩家离开游戏
* [ ] 24-移动监听
* [x] 25-攻击监听
* [ ] 26-爆炸监听
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
## sendForm(字符串,字符串)
	向玩家发送表单
```py
print(mc.sendForm('a6031970-8ed4-319c-aa45-ac844fd70261','{"content": [{"default": 1,"options": ["Option 1","Option 2","Option 3"],"type": "dropdown","text": "如你所见，下拉框"}],"type": "custom_form","title": "这是一个自定义窗体"}'))
```
## getOnLinePlayers()
	获取当前在线玩家列表以及uuid,xuid字符串
```py
print(mc.getOnLinePlayers())
```
# 已知问题
* [ ] 控制台编码问题(目前是mbcs)
* [x] 错误流消失问题(已解决)
* [ ] 多线程堵塞问题(py内)
