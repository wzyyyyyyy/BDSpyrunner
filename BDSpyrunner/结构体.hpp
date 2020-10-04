#pragma once
#include "Ԥ����ͷ.h"
using namespace std;
struct BlockLegacy {
	string getBlockName() {
		return *(string*)((__int64)this + 112);
	}
	// ��ȡ����ID��
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
	// ȡ����
	Block* getBlock() {
		return *reinterpret_cast<Block**>(reinterpret_cast<VA>(this) + 16);
	}
	// ȡ����λ��
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
	// ��ȡ����Դ
	VA getBlockSouce() {					// IDA Level::tickEntities
		return *((VA*)this + 9);
	}
};
struct Level {
	// ��ȡά��
	Dimension* getDimension(int did) {
		return SYMCALL(Dimension*, "?getDimension@Level@@QEBAPEAVDimension@@V?$AutomaticID@VDimension@@H@@@Z",
			this, did);
	}
	// ��ȡ�Ʒְ�
	VA getScoreBoard() {				// IDA Level::removeEntityReferences
		return *(VA*)((VA)this + 8280);
	}
};
struct MCUUID {
	// ȡuuid�ַ���
	string toString() {
		string s;
		SYMCALL(string&, "?asString@UUID@mce@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ", this, &s);
		return s;
	}
};
struct Vec3 {
	float x, y, z;
};
struct MobEffectInstance {
	char fill[0x1C];
};
struct Item{
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
	// ȡ��ƷID
	short getId() {
		return SYMCALL(short, "?getId@ItemStackBase@@QEBAFXZ",
			this);
	}
	// ȡ��Ʒ����ֵ
	short getAuxValue() {
		return SYMCALL(short, "?getAuxValue@ItemStackBase@@QEBAFXZ",
			this);
	}
	// ȡ��Ʒ����
	std::string getName() {
		std::string str;
		SYMCALL(__int64, "?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &str);
		return str;
	}
	// ȡ����������
	int getStackSize() {			// IDA ContainerModel::networkUpdateItem
		return *((char*)this + 34);
	}
	// �ж��Ƿ������
	/*bool isNull() {
		return SYMCALL(bool,
			MSSYM_B1QA6isNullB1AE13ItemStackBaseB2AAA4QEBAB1UA3NXZ,
			this);
	}*/
};
struct Actor {
	// ��ȡ����������Ϣ
	string getNameTag() {
		return SYMCALL(string&, "?getNameTag@Actor@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this);
	}
	// ��ȡ���ﵱǰ����ά��ID
	int getDimensionId() {
		int dimensionId;
		SYMCALL(int&, "?getDimensionId@Actor@@UEBA?AV?$AutomaticID@VDimension@@H@@XZ",
			this, &dimensionId);
		return dimensionId;
	}
	// ��ȡ���ﵱǰ��������
	Vec3* getPos() {
		return SYMCALL(Vec3*, "?getPos@Actor@@UEBAAEBVVec3@@XZ",
			this);
	}
	// �Ƿ�����
	const BYTE isStand() {				// IDA MovePlayerPacket::MovePlayerPacket
		return *reinterpret_cast<BYTE*>(reinterpret_cast<VA>(this) + 416);
	}
	// ȡ����Դ
	BlockSource* getRegion() {
		return *reinterpret_cast<BlockSource**>(reinterpret_cast<VA>(this) + 414 * sizeof(void*));
	}
	// ��ȡ��������
	std::string getTypeName() {
		std::string actor_typename;
		SYMCALL(std::string&,
			"?getEntityName@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVActor@@@Z",
			&actor_typename, this);
		return actor_typename;
	}

	// ��ȡʵ������
	int getEntityTypeId() {
		return SYMCALL(int,
			"?getEntityTypeId@Actor@@UEBA?AW4ActorType@@XZ",
			this);
		//		if (t == 1)		// δ֪���ͣ����������
		//			return 319;
	}

	// ��ȡ��ѯ��ID
	VA* getUniqueID() {
		return SYMCALL(VA*, "?getUniqueID@Actor@@QEBAAEBUActorUniqueID@@XZ", this);
	}

	// ��ȡʵ������
	std::string getEntityTypeName() {
		std::string en_name;
		SYMCALL(std::string&,
			"?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&en_name, getEntityTypeId());
		return en_name;
	}

	// ��������
	VA updateAttrs() {
		return SYMCALL(VA, "?_sendDirtyActorData@Actor@@QEAAXXZ", this);
	}
	// ���һ��״̬
	VA addEffect(VA ef) {
		return SYMCALL(VA, "?addEffect@Actor@@QEAAXAEBVMobEffectInstance@@@Z", this, ef);
	}
};
struct Mob : Actor {
	// ��ȡ״̬�б�
	std::vector<MobEffectInstance>* getEffects() {					// IDA Mob::addAdditionalSaveData
		return (std::vector<MobEffectInstance>*)((VA*)this + 152);
	}

	// ��ȡװ������
	VA getArmor() {					// IDA Mob::addAdditionalSaveData
		return VA(this) + 1400;
	}
	// ��ȡ��ͷ����
	VA getHands() {
		return VA(this) + 1408;		// IDA Mob::readAdditionalSaveData
	}
	// ���浱ǰ����������
	VA saveOffhand(VA hlist) {
		return SYMCALL(VA, "?saveOffhand@Mob@@IEBA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, hlist);
	}
	// ��ȡ��ͼ��Ϣ
	VA getLevel() {					// IDA Mob::die
		return *((VA*)((VA)this + 816));
	}
};
struct Player : Mob {
	// ȡuuid
	MCUUID* getUuid() {	// IDA ServerNetworkHandler::_createNewPlayer
		return (MCUUID*)((char*)this + 2720);
	}

	// ���ݵ�ͼ��Ϣ��ȡ���xuid
	std::string& getXuid(VA level) {
		return SYMCALL(std::string&, "?getPlayerXUID@Level@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVUUID@mce@@@Z",
			level, (char*)this + 2720);
	}
	// ��������������
	void reName(std::string name) {
		SYMCALL(void, "?setName@Player@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			this, name);
	}
	// ��ȡ�����ʶ��
	VA getNetId() {
		return (VA)this + 2432;		// IDA ServerPlayer::setPermissions
	}

	VA getContainerManager() {
		return (VA)this + 2912;		// IDA Player::setContainerManager
	}
	// ��ȡ����
	VA getSupplies() {				// IDA Player::add
		return *(VA*)(*((VA*)this + 366) + 176);
	}
	// ��ȡĩӰ��
	VA getEnderChestContainer() {
		return SYMCALL(VA, "?getEnderChestContainer@Player@@QEAAPEAVEnderChestContainer@@XZ", this);
	}
	// ����һ��װ��
	VA setArmor(int i, VA item) {
		return SYMCALL(VA, "?setArmor@ServerPlayer@@UEAAXW4ArmorSlot@@AEBVItemStack@@@Z", this, i, item);
	}
	// ���ø���
	VA setOffhandSlot(VA item) {
		return SYMCALL(VA, "?setOffhandSlot@Player@@UEAAXAEBVItemStack@@@Z", this, item);
	}
	// ���һ����Ʒ
	void addItem(VA item) {
		SYMCALL(VA, "?add@Player@@UEAA_NAEAVItemStack@@@Z", this, item);
	}
	// ��ȡ��ǰѡ�еĿ�λ��
	int getSelectdItemSlot() {			// IDA Player::getSelectedItem
		VA v1 = *((VA*)this + 366);
		return *(unsigned int*)(v1 + 16);
	}
	// ��ȡ��ǰ��Ʒ
	ItemStack* getSelectedItem() {
		return SYMCALL(ItemStack*, "?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ", this);
	}
	// ��ȡ��Ϸʱ����Ȩ��
	char getPermission() {						// IDA ServerPlayer::setPermissions
		return **(char**)((VA)this + 2112);
	}
	// ������Ϸʱ����Ȩ��
	void setPermission(char m) {
		SYMCALL(void, "?setPermissions@ServerPlayer@@UEAAXW4CommandPermissionLevel@@@Z",
			this, m);
	}
	// ��ȡ��Ϸʱ����Ȩ��
	char getPermissionLevel() {						// IDA Abilities::setPlayerPermissions
		return (*(char**)((VA)this + 2112))[1];
	}
	// ������Ϸʱ����Ȩ��
	void setPermissionLevel(char m) {
		SYMCALL(void, "?setPlayerPermissions@Abilities@@QEAAXW4PlayerPermissionLevel@@@Z",
			(VA)this + 2112, m);
	}
	// ����������Ʒ�б�
	void updateInventory() {
		VA itm = (VA)this + 4472;				// IDA Player::drop
		SYMCALL(VA, "?forceBalanceTransaction@InventoryTransactionManager@@QEAAXXZ", itm);
	}
	// �������ݰ�
	VA sendPacket(VA pkt) {
		return SYMCALL(VA, "?sendNetworkPacket@ServerPlayer@@UEBAXAEAVPacket@@@Z",
			this, pkt);
	}
};
struct LevelContainerModel {
	// ȡ������
	Player* getPlayer() {
		return ((Player**)this)[26];
	}
};
struct Container {
	VA vtable;
	// ��ȡ������������Ʒ
	VA getSlots(std::vector<ItemStack*>* s) {
		return SYMCALL(VA, "?getSlots@Container@@UEBA?BV?$vector@PEBVItemStack@@V?$allocator@PEBVItemStack@@@std@@@std@@XZ",
			this, s);
	}

};
struct SimpleContainer : Container {
	// ��ȡһ��ָ��������Ʒ
	ItemStack* getItem(int slot) {
		return SYMCALL(ItemStack*, "?getItem@SimpleContainer@@UEBAAEBVItemStack@@H@Z", this, slot);
	}
	// ����һ��ָ�����ڵ���Ʒ
	VA setItem(int slot, ItemStack* item) {
		return SYMCALL(VA, "?setItem@SimpleContainer@@UEAAXHAEBVItemStack@@@Z",
			this, slot, item);
	}
};
struct FillingContainer : Container {
	// ��ʽ������������Ʒ��tag
	VA save(VA tag) {
		return SYMCALL(VA, "?save@FillingContainer@@QEAA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, tag);
	}
	// ����������ָ��λ�õ���Ʒ
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
	// ȡ������
	Player* getPlayer() {				// IDA ContainerManagerModel::ContainerManagerModel
		return *reinterpret_cast<Player**>(reinterpret_cast<VA>(this) + 8);
	}
};
struct LevelContainerManagerModel
	:ContainerManagerModel {
};
struct TextPacket {
	char filler[0xC8];
	// ȡ�����ı�
	std::string toString() {			// IDA ServerNetworkHandler::handle
		std::string str = std::string(*(std::string*)((VA)this + 80));
		return str;
	}
};
struct CommandRequestPacket {
	char filler[0x90];
	// ȡ�����ı�
	std::string toString() {			// IDA ServerNetworkHandler::handle
		std::string str = std::string(*(std::string*)((VA)this + 40));
		return str;
	}
};
struct ModalFormRequestPacket {
	char filler[0x48];
};
struct ModalFormResponsePacket {
	// ȡ�����ID
	UINT getFormId() {
		return *(UINT*)((VA)this + 40);
	}
	// ȡѡ�����
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