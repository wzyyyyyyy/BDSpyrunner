#include "预编译头.h"
#include "结构体.hpp"
#include "tick/tick.h"
#include <thread>
using namespace std;
#pragma warning(disable:4996)
#pragma region 宏定义
//调用所有函数
#define CallAll(type,...) \
	PyObject* ret = 0;\
	int res = -1;\
	for(int i = 0;i<10;i++){\
		if(PyCallable_Check(funcs[type][i])){\
		ret = PyObject_CallFunction(funcs[type][i],__VA_ARGS__);\
		}\
		else break;\
	}\
	if (ret)\
		PyArg_Parse(ret, "p", &res);\
	PyErr_Print()
//标准流输出信息
#define pr(...) cout <<__VA_ARGS__<<endl
//THook返回判断
#define RET(...) \
	if (res) return original(__VA_ARGS__);\
	else return 0
//根据Player*获取玩家基本信息
#define getPlayerInfo(p) \
	string pn = p->getNameTag();\
	int did = p->getDimensionId();\
	Vec3* pp = p->getPos();\
	BYTE st = p->isStand();
// 添加函数成员
#define AddFunction(n, func) \
	if(PyCallable_Check(func))\
	for (int i = 0; i < 10; i++) {\
		if (!funcs[n][i]) {\
			funcs[n][i] = func;\
			break;\
		}\
	}
