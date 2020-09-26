#pragma once
//-----------------------
// 结构体定义
//-----------------------
#include "预编译头.h"
using namespace std;
// 玩家坐标结构体
struct Vec3 {
	float x, y, z;
};
struct BlockLegacy {
	string getBlockName() {
		return *(string*)((__int64)this + 112);
	}
	// 获取方块ID号
	auto getBlockItemID() const {			// IDA VanillaItems::initCreativeItemsCallback Item::beginCreativeGroup "itemGroup.name.planks"
		short v3 = *(short*)((VA)this + 268);
		if (v3 < 0x100) {
			return v3;
		}
		return (short)(255 - v3);
	}

};
struct Block {
	BlockLegacy* getBlockLegacy() {
		return SYMCALL(BlockLegacy*, "?getLegacyBlock@Block@@QEBAAEBVBlockLegacy@@XZ",
			this);
	}
};
struct BlockPos {
	int x, y, z;
};
struct BlockActor {
	// 取方块
	Block* getBlock() {
		return *reinterpret_cast<Block**>(reinterpret_cast<VA>(this) + 16);
	}
	// 取方块位置
	BlockPos* getPosition() {				// IDA BlockActor::BlockActor
		return reinterpret_cast<BlockPos*>(reinterpret_cast<VA>(this) + 44);
	}
};
struct BlockSource {
	Block* getBlock(BlockPos* bp) {
		return SYMCALL(Block*, "?getBlock@BlockSource@@QEBAAEBVBlock@@AEBVBlockPos@@@Z",
			this, bp);
	}
};
struct Dimension {
	// 获取方块源
	VA getBlockSouce() {					// IDA Level::tickEntities
		return *((VA*)this + 9);
	}
};
struct Level {
	// 获取维度
	Dimension* getDimension(int did) {
		return SYMCALL(Dimension*, "?getDimension@Level@@QEBAPEAVDimension@@V?$AutomaticID@VDimension@@H@@@Z",
			this, did);
	}
	// 获取计分板
	VA getScoreBoard() {				// IDA Level::removeEntityReferences
		return *(VA*)((VA)this + 8280);
	}
};
struct Actor {
	// 获取生物名称信息
	string getNameTag() {
		return SYMCALL(string&, "?getNameTag@Actor@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this);
	}
	// 获取生物当前所处维度ID
	int getDimensionId() {
		int dimensionId;
		SYMCALL(int&, "?getDimensionId@Actor@@UEBA?AV?$AutomaticID@VDimension@@H@@XZ",
			this, &dimensionId);
		return dimensionId;
	}
	// 获取生物当前所在坐标
	Vec3* getPos() {
		return SYMCALL(Vec3*, "?getPos@Actor@@UEBAAEBVVec3@@XZ",
			this);
	}
	// 是否悬空
	const BYTE isStand() {				// IDA MovePlayerPacket::MovePlayerPacket
		return *reinterpret_cast<BYTE*>(reinterpret_cast<VA>(this) + 416);
	}

};
struct Mob : Actor {
	// 获取状态列表
	//std::vector<MobEffectInstance>* getEffects() {					// IDA Mob::addAdditionalSaveData
	//	return (std::vector<MobEffectInstance>*)((VA*)this + 152);
	//}

