#include <4dm.h>
#include <glm/gtc/random.hpp>
#include "Sounds.h"
#include "JSONData.h"
#include "JSONPacket.h"

using namespace fdm;

enum ActionType : int {
	NOTHING,
	TP_UP,
	GIVE_ORE,
	GIVE_ANYTHING,
	GIVE_GRASS,
	SPAWN_SPIDERS,
	SPAWN_BUTTERFLY,
	EXPLODE,
	ADD_SICKNESS,
	SWAP_DAYNIGHT,
	SELF_DELETE
};

// Helper stuff
inline static void spawnEntity(World* world, std::unique_ptr<Entity>& entity) {
	glm::vec4 entityPos = entity->getPos();
	Chunk* chunk = world->getChunkFromCoords(entityPos.x, entityPos.z, entityPos.w);
	if (chunk) world->addEntityToChunk(entity, chunk);
}
inline static void spawnEntityItem(World* world, const std::string& itemName, const int& count, const  glm::vec4& position) {
	std::unique_ptr<Entity> spawnedEntity = EntityItem::createWithItem(
		Item::create(itemName, count), position, {}
	);
	((EntityItem*)spawnedEntity.get())->combineWithNearby(world);
	spawnEntity(world, spawnedEntity);
}
const std::vector<std::string> entitiesList = {
	"Spider",
	"Butterfly"
};
nlohmann::json getAttributes(const std::string& name) {
	if (name == "Spider") return nlohmann::json{ {"type",rand() % 5} };
	else if (name == "Butterfly") return nlohmann::json{ {"type",rand() % 4} };
	else return nlohmann::json::object();
}
// Init
using ActionFunc = std::function<void(World*, Player*)>;

