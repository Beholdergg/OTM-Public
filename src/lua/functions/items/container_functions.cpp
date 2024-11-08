/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "lua/functions/items/container_functions.hpp"

#include "game/game.hpp"
#include "items/item.hpp"
#include "utils/tools.hpp"

int ContainerFunctions::luaContainerCreate(lua_State* L) {
	// Container(uid)
	const uint32_t id = getNumber<uint32_t>(L, 2);

	const auto &container = getScriptEnv()->getContainerByUID(id);
	if (container) {
		pushUserdata(L, container);
		setMetatable(L, -1, "Container");
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetSize(lua_State* L) {
	// container:getSize()
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		lua_pushnumber(L, container->size());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetMaxCapacity(lua_State* L) {
	// container:getMaxCapacity()
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		lua_pushnumber(L, container->getMaxCapacity());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetCapacity(lua_State* L) {
	// container:getCapacity()
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		lua_pushnumber(L, container->capacity());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetEmptySlots(lua_State* L) {
	// container:getEmptySlots([recursive = false])
	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	uint32_t slots = container->capacity() - container->size();
	const bool recursive = getBoolean(L, 2, false);
	if (recursive) {
		for (ContainerIterator it = container->iterator(); it.hasNext(); it.advance()) {
			if (const auto &tmpContainer = (*it)->getContainer()) {
				slots += tmpContainer->capacity() - tmpContainer->size();
			}
		}
	}
	lua_pushnumber(L, slots);
	return 1;
}

int ContainerFunctions::luaContainerGetItemHoldingCount(lua_State* L) {
	// container:getItemHoldingCount()
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		lua_pushnumber(L, container->getItemHoldingCount());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetItem(lua_State* L) {
	// container:getItem(index)
	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	const uint32_t index = getNumber<uint32_t>(L, 2);
	const auto &item = container->getItemByIndex(index);
	if (item) {
		pushUserdata<Item>(L, item);
		setItemMetatable(L, -1, item);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerHasItem(lua_State* L) {
	// container:hasItem(item)
	const auto &item = getUserdataShared<Item>(L, 2);
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		pushBoolean(L, container->isHoldingItem(item));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerAddItem(lua_State* L) {
	// container:addItem(itemId[, count/subType = 1[, index = INDEX_WHEREEVER[, flags = 0]]])
	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		reportErrorFunc("Container is nullptr");
		return 1;
	}

	uint16_t itemId;
	if (isNumber(L, 2)) {
		itemId = getNumber<uint16_t>(L, 2);
	} else {
		itemId = Item::items.getItemIdByName(getString(L, 2));
		if (itemId == 0) {
			lua_pushnil(L);
			reportErrorFunc("Item id is wrong");
			return 1;
		}
	}

	auto count = getNumber<uint32_t>(L, 3, 1);
	const ItemType &it = Item::items[itemId];
	if (it.stackable) {
		count = std::min<uint16_t>(count, it.stackSize);
	}

	const auto &item = Item::CreateItem(itemId, count);
	if (!item) {
		lua_pushnil(L);
		reportErrorFunc("Item is nullptr");
		return 1;
	}

	const auto index = getNumber<int32_t>(L, 4, INDEX_WHEREEVER);
	const auto flags = getNumber<uint32_t>(L, 5, 0);

	ReturnValue ret = g_game().internalAddItem(container, item, index, flags);
	if (ret == RETURNVALUE_NOERROR) {
		pushUserdata<Item>(L, item);
		setItemMetatable(L, -1, item);
	} else {
		reportErrorFunc(fmt::format("Cannot add item to container, error code: '{}'", getReturnMessage(ret)));
		pushBoolean(L, false);
	}
	return 1;
}

int ContainerFunctions::luaContainerAddItemEx(lua_State* L) {
	// container:addItemEx(item[, index = INDEX_WHEREEVER[, flags = 0]])
	const auto &item = getUserdataShared<Item>(L, 2);
	if (!item) {
		lua_pushnil(L);
		return 1;
	}

	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	if (item->getParent() != VirtualCylinder::virtualCylinder) {
		reportErrorFunc("Item already has a parent");
		lua_pushnil(L);
		return 1;
	}

	const auto index = getNumber<int32_t>(L, 3, INDEX_WHEREEVER);
	const auto flags = getNumber<uint32_t>(L, 4, 0);
	ReturnValue ret = g_game().internalAddItem(container, item, index, flags);
	if (ret == RETURNVALUE_NOERROR) {
		ScriptEnvironment::removeTempItem(item);
	}
	lua_pushnumber(L, ret);
	return 1;
}

int ContainerFunctions::luaContainerGetCorpseOwner(lua_State* L) {
	// container:getCorpseOwner()
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		lua_pushnumber(L, container->getCorpseOwner());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetItemCountById(lua_State* L) {
	// container:getItemCountById(itemId[, subType = -1])
	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	uint16_t itemId;
	if (isNumber(L, 2)) {
		itemId = getNumber<uint16_t>(L, 2);
	} else {
		itemId = Item::items.getItemIdByName(getString(L, 2));
		if (itemId == 0) {
			lua_pushnil(L);
			return 1;
		}
	}

	const auto subType = getNumber<int32_t>(L, 3, -1);
	lua_pushnumber(L, container->getItemTypeCount(itemId, subType));
	return 1;
}

int ContainerFunctions::luaContainerGetContentDescription(lua_State* L) {
	// container:getContentDescription([oldProtocol])
	const auto &container = getUserdataShared<Container>(L, 1);
	if (container) {
		pushString(L, container->getContentDescription(getBoolean(L, 2, false)));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ContainerFunctions::luaContainerGetItems(lua_State* L) {
	// container:getItems([recursive = false])
	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	const bool recursive = getBoolean(L, 2, false);
	const std::vector<std::shared_ptr<Item>> items = container->getItems(recursive);

	lua_createtable(L, static_cast<int>(items.size()), 0);

	int index = 0;
	for (const auto &item : items) {
		index++;
		pushUserdata(L, item);
		setItemMetatable(L, -1, item);
		lua_rawseti(L, -2, index);
	}
	return 1;
}

int ContainerFunctions::luaContainerRegisterReward(lua_State* L) {
	// container:registerReward()
	const auto &container = getUserdataShared<Container>(L, 1);
	if (!container) {
		lua_pushnil(L);
		return 1;
	}

	const int64_t rewardId = getTimeMsNow();
	const auto &rewardContainer = Item::CreateItem(ITEM_REWARD_CONTAINER);
	rewardContainer->setAttribute(ItemAttribute_t::DATE, rewardId);
	container->setAttribute(ItemAttribute_t::DATE, rewardId);
	container->internalAddThing(rewardContainer);
	container->setRewardCorpse();

	pushBoolean(L, true);
	return 1;
}
