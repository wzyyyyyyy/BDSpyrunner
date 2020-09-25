#include "预编译头.h"
#include "bdxcore.h"
#include "BDS内容.hpp"
#define PY_SSIZE_T_CLEAN
#include "include/Python.h"
#include "tick/tick.h"
#include "json/CJsonObject.hpp"
#include <thread>
using namespace std;

PyObject* mode[1];
static VA p_spscqueue = 0;
static VA p_level = 0;
// 执行后端指令
static bool runcmd(std::string cmd) {
	if (p_spscqueue != 0) {
		if (p_level) {
			auto fr = [cmd]() {
			cout <<"执行了"<< cmd << endl;
				SymCall("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
				 bool,VA,string)(p_spscqueue, cmd);
			};
			safeTick(fr);
			return true;
		}
	}
	return false;
}
void call(PyObject* f) {
	PyObject_CallObject(f,0);
}
static void readAllFile(std::string path) {
	std::string pair = path + "\\*.py";
	_WIN32_FIND_DATAA ffd;
	HANDLE dfh = FindFirstFileA(pair.c_str(), &ffd);
	if (INVALID_HANDLE_VALUE != dfh) {
		do
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// 目录，不作处理
			}
			else
			{
				string strFname("\"" + path + "/" + ffd.cFileName+"\"");
				cout <<"读取PY文件：" << strFname<<endl;
				PyRun_SimpleString((string("exec(open(")+strFname+string(").read())")).c_str());
			}
		} while (FindNextFileA(dfh, &ffd) != 0);
		int dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
		{
			//DisplayErrorBox(TEXT("FindFirstFile"));
		}
		FindClose(dfh);
	}
}
void delay(int ms) {
	Sleep(ms);
	printf("setTimeout执行了\n");
	return;
}
static PyObject* setTimeout(PyObject *self, PyObject *args) {
	//(void)self;
	int time = 0;
	PyObject* func = 0;
	if (!_PyArg_ParseTuple_SizeT(args,"Oi",&func,&time))
	{
		Py_INCREF(Py_None);
	}
	else
	{
		thread t(delay,time);
		t.join();
		PyObject_CallObject(func,0);
	}
	return Py_None;
}
static PyObject* runcmd(PyObject *self, PyObject *args) {
	//(void)self;
	PyObject *pArgs = NULL;
	char* cmd = 0;
	bool result = 0;
	if (!PyArg_ParseTuple(args,"s",&cmd))
	{
		Py_INCREF(Py_None);
		pArgs = Py_None;
	}
	else
	{
		result = runcmd(cmd);
		pArgs = Py_BuildValue("i", result);
	}
	return pArgs;
}
static PyObject* setListener(PyObject *self, PyObject *args) {
	//(void)self;
	int m = 0;
	PyObject* func = 0;
	if (!PyArg_ParseTuple(args, "iO", &m, &func))
	{
		Py_INCREF(Py_None);
	}
	else
	{
		mode[m] = func;
		cout << func << endl << mode[0] << endl;
		call(mode[0]);
	}
	return Py_None;
}
static PyMethodDef mcMethods[] = { //方法列表
	{"runcmd", runcmd, METH_VARARGS,""},
	{"setTimeout", setTimeout, METH_VARARGS,""},
	{"setListener", setListener, METH_VARARGS,""},
	{0,0,0,0}
};
static PyModuleDef mcModule = { //模块声明
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
static PyObject* PyInit_mc(void) {//模块初始化函数
	return PyModule_Create(&mcModule);
}

void child() {
	PyImport_AppendInittab("mc", &PyInit_mc); //增加一个模块
	Py_Initialize();
	readAllFile("py");
	Py_Finalize();
}
//获取指令队列
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
// 玩家破坏方块
THook(bool,"?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	void* _this, void* pBlkpos) {
	//auto pPlayer = *reinterpret_cast<Player**>(reinterpret_cast<VA>(_this) + 8);
	//auto pBlockSource = *(BlockSource**)(*((VA*)_this + 1) + 800);
	//auto pBlk = pBlockSource->getBlock(pBlkpos);
	//neb::CJsonObject jv;
	//addPlayerInfo(jv, pPlayer);
	//jv["position"] = toJson(pBlkpos->getPosition()->toJsonString());
	//jv["blockid"] = pBlk->getLegacyBlock()->getBlockItemID();
	//jv["blockname"] = pBlk->getBlockLegacy()->BlockName;
	call(mode[0]);
	return original(_this, pBlkpos);
}
void init() {
	cout << u8"[插件]pyr加载成功" << endl;
	thread py(child);
	py.detach();
}
void exit() {
}