#pragma endregion
#pragma region 全局变量
const short MAX = 10;
static VA p_spscqueue = 0;
static VA p_level = 0;
static VA STD_COUT_HANDLE = *(VA*)SYM("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A");
static unordered_map<string, PyObject* [MAX]> funcs;//py函数
static unordered_map<string, Player*> onlinePlayers;//在线玩家
static unordered_map<Player*, bool> playerSign;//玩家在线
static map<unsigned, bool> fids;//表单ID
static map<string, string> command;//注册命令
unsigned short runningCommandCount = 0;//正在执行的命令数
#pragma endregion
#pragma region 函数定义
// 判断指针是否为玩家列表中指针
static bool checkIsPlayer(void* p) {
	return playerSign[(Player*)p];
}
// UTF-8 转 GBK
static string UTF8ToGBK(const char* strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}
// GBK 转 UTF-8
static string GBKToUTF8(const char* strGBK)
{
	string strOutUTF8 = "";
	WCHAR* str1;
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK, -1, NULL, 0);
	str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, strGBK, -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char* str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	strOutUTF8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return strOutUTF8;
}
// 多线程延时
static void delay(int time, PyObject* func) {
	Sleep(time);
	PyObject_CallFunction(func, 0);
	return;
}
// 执行后端指令
static bool runcmd(string cmd) {
	if (p_spscqueue != 0) {
		if (p_level) {
			auto fr = [cmd]() {
				SYMCALL(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
					p_spscqueue, cmd);
			};
			runningCommandCount++;
			safeTick(fr);
			return true;
		}
	}
	return false;
}
// 获取一个未被使用的基于时间秒数的id
static unsigned getFormId() {
	unsigned id = (int)time(0) + rand();
	do {
		++id;
	} while (id == 0 || fids[id]);
	fids[id] = true;
	return id;
}
// 放弃一个表单
static bool releaseForm(unsigned fid)
{
	if (fids[fid]) {
		fids.erase(fid);
		return true;
	}
	return false;
}
// 发送一个SimpleForm的表单数据包
static unsigned sendForm(string uuid, string str)
{
	unsigned fid = getFormId();
	// 此处自主创建包
	auto fr = [uuid, fid, str]() {
		Player* p = onlinePlayers[uuid];
		if (playerSign[p]) {
			VA tpk;
			//ModalFormRequestPacket sec;
			SYMCALL(VA, "?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
				&tpk, 100);
			*(VA*)(tpk + 40) = fid;
			*(string*)(tpk + 48) = str;
			p->sendPacket(tpk);
		}
	};
	safeTick(fr);
	return fid;
}
// 命令输出
static void logout(string str) {
	SYMCALL(VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
		STD_COUT_HANDLE, str.c_str(), str.length());
}
// 执行指令
static PyObject* api_runCmd(PyObject* self, PyObject* args) {
	PyObject* pArgs = NULL;
	char* cmd = 0;
	if (!PyArg_ParseTuple(args, "s", &cmd));
	else runcmd(cmd);
	return Py_None;
}
// 标准流输出
static PyObject* api_log(PyObject* self, PyObject* args) {
	char* msg = 0;
	int num = 0;
	if (PyArg_ParseTuple(args, "s", &msg))pr(msg);
	else if (PyArg_ParseTuple(args, "i", &num))pr(num);
	PyErr_Clear();
	return Py_None;
}
// 指令输出
static PyObject* api_logout(PyObject* self, PyObject* args) {
	char* msg;
	if (PyArg_ParseTuple(args, "s", &msg)) {
		logout(msg);
	}
	return Py_None;
}
// 延时
static PyObject* api_setTimeout(PyObject* self, PyObject* args) {
	int time = 0;
	PyObject* func = 0;
	if (!PyArg_ParseTuple(args, "Oi", &func, &time));
	else {
		thread t(delay, time, func);
		t.detach();
	}
	return Py_None;
}
// 设置监听
static PyObject* api_setListener(PyObject* self, PyObject* args) {
	//(void)self;
	int m = 0;
	PyObject* func = 0;
	if (PyArg_ParseTuple(args, "iO", &m, &func))
		switch (m) {
		case 1:AddFunction("ServerCmd", func); break;
		case 2:AddFunction("ServerCmdOutput", func); break;
		case 3:AddFunction("FormSelected", func); break;
		case 4:AddFunction("UseItem", func); break;
		case 5:AddFunction("PlacedBlock", func); break;
		case 6:AddFunction("DestroyBlock", func); break;
		case 7:AddFunction("StartOpenChest", func); break;
		case 8:AddFunction("StartOpenBarrel", func); break;
		case 9:AddFunction("StopOpenChest", func); break;
		case 10:AddFunction("StopOpenBarrel", func); break;
		case 11:AddFunction("SetSlot", func); break;
		case 12:AddFunction("ChangeDimension", func); break;
		case 13:AddFunction("MobDie", func); break;
		case 14:AddFunction("MobHurt", func); break;
		case 15:AddFunction("Respawn", func); break;
		case 16:AddFunction("Chat", func); break;
		case 17:AddFunction("InputText", func); break;
		case 18:AddFunction("CommandBlockUpdate", func); break;
		case 19:AddFunction("InputCommand", func); break;
		case 20:AddFunction("BlockCmd", func); break;
		case 21:AddFunction("NpcCmd", func); break;
		case 22:AddFunction("LoadName", func); break;
		case 23:AddFunction("PlayerLeft", func); break;
		case 24:AddFunction("Move", func); break;
		case 25:AddFunction("Attack", func); break;
		case 26:AddFunction("LevelExplode", func); break;
		default: pr(u8"找不到此类监听!"); break;
		}
	return Py_None;
}
// 增加玩家等级
static PyObject* api_addLevel(PyObject* self, PyObject* args) {
	char* uuid;
	int lv = 0;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			SYMCALL(void, "?addLevels@Player@@UEAAXH@Z", pl, lv);
		}
	}
	return Py_None;
}
// 设置玩家名字
static PyObject* api_setNameTag(PyObject* self, PyObject* args) {
	char* uuid;
	char* name;
	if (PyArg_ParseTuple(args, "ss", &uuid, &name)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			pl->reName(name);
		}
	}
	return Py_None;
}
// 获取玩家权限
static PyObject* api_getPlayerPerm(PyObject* self, PyObject* args) {
	char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			return Py_BuildValue("i", pl->getPermission());
		}
	}
	return Py_None;
}
// 设置玩家权限
static PyObject* api_setPlayerPerm(PyObject* self, PyObject* args) {
	char* uuid;
	int lv;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			pl->setPermissionLevel(lv);
		}
	}
	return Py_None;
}
// 发送表单
static PyObject* api_sendForm(PyObject* self, PyObject* args) {
	char* uuid;
	char* str;
	if (!PyArg_ParseTuple(args, "ss", &uuid, &str));
	else
	{
		return Py_BuildValue("i", sendForm(uuid, str));
	}
	return Py_None;
}
// 获取在线玩家列表
static PyObject* api_getOnLinePlayers(PyObject* self, PyObject* args) {
	PyObject* ret = PyDict_New();
	for (auto& op : playerSign) {
		Player* p = op.first;
		if (op.second) {
			PyDict_SetItemString(ret, p->getNameTag().c_str(), Py_BuildValue("[s,s]", p->getUuid()->toString().c_str(), p->getXuid(p_level).c_str()));
		}
	}
	return Py_BuildValue("O", ret);
}
// 设置指令说明
static PyObject* api_setCommandDescribe(PyObject* self, PyObject* args) {
	char* cmd;
	char* des;
	if (!PyArg_ParseTuple(args, "ss", &cmd, &des));
	else
	{
		command[cmd] = des;
	}
	return Py_None;
}
static PyObject* api_getPos(PyObject* self, PyObject* args) {
	char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			Vec3* pos = SYMCALL(Vec3*, "?getPos@Actor@@UEBAAEBVVec3@@XZ", pl);
			PyObject* ret = PyDict_New();
			PyDict_SetItemString(ret, "x", Py_BuildValue("f", pos->x));
			PyDict_SetItemString(ret, "y", Py_BuildValue("f", pos->y));
			PyDict_SetItemString(ret, "z", Py_BuildValue("f", pos->z));
			return ret;
		}
	}
}
static PyObject* api_getHand(PyObject* self, PyObject* args) {
	char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			    ItemStack* item = SYMCALL(ItemStack * ,"?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ", pl);
				short item_aux = SYMCALL(short, "?getAuxValue@ItemStackBase@@QEBAFXZ",item); 
				short item_id = SYMCALL(short, "?getAuxValue@ItemStackBase@@QEBAFXZ",item);
		        std::string str;
				SYMCALL(__int64, "?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",item, &str); 
				std::string item_cname = str;
			    PyObject* ret = PyDict_New();
				PyDict_SetItemString(ret, "aux", Py_BuildValue("i", (int)item_aux));
				PyDict_SetItemString(ret, "cname", Py_BuildValue("s", item_cname.c_str()));
				PyDict_SetItemString(ret, "id", Py_BuildValue("i", (int)item_id));
				return ret;
		}
	}
}
// 方法列表
static PyMethodDef mcMethods[] = {
	{"getHand", api_getHand, METH_VARARGS,""},
	{"getPos", api_getPos, METH_VARARGS,""},
	{"runcmd", api_runCmd, METH_VARARGS,""},
	{"setPlayerPerm",api_setPlayerPerm, METH_VARARGS,""},
	{"addLevel", api_addLevel, METH_VARARGS,""},
	{"setNameTag", api_setNameTag, METH_VARARGS,""},
	{"getPlayerPerm", api_getPlayerPerm, METH_VARARGS,""},
	{"setListener", api_setListener, METH_VARARGS,""},
	{"log", api_log, METH_VARARGS,""},
	{"logout", api_logout, METH_VARARGS,""},
	{"setTimeout", api_setTimeout, METH_VARARGS,""},
	{"sendForm", api_sendForm, METH_VARARGS,""},
	{"getOnLinePlayers", api_getOnLinePlayers, METH_NOARGS,""},
	{"setCommandDescribe", api_setCommandDescribe, METH_VARARGS,""},
	{NULL,NULL,NULL,NULL}
};
// 模块声明
static PyModuleDef mcModule = {
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
// 模块初始化函数
static PyObject* PyInit_mc() {
	return PyModule_Create(&mcModule);
}
// 插件载入
void init() {
	pr(u8"[插件]Python runner(测试版)加载成功");
	Py_LegacyWindowsStdioFlag = 1;
	PyImport_AppendInittab("mc", &PyInit_mc); //增加一个模块
	Py_Initialize();
	//PyEval_InitThreads();
	_finddata64i32_t fileinfo;//用于查找的句柄
	long long handle = _findfirst64i32("./py/*.py", &fileinfo);
	FILE* f;
	// 未找到插件不需要执行
	if (handle != -1LL)
		do {
			Py_NewInterpreter();
			cout << u8"读取Py文件:" << fileinfo.name << endl;
			char fn[32] = "./py/";
			strcat(fn, fileinfo.name);
			f = fopen(fn, "rb");
			PyRun_SimpleFileExFlags(f, fileinfo.name, 1, 0);
		} while (!_findnext64i32(handle, &fileinfo));
		_findclose(handle);
}
// 插件卸载
void exit() {
	Py_FinalizeEx();
}
#pragma endregion
#pragma region THook列表
// 获取指令队列
THook(VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	p_spscqueue = original(_this);
	return p_spscqueue;
}
// 获取地图初始化信息
THook(VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	VA level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	p_level = level;
	return level;
}
// 服务器后台指令输出
THook(VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		CallAll("ServerCmdOutput", "{s:s}", "msg", str);
		RET(handle, str, size);
	}
	return original(handle, str, size);
}
// 玩家加载名字
THook(VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	string uuid = p->getUuid()->toString();
	onlinePlayers[uuid] = p;
	playerSign[p] = true;
	getPlayerInfo(p);
	CallAll("LoadName", "{s:s,s:[f,f,f],s:i,s:i,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"uuid", uuid.c_str()
	);
	return original(a1, p);
}
// 玩家离开游戏
THook(void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	string uuid = p->getUuid()->toString();
	playerSign[p] = false;
	playerSign.erase(p);
	onlinePlayers[uuid] = NULL;
	onlinePlayers.erase(uuid);
	getPlayerInfo(p);
	CallAll("PlayerLeft", "{s:s,s:[f,f,f],s:i,s:i,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"uuid", uuid.c_str()
	);
	return original(_this, p, v3);
}
// 玩家操作物品
THook(bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	void* _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, void* v5, Block* b) {
	Player* p = *(Player**)((VA)_this + 8);
	getPlayerInfo(p);
	short iid = item->getId();
	short iaux = item->getAuxValue();
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll("UseItem", "{s:s,s:[f,f,f],s:i,s:i,s:i,s:s,s:s,s:is:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, item, bp, a4, v5, b);
}
// 玩家放置方块
THook(bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, struct Actor* p, bool _bool) {
	if (p && checkIsPlayer(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		getPlayerInfo(p);
		CallAll("PlacedBlock", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
			"playername", pn.c_str(),
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z,
			"isstand", st
		);
		RET(_this, b, bp, a4, p, _bool);
	}
	return original(_this, b, bp, a4, p, _bool);
}
// 玩家破坏方块
THook(bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	void* _this, BlockPos* bp) {
	Player* p = *(Player**)((VA)_this + 8);
	BlockSource* bs = *(BlockSource**)(*((VA*)_this + 1) + 800);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	getPlayerInfo(p);
	CallAll("DestroyBlock", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, bp);
}
// 控制台输入
THook(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload\r") {
		if (!Py_FinalizeEx()) {
			funcs.clear();
			pr("pyr stoped");
			init();
			pr("pyr reload successed");
			return 0;
		}
	}
	// 插件指令不触发
	if (runningCommandCount > 0) {
		runningCommandCount--;
		return original(_this, cmd);
	}
	CallAll("ServerCmd", "{s:s}", "cmd", (*cmd).substr(0, (*cmd).length() - 1));
	RET(_this, cmd);
}
// 玩家开箱准备
THook(bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll("StartOpenChest", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
	//return original(_this, p, bp);
}
// 玩家开桶准备
THook(bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll("StartOpenBarrel", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
// 玩家关闭箱子
THook(void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll("StopOpenChest", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
// 玩家关闭木桶
THook(void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll("StopOpenBarrel", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
// 玩家放入取出
THook(void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	LevelContainerModel* a1, VA slot) {
	VA v3 = *((VA*)a1 + 26);				// IDA LevelContainerModel::_getContainer
	BlockSource* bs = *(BlockSource**)(*(VA*)(v3 + 808) + 72);
	BlockPos* bp = (BlockPos*)((char*)a1 + 216);
	Block* b = bs->getBlock(bp);
	short bid = b->getBlockLegacy()->getBlockItemID();
	string bn = b->getBlockLegacy()->getBlockName();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// 非箱子、桶、潜影盒的情况不作处理
		VA v5 = (*(VA(**)(LevelContainerModel*))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* is = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			auto iid = is->getId();
			auto iaux = is->getAuxValue();
			auto isize = is->getStackSize();
			auto iname = string(is->getName());
			auto p = a1->getPlayer();
			getPlayerInfo(p);
			CallAll("SetSlot", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i,s:i,s:i,s:s,s:i,s:i}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"blockname", bn.c_str(),
				"blockid", bid,
				"position", bp->x, bp->y, bp->z,
				"isstand", st,
				"itemid", iid,
				"itemcount", isize,
				"itemname", iname,
				"itemaux", iaux,
				"slot", slot
			);
		}
		else
			original(a1, slot);
	}
	else
		original(a1, slot);
}
// 玩家选择表单
THook(void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle, ModalFormResponsePacket** fp) {
	ModalFormResponsePacket* fmp = *fp;
	Player* p = SYMCALL(Player*, "?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, *(char*)((VA)fmp + 16));
	if (p) {
		unsigned fid = fmp->getFormId();
		pr(fid);
		if (releaseForm(fid)) {
			getPlayerInfo(p);
			string uuid = p->getUuid()->toString();
			string slc = fmp->getSelectStr();
			CallAll("FormSelected", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"isstand", st,
				"uuid", uuid.c_str(),
				"selected", slc.c_str()
			);
			return;
		}
	}
	original(_this, id, handle, fp);
}
// 玩家攻击
THook(bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	string an = a->getNameTag();
	string atn = a->getTypeName();
	getPlayerInfo(p);
	CallAll("Attack", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"actorname", an.c_str(),
		"actortype", atn.c_str()
	);
	RET(p, a);
}
// 玩家切换维度
THook(bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	void* _this, Player* p, void* req) {
	getPlayerInfo(p);
	CallAll("ChangeDimension", "{s:s,s:[f,f,f],s:i,s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st
	);
	RET(_this, p, req);
}
// 生物死亡
THook(void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, void* dmsg) {
	PyObject* args = PyDict_New(); char v72; VA v2[2];
	v2[0] = (VA)_this;
	v2[1] = (VA)dmsg;
	VA v7 = *((VA*)(v2[0] + 816));
	VA* srActid = (VA*)(*(VA(__fastcall**)(VA, char*))(*(VA*)v2[1] + 64))(v2[1], &v72);
	Actor* SrAct = SYMCALL(Actor*, "?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		v7, *srActid, 0);
	string sr_name = "";
	string sr_type = "";
	if (SrAct) {
		sr_name = SrAct->getNameTag();
		int srtype = checkIsPlayer(SrAct) ? 319 : SrAct->getEntityTypeId();
		SYMCALL(string&, "?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&sr_type, srtype);
	}
	if (checkIsPlayer(_this)) {
		getPlayerInfo(((Player*)_this));
		string pt;
		SYMCALL(string&, "?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&pt, 319);
		PyDict_SetItemString(args, "playername", Py_BuildValue("s", pn));
		PyDict_SetItemString(args, "isstand", Py_BuildValue("i", st));
		PyDict_SetItemString(args, "mobname", Py_BuildValue("s", pn.c_str()));
		PyDict_SetItemString(args, "mobtype", Py_BuildValue("s", pt.c_str()));
		PyDict_SetItemString(args, "XYZ", Py_BuildValue("[f,f,f]", pp->x, pp->y, pp->z));
	}
	else {
		Vec3* pp = _this->getPos();
		PyDict_SetItemString(args, "mobname", Py_BuildValue("s", _this->getNameTag().c_str()));
		PyDict_SetItemString(args, "mobtype", Py_BuildValue("s", _this->getEntityTypeName().c_str()));
		PyDict_SetItemString(args, "XYZ", Py_BuildValue("[f,f,f]", pp->x, pp->y, pp->z));
	}
	PyDict_SetItemString(args, "srcname", Py_BuildValue("s", sr_name.c_str()));
	PyDict_SetItemString(args, "srctype", Py_BuildValue("s", sr_type.c_str()));
	PyDict_SetItemString(args, "dmcase", Py_BuildValue("i", *((unsigned*)dmsg + 2)));
	CallAll("MobDie", "O", args);
	original(_this, dmsg);
}
// 玩家重生
THook(void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	getPlayerInfo(p);
	CallAll("Respawn", "{s:s,s:[f,f,f],s:i,s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st
	);
	original(p);
}
// 聊天消息
THook(void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	void* _this, string& sender, string& target, string& msg, string& style) {
	CallAll("Chat", "{s:s,s:s,s:s,s:s}",
		"sender", sender,
		"target", target,
		"msg", msg,
		"style", style
	);
	original(_this, sender, target, msg, style);
}
// 玩家输入文本
THook(void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, TextPacket* tp) {
	Player* p = SYMCALL(Player*, "?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)tp + 16));
	if (p) {
		getPlayerInfo(p);
		CallAll("InputText", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"msg", tp->toString(),
			"isstand", st
		);
	}
	original(_this, id, tp);
}
// 玩家输入指令
THook(void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, CommandRequestPacket* crp) {
	Player* p = SYMCALL(Player*, "?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)crp + 16));
	if (p) {
		getPlayerInfo(p);
		CallAll("InputCommand", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"cmd", crp->toString(),
			"isstand", st
		);
	}
	original(_this, id, crp);
}
// 命令注册
THook(void, "?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	void* _this) {
	for (auto& cmd : command) {
		SYMCALL(void, "?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 64);
	}
	original(_this);
}
#pragma endregion
