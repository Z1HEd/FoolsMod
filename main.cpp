#include <4dm.h>
#include "Sounds.h"

using namespace fdm;

// Initialize the DLLMain
initDLL

MeshRenderer renderer{};
std::vector<nlohmann::json> recipes = {};
std::string buttonPressSound = "assets/SwitchSound.ogg";
std::string explosionSound = "assets/ExplosionSound.ogg";
std::string magicSound = "assets/MagicSound.ogg";
std::string sickSound = "assets/SicknessSound.ogg";
std::string spawnSound = "assets/SpawnSound.ogg";
std::string windSound = "assets/WindSound.ogg";


// Mangle main menu buttons
$hook(void, StateTitleScreen,init, StateManager& s) {
	original(self, s);
	
	auto singleplayerCallback = self->singleplayerButton.callback;
	auto multiplayerCallback = self->multiplayerButton.callback;
	auto tutorialCallback = self->tutorialButton.callback;
	auto settingsCallback = self->settingsButton.callback;
	auto creditsCallback = self->creditsButton.callback;
	auto quitCallback = self->quitButton.callback;

	self->singleplayerButton.callback = quitCallback;
	self->multiplayerButton.callback = creditsCallback;
	self->tutorialButton.callback = multiplayerCallback;
	self->settingsButton.callback = singleplayerCallback;
	self->creditsButton.callback = tutorialCallback;
	self->quitButton.callback = settingsCallback;
}

// Button press detection



// Render item material
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	if (self->name != "Funny Button") return original(self, pos);
	TexRenderer& tr = ItemTool::tr;
	FontRenderer& fr = ItemMaterial::fr;

	const Tex2D* ogTex = tr.texture; // remember the original texture

	static std::string iconPath = "assets/RedButton.png";

	tr.texture = ResourceManager::get(iconPath, true); // set to custom texture
	tr.setClip(0, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();
	tr.texture = ogTex; // return to the original texture
}

// Render button item

$hook(void, ItemMaterial, renderEntity, const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) {
	if (self->getName() != "Funny Button") return original(self, MV, inHand, lightDir);

	Player* player = &StateGame::instanceObj.player;

	m4::Mat5 baseMV = MV;
	baseMV.translate(glm::vec4{ 0.0f, .6f, -0.5f, 0.001f });
	baseMV *= m4::Rotor
	(
		{
			m4::wedge({0, 0, 1, 0}, {0, 1, 0, 0}), // ZY
			glm::pi<float>() / 3
		}
	);
	baseMV.scale(glm::vec4{ 0.5f,.7f,0.25f,0.25f });
	baseMV.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 buttonMV = MV;
	buttonMV.translate(glm::vec4{ 0.0f, .6f, -0.5f, 0.001f });
	buttonMV *= m4::Rotor
	(
		{
			m4::wedge({0, 0, 1, 0}, {0, 1, 0, 0}), // ZY
			glm::pi<float>() / 3
		}
	);
	buttonMV.translate(glm::vec4{ 0.0f , 0.0f, 0.1f, 0.0f });
	buttonMV.scale(glm::vec4{ 0.3f,.30f,0.2f,0.07f });
	buttonMV.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	const Shader* shader = ShaderManager::get("tetSolidColorNormalShader");

	shader->use();

	glUniform4f(glGetUniformLocation(shader->id(), "lightDir"), lightDir.x, lightDir.y, lightDir.z, lightDir.w);
	//IRON COLOR
	glUniform4f(glGetUniformLocation(shader->id(), "inColor"), 111.0f / 255.0f, 110.0f / 255.0f, 109.0f / 255.0f, 1);
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(baseMV) / sizeof(float), &baseMV[0][0]);
	renderer.render();

	// RED COLOR
	glUniform4f(glGetUniformLocation(shader->id(), "inColor"), 1, 0, 0, 1);
	glUniform1fv(glGetUniformLocation(shader->id(), "MV"), sizeof(buttonMV) / sizeof(float), &buttonMV[0][0]);
	renderer.render();

}

