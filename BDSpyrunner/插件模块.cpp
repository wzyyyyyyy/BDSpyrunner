#include "Ԥ����ͷ.h"
#include "�ṹ��.hpp"
#include "tick/tick.h"
#include <thread>
#pragma warning(disable:4996)
//�������к���
#define CallAll(arr,...) \
	PyObject* ret = 0;\
	int res = -1;\
	for(int i = 0;i<10;i++){\
		if(PyCallable_Check(arr[i])){\
		ret = PyObject_CallFunction(arr[i],__VA_ARGS__);\
		}\
		else break;\
	}\
	if (ret != 0)\
		PyArg_Parse(ret, "p", &res);\
	PyErr_Print();
//��׼�������Ϣ
#define pr(...) cout <<__VA_ARGS__<<endl
//THook�����ж�
#define RET(...) \
	if (res) return original(__VA_ARGS__);\
	else return 0;
//����Player*��ȡ��һ�����Ϣ
#define getPlayerInfo(p) \
	string pn = p->getNameTag();\
	int did = p->getDimensionId();\
	Vec3* pp = p->getPos();\
	BYTE st = p->isStand();
using namespace std;
//-----------------------
// ȫ�ֱ���
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
static unordered_map<string, Player*> onlinePlayers;
static unordered_map<Player*, bool> playerSign;
static map<unsigned, bool> fids;
unsigned short runningCommandCount = 0;	// ����ִ�е�������
/*
-----------------------
 ��������
-----------------------
*/
// ���������¼�
static void clearAllEvents() {
	for (int i = 0; i < MAX; i++) {
		ServerCmd[i] = NULL;
		ServerCmdOutput[i] = NULL;
		FormSelected[i] = NULL;
		UseItem[i] = NULL;
		PlacedBlock[i] = NULL;
		DestroyBlock[i] = NULL;
		StartOpenChest[i] = NULL;
		StartOpenBarrel[i] = NULL;
		StopOpenChest[i] = NULL;
		StopOpenBarrel[i] = NULL;
		SetSlot[i] = NULL;
	}
}
// �ж�ָ���Ƿ�Ϊ����б���ָ��
static bool checkIsPlayer(void* p) {
	return playerSign[(Player*)p];
}
// UTF-8 ת GBK
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
// GBK ת UTF-8
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
// ��������Ա
static void AddArrayMember(PyObject* arr[], PyObject* mem) {
	for (int i = 0; i < 10; i++) {
		if (arr[i] == 0 && PyCallable_Check(mem)) {
			arr[i] = mem;
			break;
		}
	}
};
// ���߳���ʱ
static void delay(int time, PyObject* func) {
	Sleep(time);
	PyObject_CallFunction(func, 0);
	return;
}
// ִ�к��ָ��
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
// ��ȡһ��δ��ʹ�õĻ���ʱ��������id
static unsigned getFormId() {
	unsigned id = (int)time(0) + rand();
	do {
		++id;
	} while (id == 0 || fids[id]);
	fids[id] = true;
	return id;
}
// ����һ��SimpleForm�ı����ݰ�
static unsigned sendForm(string uuid, string str)
{
	unsigned fid = getFormId();
	// �˴�����������
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
// ִ��ָ��
static PyObject* api_runCmd(PyObject* self, PyObject* args) {
	PyObject* pArgs = NULL;
	char* cmd = 0;
	if (!PyArg_ParseTuple(args, "s", &cmd));
	else runcmd(cmd);
	return Py_None;
}
// ���
static PyObject* api_log(PyObject* self, PyObject* args) {
	char* msg = 0;
	if (!PyArg_ParseTuple(args, "s", &msg));
	else pr(msg);
	return Py_None;
}
// ��ʱ
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
// ���ü���
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
		default: pr(u8"�Ҳ����������!"); break;
		}
		//pr(PyObject_CallFunction(StartOpenChest[0], "{i:s}", 64, "x"));

	}
	return Py_None;
}
// ���ͱ�
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
// ��ȡ��������б�
static PyObject* api_getOnLinePlayers(PyObject* self, PyObject* args) {
	PyObject* ret = 0;
	if (!PyArg_ParseTuple(args, ""));
	else
	{
		PyObject* ret = PyDict_New();
		for (auto& op : playerSign) {
			Player* p = op.first;
			if (op.second) {
				PyDict_SetItemString(ret, p->getNameTag().c_str(), Py_BuildValue("[s,s]", p->getUuid()->toString().c_str(), p->getXuid(p_level).c_str()));
			}
		}

		return Py_BuildValue("O", ret);
	}
	return Py_None;
}
// �����б�
static PyMethodDef mcMethods[] = {
	{"runcmd", api_runCmd, METH_VARARGS,""},
	{"setListener", api_setListener, METH_VARARGS,""},
	{"log", api_log, METH_VARARGS,""},
	{"setTimeout", api_delay, METH_VARARGS,""},
	{"sendForm", api_sendForm, METH_VARARGS,""},
	{"getOnLinePlayers", api_getOnLinePlayers, METH_VARARGS,""},
	{0,0,0,0}
};
// ģ������
static PyModuleDef mcModule = {
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
// ģ���ʼ������
static PyObject* PyInit_mc(void) {
	return PyModule_Create(&mcModule);
}
// �������
void init() {
	pr(u8"[���]Python runner(���԰�)���سɹ�");
	Py_LegacyWindowsStdioFlag = 1;
	PyImport_AppendInittab("mc", &PyInit_mc); //����һ��ģ��
	Py_Initialize();
	PyEval_InitThreads();
	//Ŀ���ļ���·��
	const char* inPath = "./py/*.py";
	//���ڲ��ҵľ��
	long long handle;
	struct _finddata64i32_t fileinfo;
	FILE* f;
	//��һ�β���
	handle = _findfirst64i32(inPath, &fileinfo);
	// δ�ҵ��������Ҫִ��
	if (handle != -1LL) {
		do
		{
			Py_NewInterpreter();
			cout << u8"��ȡPy�ļ�:" << fileinfo.name << endl;
			char fn[32] = "./py/";
			strcat(fn, fileinfo.name);
			f = fopen(fn, "rb");
			PyRun_SimpleFileEx(f, fileinfo.name, 1);
			//PyRun_SimpleString((string("exec(open(\"./py/") + fileinfo.name + string("\").read())")).c_str());
		} while (!_findnext(handle, &fileinfo));
	}
	_findclose(handle);
}
// ���ж��
void exit() {
	Py_FinalizeEx();
}
/*
-----------------------
 THook�б�
-----------------------
*/
// ��ȡָ�����
THook(VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	p_spscqueue = original(_this);
	return p_spscqueue;
}
// ��ȡ��ͼ��ʼ����Ϣ
THook(VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	VA level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	p_level = level;
	return level;
}
// ��Ҽ�������
THook(VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	VA hret = original(a1, p);
	onlinePlayers[p->getUuid()->toString()] = p;
	playerSign[p] = true;
	return hret;
}
// ����뿪��Ϸ
THook(void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	playerSign[p] = false;
	playerSign.erase(p);
	//onlinePlayers[uuid] = NULL;
	//onlinePlayers.erase(uuid);
}
// ��Ҳ�����Ʒ
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
		"isstand", st
	);
	RET(_this, item, bp, a4, v5, b)
}
// ��ҷ��÷���
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
			"isstand", st
		);
		RET(_this, b, bp, a4, p, _bool)
	}
	return original(_this, b, bp, a4, p, _bool);
}
// ����ƻ�����
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
		"isstand", st
	);
	RET(_this, bp)
}
// ����̨����
THook(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload\r") {
		if (!Py_FinalizeEx()) {
			clearAllEvents();
			pr("pyr stoped");
			init();
			pr("pyr reload successed");
			return 0;
		}
	}
	// ���ָ�����
	if (runningCommandCount > 0) {
		runningCommandCount--;
		return original(_this, cmd);
	}
	CallAll(ServerCmd, "{s:s}", "cmd", (*cmd).substr(0, (*cmd).length() - 1));
	RET(_this, cmd)
}
// ��ҿ���׼��
THook(bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	//BlockSource* bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//PyObject_CallFunction(StartOpenChest[0], "i", 64);
	getPlayerInfo(p);
	CallAll(StartOpenChest, "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", 1
	);
	RET(_this, p, bp);
	//return original(_this, p, bp);
}
// ��ҿ�Ͱ׼��
THook(bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
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
		"isstand", st
	);
	RET(_this, p, bp);
}
// ��ҹر�����
THook(void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
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
		"isstand", st
	);
	return original(_this, p);
}
// ��ҹر�ľͰ
THook(void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
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
		"isstand", st
	);
	return original(_this, p);
}
// ��ҷ���ȡ������
THook(void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	LevelContainerModel* a1, VA slot) {
	VA v3 = *((VA*)a1 + 26);				// IDA LevelContainerModel::_getContainer
	BlockSource* bs = *(BlockSource**)(*(VA*)(v3 + 808) + 72);
	BlockPos* bp = (BlockPos*)((char*)a1 + 216);
	Block* b = bs->getBlock(bp);
	short bid = b->getBlockLegacy()->getBlockItemID();
	string bn = b->getBlockLegacy()->getBlockName();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// �����ӡ�Ͱ��ǱӰ�е������������
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
				"blockname", bn.c_str(),
				"bid", bid,
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