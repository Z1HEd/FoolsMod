#include <4dm.h>
#include <glm/gtc/random.hpp>
#include "Sounds.h"

using namespace fdm;

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
using ActionFunc = std::function<bool(World*, Player*)>;

bool giveDeadlyOre(World* world, Player* player) {
	spawnEntityItem(world, "Deadly Ore", 10, player->pos);
	AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player,"You recieved a gift!",0x00ff00);
	return true;
}
bool giveGrass(World* world, Player* player) {
	spawnEntityItem(world, "Midnight Grass", 1, player->pos);
	AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player, "You recieved a gift!", 0x00ff00);
	return true;
}
bool tpUp(World* world, Player* player) {
	AudioManager::playSound4D(windSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	player->pos += glm::vec4{0,50,0,0};
	player->touchingGround = false;
	player->currentBlock = glm::floor(player->pos);
	StateGame::instanceObj.addChatMessage(player, "A strong wind lifts you up!", 0xffffff);
	return true;
}
bool doNothing(World* world, Player* player) { 
	StateGame::instanceObj.addChatMessage(player, "Nothing happened...", 0x888888); 
	return true;
}
bool spawnButterflies(World* world, Player* player) {
	glm::vec4 offset = { 0, 1.5, 0, 0 };
	glm::vec4 fixRenderingOffset = player->over * 0.0001f;
	for (int i = 0;i < 5;i++) {
		std::unique_ptr<Entity> entity = Entity::createWithAttributes("Butterfly", 
			player->pos+ offset + fixRenderingOffset,
			getAttributes("Butterfly"));
		spawnEntity(world, entity);
	}
	AudioManager::playSound4D(spawnSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player, "A flutter of butterflies has appeared!", 0x0000ff);
	return true;
}
bool spawnSpiders(World* world, Player* player) {
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
	AudioManager::playSound4D(spawnSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player, "A clutter of spiders has appeared!", 0x00ff00);
	return true;
}
bool deleteItself(World* world, Player* player) {
	player->getSelectedHotbarSlot().release();
	AudioManager::playSound4D(magicSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player, "No more fun for you!", 0x888888);
	return false;
}
static float motionSicknessDuration = 0.0f;
m4::Mat5 targetPlane = m4::Mat5::identity();
bool addMotionSickness(World* world, Player* player) {
	motionSicknessDuration += 10;
	AudioManager::playSound4D(sickSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player, "You are feeling sick...", 0x008800);
	return true;
}
bool explode(World* world, Player* player)
{
	glm::vec4 explosionPos = player->pos;
	float explosionRadius = 2.0f;
	int radius = glm::ceil(explosionRadius);
	float maxDamage = 160;
	glm::ivec4 centerBlock = glm::ivec4(glm::round(explosionPos));

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

						if (entity->getName() == "Player")
						{
							EntityPlayer* entityPlayer = reinterpret_cast<EntityPlayer*>(entity);
							if (entityPlayer->player)
								entityPlayer->player->vel += knockbackVel;
						}

						entity->takeDamage(maxDamage * strength, world);
					}
				}
			}
		}
	}

	AudioManager::playSound4D(explosionSound, "ambience", player->cameraPos, glm::vec4{ 0 });
	StateGame::instanceObj.addChatMessage(player, "You have spontaneously exploded!", 0xff0000);
	return true;
}
std::vector<std::pair<ActionFunc, int>> absoluteWeights =
{
	{doNothing,15},
	{tpUp,6},
	{addMotionSickness,4},
	{giveDeadlyOre,4},
	{giveGrass,2},
	{spawnButterflies,6},
	{spawnSpiders,3},
	{deleteItself,1},
	{explode,1}
};
std::map<int, ActionFunc, std::greater<int>> accumulatedWeights = {};
int totalWeight = 0;
bool initialized = false;

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
	
	if (self == &StateGame::instanceObj.player && motionSicknessDuration > 0)
		motionSicknessDuration -= dt;
}

//Applying actions
ActionFunc getRandomAction() {
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

	return getRandomAction()(world, player);
}