// Init stuff
void addRecipe(const std::string& resultName, int resultCount,
	const std::vector<std::pair<std::string, int>>& components) {

	nlohmann::json recipeJson;
	recipeJson["result"] = { {"name", resultName}, {"count", resultCount} };

	nlohmann::json recipeComponents = nlohmann::json::array();
	for (const auto& [name, count] : components) {
		recipeComponents.push_back({ {"name", name}, {"count", count} });
	}

	recipeJson["recipe"] = recipeComponents;
	recipes.push_back(recipeJson);
}
void initRecipes() {

	addRecipe("Funny Button", 1, { {"Glass",1},{"Red Flower", 1}, {"Stone", 5} });
}
void initBlueprints() {
	(Item::blueprints)["Funny Button"] =
	{
		{ "type", "material"},
		{ "baseAttributes", nlohmann::json::object()}
	};

}
void initSounds() {
	buttonPressSound = std::format("../../{}/{}", fdm::getModPath(fdm::modID), buttonPressSound);
	explosionSound = std::format("../../{}/{}", fdm::getModPath(fdm::modID), explosionSound);
	windSound = std::format("../../{}/{}", fdm::getModPath(fdm::modID), windSound);
	magicSound = std::format("../../{}/{}", fdm::getModPath(fdm::modID), magicSound);
	sickSound = std::format("../../{}/{}", fdm::getModPath(fdm::modID), sickSound);
	spawnSound = std::format("../../{}/{}", fdm::getModPath(fdm::modID), spawnSound);

	if (!AudioManager::loadSound(explosionSound)) Console::printLine("Cannot load sound: ", explosionSound);
	if (!AudioManager::loadSound(windSound)) Console::printLine("Cannot load sound: ", windSound);
	if (!AudioManager::loadSound(spawnSound)) Console::printLine("Cannot load sound: ", spawnSound);
	if (!AudioManager::loadSound(sickSound)) Console::printLine("Cannot load sound: ", sickSound);
	if (!AudioManager::loadSound(magicSound)) Console::printLine("Cannot load sound: ", magicSound);
	if (!AudioManager::loadSound(buttonPressSound)) Console::printLine("Cannot load sound: ", buttonPressSound);
}
void initRenderer() {
	MeshBuilder mesh{ BlockInfo::HYPERCUBE_FULL_INDEX_COUNT };
	// vertex position attribute
	mesh.addBuff(BlockInfo::hypercube_full_verts, sizeof(BlockInfo::hypercube_full_verts));
	mesh.addAttr(GL_UNSIGNED_BYTE, 4, sizeof(glm::u8vec4));
	// per-cell normal attribute
	mesh.addBuff(BlockInfo::hypercube_full_normals, sizeof(BlockInfo::hypercube_full_normals));
	mesh.addAttr(GL_FLOAT, 1, sizeof(GLfloat));

	mesh.setIndexBuff(BlockInfo::hypercube_full_indices, sizeof(BlockInfo::hypercube_full_indices));

	renderer.setMesh(&mesh);
}
$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	//Initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();

	initSounds();
	initRenderer();
}
$hookStatic(bool, Item, loadItemInfo)
{
	bool result = original();

	static bool loaded = false;
	if (loaded) return result;
	loaded = true;

	initBlueprints();

	return result;
}
$hookStatic(bool, CraftingMenu, loadRecipes)
{
	bool result = original();

	static bool loaded = false;
	if (loaded) return result;
	loaded = true;
	initRecipes();

	if (recipes.empty()) return result;

	for (const auto& recipe : recipes) {
		if (std::any_of(CraftingMenu::recipes.begin(),
			CraftingMenu::recipes.end(),
			[&recipe](const nlohmann::json& globalRecipe) {
				return globalRecipe == recipe;
			})) continue;
		CraftingMenu::recipes.push_back(recipe);
	}
	return result;
}