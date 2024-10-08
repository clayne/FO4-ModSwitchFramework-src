#pragma once
#include "MSF_Shared.h"
#include "MSF_Data.h"
//#include "MSF_Serialization.h"


typedef UInt32 WeaponStateID;

class DataHolderParentInstance
{
public:
	ExtraDataList* extraList;
	UInt32 formID;
	UInt32 stackID;
	ObjectRefHandle refHandle;
};

class ExtraWeaponState
{
public:
	~ExtraWeaponState();
	ExtraWeaponState(ExtraRank* holder);
	static ExtraWeaponState* Init(ExtraDataList* extraDataList, EquipWeaponData* equipData);
	static bool HandleWeaponStateEvents(UInt8 eventType, Actor* actor = nullptr);
	static TESAmmo* GetAmmoForWorkbenchUI(ExtraDataList* extraList);
	//static auto GetCurrentUniqueState(BGSObjectInstanceExtra* attachedMods);
	static ModData::Mod* ExtraWeaponState::GetCurrentUniqueStateMod(BGSObjectInstanceExtra* attachedMods);
	static ModData::Mod defaultStatePlaceholder;
	//bool SetWeaponState(ExtraDataList* extraDataList, EquipWeaponData* equipData, bool temporary);
	//bool RecoverTemporaryState(ExtraDataList* extraDataList, EquipWeaponData* equipData);
	//bool SetCurrentStateTemporary();
	bool HandleEquipEvent(ExtraDataList* extraDataList, EquipWeaponData* equipData);
	bool HandleFireEvent(ExtraDataList* extraDataList, EquipWeaponData* equipData);
	bool HandleAmmoChangeEvent(ExtraDataList* extraDataList, EquipWeaponData* equipData);
	bool HandleReloadEvent(ExtraDataList* extraDataList, EquipWeaponData* equipData, UInt8 eventType);
	static bool HandleModChangeEvent(ExtraDataList* extraDataList, std::vector<std::pair<BGSMod::Attachment::Mod*, bool>>* modsToModify, UInt8 eventType); //update burst manager
	bool UpdateWeaponStates(ExtraDataList* extraDataList, EquipWeaponData* equipData, UInt8 eventType, std::vector<std::pair<BGSMod::Attachment::Mod*, bool>>* modsToModify = nullptr);
	TESAmmo* GetCurrentAmmo();
	bool SetCurrentAmmo(TESAmmo* ammo);
	void PrintStoredData();

	enum
	{
		kEventTypeUndefined,
		kEventTypeEquip,
		kEventTypeAmmoCount,
		kEventTypeFireWeapon,
		kEventTypeReload,
		kEventTypeModded,
		kEventTypeModdedWorkbench,
		kEventTypeModdedGameplay,
		kEventTypeModdedSwitch,
		kEventTypeModdedAmmo,
		kEventTypeModdedNewChamber
	};

	class WeaponState
	{
	public:
		WeaponState(ExtraDataList* extraDataList, EquipWeaponData* equipData, ModData::Mod* currUniqueStateMod);
		WeaponState(UInt16 flags, UInt16 ammoCapacity, UInt16 chamberSize, UInt16 shotCount, UInt64 loadedAmmo, TESAmmo* chamberedAmmo, std::vector<TESAmmo*>* BCRammo, std::vector<BGSMod::Attachment::Mod*>* newStateMods);
		bool FillData(ExtraDataList* extraDataList, EquipWeaponData* equipData, ModData::Mod* currUniqueStateMod);
		bool UpdateAmmoState(ExtraDataList* extraDataList, BGSObjectInstanceExtra* attachedMods, UInt8 eventType, bool stateChange, std::vector<std::pair<BGSMod::Attachment::Mod*, bool>>* modsToModify);
		WeaponState* Clone();
		enum
		{
			bHasLevel = 0x0001,
			bActive = 0x0002,
			bHasTacticalReload = 0x0010,
			bHasBCR = 0x0020,
			bChamberLIFO = 0x0040, //otherwise FIFO
			mChamberMask = 0x00F0
		};
		UInt16 flags; //state flags
		UInt16 ammoCapacity;
		UInt16 chamberSize; //if -1: equals to ammoCapacity
		volatile short shotCount; 
		volatile int loadedAmmo;
		TESAmmo* chamberedAmmo;
		std::vector<TESAmmo*> BCRammo; //if BCR && TR: size=ammoCap; if !BCR && TR: size=chamber; if BCR && !TR: size=ammoCap
		std::vector<BGSMod::Attachment::Mod*> stateMods;
		//AmmoData::AmmoMod* currentSwitchedAmmo;
		//AmmoData::AmmoMod* switchToAmmoAfterFire;
		//std::vector<ModData::Mod*> attachedMods; //maybe later
	private:
		WeaponState() {};
	};
	friend class StoredExtraWeaponState;
	bool GetUniqueStateModsToModify(BGSObjectInstanceExtra* attachedMods, std::vector<std::pair<BGSMod::Attachment::Mod*, bool>>* modsToModify, std::pair<WeaponState*, WeaponState*>* stateChange);
private:
	ExtraWeaponState(ExtraDataList* extraDataList, EquipWeaponData* equipData);
	WeaponStateID ID;
	ExtraRank* holder; //ExtraDataList
	std::map<BGSMod::Attachment::Mod*, WeaponState*> weaponStates;
	WeaponState* currentState;
	//BurstModeManager* burstModeManager;
};

