#include "预编译头.h"
//#include "bdxcore.h"
#include "结构体.hpp"
#include "tick/tick.h"
#include <thread>
#define PY_SSIZE_T_CLEAN
#include "include/Python.h"
#pragma warning(disable:4996)
//调用所有函数
#define CallAll(arr,...) \
	PyObject* ret = 0;\
	int res = -1;\
	for(int i = 0;i<10;i++){\
		if(PyCallable_Check(arr[i]))\
		ret = PyObject_CallFunction(arr[i],__VA_ARGS__);\
		else break;\
	}\
	PyArg_Parse(ret, "p", &res)
//标准流输出信息
#define pr(...) cout <<__VA_ARGS__<<endl
//THook返回判断
#define RET(...) \
	if (res) return original(__VA_ARGS__);\
	else return 0;
//根据Player*获取玩家基本信息
#define getPlayerInfo(p) \
	string pn = p->getNameTag();\
	int did = p->getDimensionId();\
	Vec3* pp = p->getPos()
using namespace std;
//-----------------------
// 全局变量
//-----------------------
const char MAX = 10;
PyObject
* ServerCmd[MAX],
* ServerCmdOutput[MAX],
* FormSelected[MAX],
* UseItem[MAX],
* PlacedBlock[MAX],
* DestroyBlock[MAX];
static VA p_spscqueue = 0;
static VA p_level = 0;
static std::unordered_map<std::string, Player*> onlinePlayers;
static std::unordered_map<Player*, bool> playerSign;
static map<unsigned, bool> fids;
//-----------------------
// 函数定义
//-----------------------
// 判断指针是否为玩家列表中指针
static bool checkIsPlayer(void* p) {
	return playerSign[(Player*)p];
}
// UTF-8 转 GBK
static std::string UTF8ToGBK(const char* strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}

// GBK 转 UTF-8
static std::string GBKToUTF8(const char* strGBK)
{
	std::string strOutUTF8 = "";
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

// 添加数组成员
void AddArrayMember(PyObject* arr[], PyObject* mem) {
	for (int i = 0; i < 10; i++) {
		if (arr[i] == 0 && PyCallable_Check(mem)) {
			arr[i] = mem;
			break;
		}
	}
};
// 执行后端指令
static bool runcmd(string cmd) {
	if (p_spscqueue != 0) {
		if (p_level) {
			auto fr = [cmd]() {
				pr("执行了" + cmd);
				SymCall(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
					VA, string)(p_spscqueue, cmd);
			};
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
// 发送一个SimpleForm的表单数据包
static unsigned sendForm(std::string uuid, std::string str)
{
	unsigned fid = getFormId();
	// 此处自主创建包
	auto fr = [uuid, fid, str]() {
		Player* p = onlinePlayers[uuid];
		if (playerSign[p]) {
			VA tpk;
			//ModalFormRequestPacket sec;
			SymCall(VA, "?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
				VA*, int)(&tpk, 100);
			*(VA*)(tpk + 40) = fid;
			*(std::string*)(tpk + 48) = str;
			p->sendPacket(tpk);
		}
	};
	safeTick(fr);
	return fid;
}
// 执行指令
static PyObject* api_runCmd(PyObject* self, PyObject* args) {
	PyObject* pArgs = NULL;
	char* cmd = 0;
	if (!PyArg_ParseTuple(args, "s", &cmd));
	else runcmd(cmd);
	return Py_None;
}
// 输出
static PyObject* api_log(PyObject* self, PyObject* args) {
	char* msg = 0;
	if (!PyArg_ParseTuple(args, "s", &msg));
	else pr(msg);
	return Py_None;
}
// 多线程延时
static void delay(int time, PyObject* func) {
	Sleep(time);
	PyObject_CallFunction(func, 0);
	return;
}
// 延时
static PyObject* api_delay(PyObject* self, PyObject* args) {
	int time = 0;
	PyObject* func = 0;
	if (!PyArg_ParseTuple(args, "Oi", &func, &time));
	else {
		thread t(delay, time, func);
		t.detach();
	}
	return Py_None;
}
//设置监听
static PyObject* api_setListener(PyObject* self, PyObject* args) {
	//(void)self;
	int m = 0;
	PyObject* func = 0;
	if (!PyArg_ParseTuple(args, "iO", &m, &func));
	else
	{
		switch (m) {
		case 0:AddArrayMember(ServerCmd, func); break;
		case 1:AddArrayMember(ServerCmdOutput, func); break;
		case 2:AddArrayMember(FormSelected, func); break;
		case 3:AddArrayMember(UseItem, func); break;
		case 4:AddArrayMember(PlacedBlock, func); break;
		case 5:AddArrayMember(DestroyBlock, func); break;
		}
		//pr(PyObject_CallFunction(ServerCmd[0], "i", 64, "x"));

	}
	return Py_None;
}
// 方法列表
static PyMethodDef mcMethods[] = {
	{"runcmd", api_runCmd, METH_VARARGS,""},
	{"setListener", api_setListener, METH_VARARGS,""},
	{"log", api_log, METH_VARARGS,""},
	{"setTimeout", api_delay, METH_VARARGS,""},
	{0,0,0,0}
};
// 模块声明
static PyModuleDef mcModule = {
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
// 模块初始化函数
static PyObject* PyInit_mc(void) {
	return PyModule_Create(&mcModule);
}
// 插件载入
void init() {
	pr(u8"[插件]Python runner(测试版)加载成功");
	Py_LegacyWindowsStdioFlag = 1;
	Py_IsolatedFlag = 1;
	PyImport_AppendInittab("mc", &PyInit_mc); //增加一个模块
	Py_Initialize();
	PyEval_InitThreads();
	//目标文件夹路径
	const char* inPath = "./py/*.py";
	//用于查找的句柄
	long long handle;
	struct _finddata64i32_t fileinfo;
	//第一次查找
	handle = _findfirst64i32(inPath, &fileinfo);
	do
	{
		Py_NewInterpreter();
		cout << u8"读取Py文件:" << fileinfo.name << endl;
		PyRun_SimpleString((string("exec(open(\"./py/") + fileinfo.name + string("\").read())")).c_str());
	} while (!_findnext(handle, &fileinfo));
	_findclose(handle);
}
// 插件卸载
void exit() {
	Py_Finalize();
}
//-----------------------
// THook列表
//-----------------------
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
// 玩家加载名字
THook(VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	VA hret = original(a1, p);
	//onlinePlayers[uuid] = pPlayer;
	playerSign[p] = true;
	return hret;
}
// 玩家离开游戏
THook(void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	playerSign[p] = false;
	playerSign.erase(p);
	//onlinePlayers[uuid] = NULL;
	//onlinePlayers.erase(uuid);
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
	CallAll(UseItem, "{s:s,s:[f,f,f],s:i,s:i,s:i,s:s,s:s,s:is:[i,i,i]}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, item, bp, a4, v5, b)
}
// 玩家放置方块
THook(bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, struct Actor* p, bool _bool) {
	if (p && checkIsPlayer(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		getPlayerInfo(p);
		CallAll(PlacedBlock, "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i]}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z
		);
		RET(_this, b, bp, a4, p, _bool)
	}
	return 0;
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
	CallAll(DestroyBlock, "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i]}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"blockname", bn,
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, bp)
}
// 控制台输入
THook(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "py\r") {
		exit();
		init();
		return 0;
	}
	CallAll(ServerCmd, "{s:s}", "cmd", (*cmd).substr(0, (*cmd).length() - 1));
	RET(_this, cmd)
}