void giveDeadlyOre(World* world, Player* player) {
	if (world->getType() != World::TYPE_CLIENT)
		spawnEntityItem(world, "Deadly Ore", 10, player->pos);
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "You recieved a gift!", 0x00ff00);
	}
}
void giveAnything(World* world, Player* player) {
	if (world->getType() != World::TYPE_CLIENT) {
		auto it = Item::blueprints.begin();
		std::advance(it, glm::linearRand(0, (int)Item::blueprints.size()-1));
		spawnEntityItem(world, it.key(), 5, player->pos);
	}
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "You recieved something!", 0x00ff00);
	}
}
void giveGrass(World* world, Player* player) {
	if (world->getType() != World::TYPE_CLIENT) {
		spawnEntityItem(world, "Midnight Grass", 1, player->pos);
	}
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "You recieved a treasure!", 0x00ff00);
	}
}
void tpUp(World* world, Player* player) {
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(windSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		player->pos += glm::vec4{ 0,50,0,0 };
		player->touchingGround = false;
		player->currentBlock = glm::floor(player->pos);
		StateGame::instanceObj.addChatMessage(player, "A strong wind lifts you up!", 0xffffff);
	}
}
void doNothing(World* world, Player* player) {
	if (world->getType() != World::TYPE_SERVER)
		StateGame::instanceObj.addChatMessage(player, "Nothing happened...", 0x888888); 
}
void spawnButterflies(World* world, Player* player) {
	if (world->getType() != World::TYPE_CLIENT) {
		glm::vec4 offset = { 0, 1.5, 0, 0 };
		glm::vec4 fixRenderingOffset = player->over * 0.0001f;
		for (int i = 0;i < 5;i++) {
			std::unique_ptr<Entity> entity = Entity::createWithAttributes("Butterfly",
				player->pos + offset + fixRenderingOffset,
				getAttributes("Butterfly"));
			spawnEntity(world, entity);
		}
	}
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(spawnSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "A flutter of butterflies has appeared!", 0x0000ff);
	}
}
void spawnSpiders(World* world, Player* player) {
	if (world->getType() != World::TYPE_CLIENT) {
		std::vector<glm::vec4> offsets = {
			{1,0,0,0},
			{-1,0,0,0},
			{0,0,1,0},
			{0,0,-1,0},
			{0,0,0,1},
			{0,0,0,-1}
		};
		glm::vec4 fixRenderingOffset = player->over * 0.0001f;
		for (int i = 0;i < 5;i++) {
			std::unique_ptr<Entity> entity = Entity::createWithAttributes("Spider",
				player->pos + offsets[i] + fixRenderingOffset,
				getAttributes("Spider"));
			spawnEntity(world, entity);
		}
	}
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(spawnSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "A clutter of spiders has appeared!", 0xff0000);
	}
}
void deleteItself(World* world, Player* player) {
	if (world->getType() != World::TYPE_CLIENT)
		player->getSelectedHotbarSlot().release();
	if (world->getType() == World::TYPE_SERVER) // Sync the inventory if its a server
	{
		auto* server = (WorldServer*)world;
		nlohmann::json invJson = player->saveInventory();
		if (server->entityPlayerIDs.contains(player->EntityPlayerID))
			server->sendMessagePlayer({ Packet::S_INVENTORY_UPDATE, invJson.dump() }, server->entityPlayerIDs.at(player->EntityPlayerID), true);
	}
	else{
		AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "No more fun for you!", 0x888888);
	}
}
void swapDayNight(World* world, Player* player) {
	StateGame::instanceObj.time = StateGame::instanceObj.time + 0.5;
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "Time of day suddenly changed!", 0x0000ff);
	}
}
static float motionSicknessDuration = 0.0f;
m4::Mat5 targetPlane = m4::Mat5::identity();
void addMotionSickness(World* world, Player* player) {
	if (world->getType() != World::TYPE_SERVER) {
		motionSicknessDuration += 20;
		AudioManager::playSound4D(sickSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "You are feeling sick...", 0x008800);
	}
}
void explode(World* world, Player* player)
{
	glm::vec4 explosionPos = player->pos;
	float explosionRadius = 2.0f;
	int radius = glm::ceil(explosionRadius);
	float maxDamage = 160;
	glm::ivec4 centerBlock = glm::ivec4(glm::round(explosionPos));
	if (world->getType() != World::TYPE_CLIENT) {
		// destroy blocks in 4D sphere
		for (int x = -radius; x <= radius; x++)
		{
			for (int y = -radius; y <= radius; y++)
			{
				for (int z = -radius; z <= radius; z++)
				{
					for (int w = -radius; w <= radius; w++)
					{
						float dist = sqrt(x * x + y * y + z * z + w * w);
						if (dist > explosionRadius) continue;

						glm::ivec4 blockPos = centerBlock + glm::ivec4(x, y, z, w);
						uint8_t block = world->getBlock(blockPos);

						if (block != BlockInfo::AIR && block != BlockInfo::BARRIER)
							world->setBlockUpdate(blockPos, BlockInfo::AIR);
					}
				}
			}
		}
	}

	// damage and push nearby entities (only the player lmao)
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			for (int z = -1; z <= 1; ++z)
			{
				for (int w = -1; w <= 1; ++w)
				{
					Chunk* chunk = world->getChunkFromCoords(
						explosionPos.x + 8 * x,
						explosionPos.z + 8 * z,
						explosionPos.w + 8 * w
					);
					if (!chunk) continue;

					std::lock_guard lock{ world->entitiesMutex };
					for (auto& entity : chunk->entities)
					{
						if (!entity || entity->dead) continue;

						glm::vec4 entityPos = entity->getPos();
						if (entity->getName() == "Player")
							entityPos.y += Player::HEIGHT * 0.5f;

						float distance = glm::length(entityPos - explosionPos);
						if (distance > explosionRadius) continue;

						float strength = 1.0f - (distance / explosionRadius);

						glm::vec4 knockbackDir = glm::normalize(entityPos - explosionPos);
						if (distance < 0.01f) // just in case 0 happens
							knockbackDir = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

						glm::vec4 knockbackVel = knockbackDir * 10.0f * strength;

						if (entity->getName() == "Player" && world->getType() != World::TYPE_SERVER)
						{
							EntityPlayer* entityPlayer = reinterpret_cast<EntityPlayer*>(entity);
							if (entityPlayer->player)
								entityPlayer->player->vel += knockbackVel;
						}
						if (world->getType() != World::TYPE_CLIENT)
							entity->takeDamage(maxDamage * strength, world);
					}
				}
			}
		}
	}
	
	if (world->getType() != World::TYPE_SERVER) {
		AudioManager::playSound4D(explosionSound, "ambience", player->cameraPos, glm::vec4{ 0 });
		StateGame::instanceObj.addChatMessage(player, "You have spontaneously exploded!", 0xff0000);
	}
}
std::vector<std::pair<ActionType, int>> absoluteWeights =
{
	{SWAP_DAYNIGHT,4},
	{NOTHING,24},
	{TP_UP,6},
	{ADD_SICKNESS,6},
	{GIVE_ORE,4},
	{GIVE_ANYTHING,8},
	{GIVE_GRASS,8},
	{SPAWN_BUTTERFLY,6},
	{SPAWN_SPIDERS,6},
	{SELF_DELETE,1},
	{EXPLODE,2}
};
std::map<int, ActionType, std::greater<int>> accumulatedWeights = {};
int totalWeight = 0;
bool initialized = false;
std::unordered_map<ActionType, ActionFunc> callbacks = {
	{SWAP_DAYNIGHT,swapDayNight},
	{NOTHING,doNothing},
	{TP_UP,tpUp},
	{ADD_SICKNESS,addMotionSickness},
	{GIVE_ORE,giveDeadlyOre},
	{GIVE_ANYTHING,giveAnything},
	{GIVE_GRASS,giveGrass},
	{SPAWN_BUTTERFLY,spawnButterflies},
	{SPAWN_SPIDERS,spawnSpiders},
	{SELF_DELETE,deleteItself},
	{EXPLODE,explode}
};