class WeaponStateStore
{
public:
	WeaponStateStore()
	{
		mapstorage.reserve(100);
		vectorstorage.reserve(100);
		ranksToClear.reserve(100);
	};
	void Free()
	{
		for (auto& state : vectorstorage)
			delete state;
		vectorstorage.clear();
		mapstorage.clear();
		ranksToClear.clear();
	};
	WeaponStateID Add(ExtraWeaponState* state)
	{
		//nocheck
		vectorstorage.push_back(state);
		return vectorstorage.size();
	};
	ExtraWeaponState* Get(WeaponStateID id)
	{
		ExtraWeaponState* state = nullptr;
		if (id && id <= vectorstorage.size())
			state = vectorstorage[id - 1];
		return state;
	};
	ExtraWeaponState* GetValid(WeaponStateID id)
	{
		ExtraWeaponState* state = nullptr;
		if (id && id <= vectorstorage.size())
			state = vectorstorage[id - 1];
		if (state)// && state->ValidateParent())
			return state;
		return nullptr;
	};
	bool StoreForLoad(WeaponStateID id, ExtraRank* holder)
	{
		if (!holder || id == 0)
			return false;
		auto itMap = mapstorage.find(id);
		auto itClear = ranksToClear.find(id);
		if (itMap == mapstorage.end() && itClear == ranksToClear.end())
			mapstorage[id] = holder;
		else
		{
			_MESSAGE("WARNING: duplicate WeaponStateID found");
			ranksToClear.insert(ranksToClear.begin(), std::pair<WeaponStateID, ExtraRank*>(id, holder));
			ranksToClear.insert(ranksToClear.begin(), std::pair<WeaponStateID, ExtraRank*>(id, itMap->second));
			mapstorage.erase(itMap);
			return false;
		}
		//ExtraRank* occupied = mapstorage[id];
		//if (!occupied)
		//	mapstorage[id] = holder;
		return true;
	};
	ExtraRank* GetForLoad(WeaponStateID id)
	{
		if (id == 0)
			return nullptr;
		ExtraRank* holder = mapstorage[id];
		if (holder && holder->rank != id)
		{
			_DEBUG("IDerror");
			holder = nullptr;
		}
		return holder;
	};
	UInt32 GetCount()
	{
		return vectorstorage.size();
	};
	void SaveWeaponStates(std::function<bool(const F4SESerializationInterface*, UInt32, ExtraWeaponState*)> f_callback, const F4SESerializationInterface* intfc, UInt32 version)
	{
		for (const auto& state : vectorstorage)
		{
			if (state)
				f_callback(intfc, version, state);
		}
	};
	void InvalidateID(WeaponStateID id)
	{
		if (id == 0)
			return;
		auto itMap = mapstorage.find(id);
		if (itMap == mapstorage.end())
			return;
		ranksToClear.insert(ranksToClear.begin(), std::pair<WeaponStateID, ExtraRank*>(itMap->first, itMap->second));
		mapstorage.erase(itMap);
	};
	void InvalidateAllIDs()
	{
		ranksToClear.insert(mapstorage.begin(), mapstorage.end());
		mapstorage.clear();
	};
	bool ClearInvalidWeaponStates()
	{
		for (auto holder : ranksToClear)
		{
			_DEBUG("invalid: %08X %p", holder.first, holder.second);
			holder.second->rank = 0;
		}
		ranksToClear.clear();
		return true;
	};
	void PrintStoredWeaponStates()
	{
		_MESSAGE("");
		_MESSAGE("===Printing stored WeaponStates===");
		for (const auto& state : vectorstorage)
		{
			if (state)
				state->PrintStoredData();
		}
		_MESSAGE("===Printing WeaponState IDs in Player inventory===");
		for (UInt32 i = 0; i < (*g_player)->inventoryList->items.count; i++)
		{
			BGSInventoryItem inventoryItem;
			(*g_player)->inventoryList->items.GetNthItem(i, inventoryItem);
			TESObjectWEAP* weap = DYNAMIC_CAST(inventoryItem.form, TESForm, TESObjectWEAP);
			if (!weap || !inventoryItem.stack)
				continue;
			UInt32 stackID = -1;
			for (BGSInventoryItem::Stack* stack = inventoryItem.stack; stack; stack = stack->next)
			{
				stackID++;
				if (stack->extraData)
				{
					ExtraRank* extraRank = (ExtraRank*)stack->extraData->GetByType(kExtraData_Rank);
					if (!extraRank)
						continue;
					UInt32 modNo = 0;
					BGSObjectInstanceExtra* oie = (BGSObjectInstanceExtra*)stack->extraData->GetByType(kExtraData_ObjectInstance);
					if (oie)
						modNo = oie->data->blockSize / sizeof(BGSObjectInstanceExtra::Data::Form);
					uint8_t isEquipped = 0;
					if (stack->flags & BGSInventoryItem::Stack::kFlagEquipped)
						isEquipped = 1;
					_MESSAGE("ID: %08X, item: %08X, stackID: %08X, modNo: %i, isEquipped: %02X", extraRank->rank, weap->formID, stackID, modNo, isEquipped);

				}
			}
		}
		_MESSAGE("");
	};
private:
	std::unordered_map<WeaponStateID, ExtraRank*> mapstorage; //used only for the loading of f4se serialized data, WeaponStateID is invalid afterwards
	std::unordered_multimap<WeaponStateID, ExtraRank*> ranksToClear;
	std::vector<ExtraWeaponState*> vectorstorage; // used for quick access of WeaponState with ExtraRank->rank (WeaponStateID) being the vector index +1
};