	// 获取装备容器
	VA getArmor() {					// IDA Mob::addAdditionalSaveData
		return VA(this) + 1400;
	}
	// 获取手头容器
	VA getHands() {
		return VA(this) + 1408;		// IDA Mob::readAdditionalSaveData
	}
	// 保存当前副手至容器
	VA saveOffhand(VA hlist) {
		return SYMCALL(VA, "?saveOffhand@Mob@@IEBA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, hlist);
	}
	// 获取地图信息
	VA getLevel() {					// IDA Mob::die
		return *((VA*)((VA)this + 816));
	}
};
struct Player : Mob {
	// 发送数据包
	VA sendPacket(VA pkt) {
		return SYMCALL(VA, "?sendNetworkPacket@ServerPlayer@@UEBAXAEAVPacket@@@Z",
			this, pkt);
	}
};
struct ItemStackBase {
	VA vtable;
	VA mItem;
	VA mUserData;
	VA mBlock;
	short mAuxValue;
	char mCount;
	char mValid;
	char unk[4]{ 0 };
	VA mPickupTime;
	char mShowPickUp;
	char unk2[7]{ 0 };
	std::vector<VA*> mCanPlaceOn;
	VA mCanPlaceOnHash;
	std::vector<VA*> mCanDestroy;
	VA mCanDestroyHash;
	VA mBlockingTick;
	ItemStackBase* mChargedItem;
	VA uk;
public:
	/*VA save() {
		VA* cp = new VA[8]{ 0 };
		return SYMCALL(VA, MSSYM_MD5_e02d5851c93a43bfe24a4396ecb87cde, this, cp);
	}*/
#if (COMMERCIAL)
	Json::Value toJson() {
		VA t = save();
		Json::Value jv = (*(Tag**)t)->toJson();
		(*(Tag**)t)->clearAll();
		*(VA*)t = 0;
		delete (VA*)t;
		return jv;
	}
	void fromJson(Json::Value& jv) {
		VA t = Tag::fromJson(jv);
		SYMCALL(VA, MSSYM_B1QA7fromTagB1AA9ItemStackB2AAA2SAB1QA3AV1B1AE15AEBVCompoundTagB3AAAA1Z, this, *(VA*)t);
		(*(Tag**)t)->clearAll();
		*(VA*)t = 0;
		delete (VA*)t;
	}
	void fromTag(VA t) {
		SYMCALL(VA, MSSYM_B1QA7fromTagB1AA9ItemStackB2AAA2SAB1QA3AV1B1AE15AEBVCompoundTagB3AAAA1Z, this, t);
	}
#endif
	/*bool getFromId(short id, short aux, char count) {
		memcpy(this, SYM_POINT(void, MSSYM_B1QA5EMPTYB1UA4ITEMB1AA9ItemStackB2AAA32V1B1AA1B), 0x90);
		bool ret = SYMCALL(bool, MSSYM_B2QUA7setItemB1AE13ItemStackBaseB2AAA4IEAAB1UA2NHB1AA1Z, this, id);
		mCount = count;
		mAuxValue = aux;
		mValid = true;
		return ret;
	}*/
};
struct ItemStack : ItemStackBase {
	// 取物品ID
	short getId() {
		return SYMCALL(short, "?getId@ItemStackBase@@QEBAFXZ",
			this);
	}
	// 取物品特殊值
	short getAuxValue() {
		return SYMCALL(short, "?getAuxValue@ItemStackBase@@QEBAFXZ",
			this);
	}
	// 取物品名称
	std::string getName() {
		std::string str;
		SYMCALL(__int64, "?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &str);
		return str;
	}
	// 取容器内数量
	int getStackSize() {			// IDA ContainerModel::networkUpdateItem
		return *((char*)this + 34);
	}
	// 判断是否空容器
	/*bool isNull() {
		return SYMCALL(bool,
			MSSYM_B1QA6isNullB1AE13ItemStackBaseB2AAA4QEBAB1UA3NXZ,
			this);
	}*/
};
struct LevelContainerModel {
	// 取开容者
	Player* getPlayer() {
		return ((Player**)this)[26];
	}
};


struct Container {
	VA vtable;
	// 获取容器内所有物品
	VA getSlots(std::vector<ItemStack*>* s) {
		return SYMCALL(VA, "?getSlots@Container@@UEBA?BV?$vector@PEBVItemStack@@V?$allocator@PEBVItemStack@@@std@@@std@@XZ",
			this, s);
	}

};

struct SimpleContainer : Container {
	// 获取一个指定框内物品
	ItemStack* getItem(int slot) {
		return SYMCALL(ItemStack*, "?getItem@SimpleContainer@@UEBAAEBVItemStack@@H@Z", this, slot);
	}
	// 设置一个指定框内的物品
	VA setItem(int slot, ItemStack* item) {
		return SYMCALL(VA, "?setItem@SimpleContainer@@UEAAXHAEBVItemStack@@@Z",
			this, slot, item);
	}
};

struct FillingContainer : Container {
	// 格式化容器所有物品至tag
	VA save(VA tag) {
		return SYMCALL(VA, "?save@FillingContainer@@QEAA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, tag);
	}
	// 设置容器中指定位置的物品
	VA setItem(int i, VA item) {
		return SYMCALL(VA, "?setItem@FillingContainer@@UEAAXHAEBVItemStack@@@Z",
			this, i, item);
	}
};

struct IContainerManager {
	VA vtable;
};

struct PlayerInventoryProxy {
	VA vtable;
	VA vtable2;
	int mSelected;
	char uk1[4];
	ItemStack mInfiniteItem;
	char mSelectedContainerId;
	char uk2[7];
	VA mInventory;
	std::vector<ItemStack> mComplexItems;
	std::weak_ptr<VA> mHudContainerManager;
};

struct ContainerItemStack
	:ItemStack {

};

struct ContainerManagerModel {
	// 取开容者
	Player* getPlayer() {				// IDA ContainerManagerModel::ContainerManagerModel
		return *reinterpret_cast<Player**>(reinterpret_cast<VA>(this) + 8);
	}
};
struct LevelContainerManagerModel
	:ContainerManagerModel {
};
struct TextPacket {
	char filler[0xC8];
	// 取输入文本
	std::string toString() {			// IDA ServerNetworkHandler::handle
		std::string str = std::string(*(std::string*)((VA)this + 80));
		return str;
	}
};

struct CommandRequestPacket {
	char filler[0x90];
	// 取命令文本
	std::string toString() {			// IDA ServerNetworkHandler::handle
		std::string str = std::string(*(std::string*)((VA)this + 40));
		return str;
	}
};

struct ModalFormRequestPacket {
	char filler[0x48];
};

struct ModalFormResponsePacket {
	// 取发起表单ID
	UINT getFormId() {
		return *(UINT*)((VA)this + 40);
	}
	// 取选择序号
	std::string getSelectStr() {
		std::string x = *(std::string*)((VA)this + 48);
		VA l = x.length();
		if (x.c_str()[l - 1] == '\n') {
			return l > 1 ? x.substr(0, l - 1) :
				x;
		}
		return x;
	}
};