void initialize() {
	std::sort(absoluteWeights.begin(), absoluteWeights.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
	for (auto& it : absoluteWeights) {
		accumulatedWeights.insert({ it.second + totalWeight,it.first });
		totalWeight += it.second;
	}
	initialized = true;
}
// Motion sickness stuff
$hook(void,WorldManager, render, const m4::Mat5& MV, bool glasses, glm::vec3 worldColor) {
	m4::Mat5 m = MV;

	if (motionSicknessDuration > 0) {
		
		m4::Mat5 rot{ 1 };
		rot *= m4::Rotor{ { 0, 1, 0, 0, 0, 0 }, 0.3f * glm::sin((float)glfwGetTime()) };
		rot *= m4::Rotor{ { 0, 0, 0, 1, 0, 0 }, 0.1f* glm::sin(2.0f * (float)glfwGetTime()) };
		rot *= m4::Rotor{ { 0, 0, 0, 0, 0, 1 }, 0.2f * glm::sin(1.5f* (float)glfwGetTime()) };

		m = rot * m;
	}

	original(self, m, glasses, worldColor);
}
$hook(void, Player, update, World* world, double dt, EntityPlayer* entityPlayer) {
	original(self, world, dt, entityPlayer);
	
	if (world->getType() != World::TYPE_SERVER && 
		self == &StateGame::instanceObj.player && 
		motionSicknessDuration > 0)
		motionSicknessDuration -= dt;
}

//Applying actions
ActionType getRandomAction() {
	if (!initialized) initialize();
	int t = glm::linearRand(0,totalWeight);

	auto it = accumulatedWeights.upper_bound(totalWeight - t);
	if (it == accumulatedWeights.begin()) return it->second;
	if (it == accumulatedWeights.end()) return std::prev(it)->second;
	return std::prev(it)->second;
}
$hook(bool, ItemMaterial, action, World* world, Player* player, int action) {
	if (self->name != "Funny Button") return original(self, world, player, action);
	if (!action) return false;
	
	AudioManager::playSound4D(buttonPressSound, "ambience", player->cameraPos, glm::vec4{ 0 });

	ActionType type = getRandomAction();
	if (world->getType() == World::TYPE_SERVER && ((WorldServer*)world)->entityPlayerIDs.contains(player->EntityPlayerID)) {
		JSONData::sendPacketClient((WorldServer*)world, JSONPacket::S_FUNNY,
			{ {"type", type} }, ((WorldServer*)world)->entityPlayerIDs[player->EntityPlayerID]->handle);
	}
	callbacks[type](world, player);
	return type != ActionType::SELF_DELETE;
}

// Multiplayer stuff
// Client - side
$hook(void, StateIntro, init, StateManager& s) {
	original(self, s);
	JSONData::SCaddPacketCallback(JSONPacket::S_FUNNY, [](WorldClient* world, Player* player, const nlohmann::json& data)
		{
			callbacks[data["type"]](world, player);
		}
	);
}