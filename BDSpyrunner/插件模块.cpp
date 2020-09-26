#include "预编译头.h"
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
	Vec3* pp = p->getPos();\
	BYTE st = p->isStand();
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
* DestroyBlock[MAX],
* StartOpenChest[MAX],
* StartOpenBarrel[MAX],
* StopOpenChest[MAX],
* StopOpenBarrel[MAX],
* SetSlot[MAX];
static VA p_spscqueue = 0;
static VA p_level = 0;
static map<string, Player*> onlinePlayers;
static map<Player*, bool> playerSign;
static map<unsigned, bool> fids;
//-----------------------
// 函数定义
//-----------------------
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
	std::string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}
// GBK 转 UTF-8
static string GBKToUTF8(const char* strGBK)
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
				SYMCALL(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
					p_spscqueue, cmd);
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
			SYMCALL(VA, "?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
				&tpk, 100);
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
		case 1:AddArrayMember(ServerCmd, func); break;
		case 2:AddArrayMember(ServerCmdOutput, func); break;
		case 3:AddArrayMember(FormSelected, func); break;
		case 4:AddArrayMember(UseItem, func); break;
		case 5:AddArrayMember(PlacedBlock, func); break;
		case 6:AddArrayMember(DestroyBlock, func); break;
		case 7:AddArrayMember(StartOpenChest, func); break;
		case 8:AddArrayMember(StartOpenBarrel, func); break;
		case 9:AddArrayMember(StopOpenChest, func); break;
		case 10:AddArrayMember(StopOpenBarrel, func); break;
		case 11:AddArrayMember(SetSlot, func); break;
		default: pr(u8"找不到此类监听!"); break;
		}
		//pr(PyObject_CallFunction(StartOpenChest[0], "{i:s}", 64, "x"));

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
	//Py_DebugFlag = 1;Py_UTF8Mode = 1;
	Py_IsolatedFlag = 1;
	PyImport_AppendInittab("mc", &PyInit_mc); //增加一个模块
	Py_Initialize();
	PyEval_InitThreads();
	//目标文件夹路径
	const char* inPath = "./py/*.py";
	//用于查找的句柄
	long long handle;
	struct _finddata64i32_t fileinfo;
	FILE* f;
	//第一次查找
	handle = _findfirst64i32(inPath, &fileinfo);
	do
	{
		Py_NewInterpreter();
		cout << u8"读取Py文件:" << fileinfo.name << endl;
		char fn[32] = "./py/";
		strcat(fn, fileinfo.name);
		f = fopen(fn, "rb");
		PyRun_SimpleFileEx(f, fileinfo.name, 1);
		//PyRun_SimpleString((string("exec(open(\"./py/") + fileinfo.name + string("\").read())")).c_str());
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
	//onlinePlayers[uuid] = p;
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
	CallAll(UseItem, "{s:s,s:[f,f,f],s:i,s:i,s:i,s:s,s:s,s:is:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname,
		"blockname", bn.c_str(),
		"bid", bid,
		"position", bp->x, bp->y, bp->z,
		"isstand",st
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
		CallAll(PlacedBlock, "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
			"playername", pn.c_str(),
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"blockname", bn.c_str(),
			"bid", bid,
			"position", bp->x, bp->y, bp->z,
			"isstand",st
		);
		RET(_this, b, bp, a4, p, _bool)
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
	CallAll(DestroyBlock, "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"blockname", bn,
		"bid", bid,
		"position", bp->x, bp->y, bp->z,
		"isstand",st
	);
	RET(_this, bp)
}
// 控制台输入
THook(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	CallAll(ServerCmd, "{s:s}", "cmd", (*cmd).substr(0, (*cmd).length() - 1));
	RET(_this, cmd)
}
// 玩家开箱准备
THook(bool,"?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	//BlockSource* bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//PyObject_CallFunction(StartOpenChest[0], "i", 64);
	getPlayerInfo(p);
	CallAll(StartOpenChest, "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand",1
	);
	RET(_this, p, bp);
	//return original(_this, p, bp);
}
// 玩家开桶准备
THook(bool,
	"?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	//short bid = b->getBlockLegacy()->getBlockItemID();
	CallAll(StartOpenBarrel, "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand",st
	);
	RET(_this, p, bp);
}
// 玩家关闭箱子
THook(void,
	"?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll(StopOpenChest, "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand",st
	);
	return original(_this, p);
}
// 玩家关闭木桶
THook(void,
	"?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll(StopOpenBarrel, "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand",st
	);
	return original(_this,p);
}
// 玩家放入取出数量
THook(void,
	"?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
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
			CallAll(SetSlot, "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i,s:i,s:i,s:s,s:i,s:i}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"blockname", bn,
				"bid", bid,
				"position", bp->x, bp->y, bp->z,
				"isstand",st,
				"itemid",iid,
				"itemcount",isize,
				"itemname",iname,
				"itemaux",iaux,
				"slot",slot
			);
		}
		else
			original(a1, slot);
	}
	else
		original(a1, slot);
}