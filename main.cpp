#include <4dm.h>
#include "Sounds.h"

using namespace fdm;

// Initialize the DLLMain
initDLL

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
	/*
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
	self->quitButton.callback = settingsCallback;*/
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

//Init stuff
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	if (recipes.empty()) return;

	for (const auto& recipe : recipes) {
		if (std::any_of(CraftingMenu::recipes.begin(),
			CraftingMenu::recipes.end(),
			[&recipe](const nlohmann::json& globalRecipe) {
				return globalRecipe == recipe;
			})) continue;
		CraftingMenu::recipes.push_back(recipe);
	}
}
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
void InitRecipes() {

	addRecipe("Funny Button", 1, { {"Glass",1},{"Red Flower", 1}, {"Stone", 5} });
}
void InitBlueprints() {
	(Item::blueprints)["Funny Button"] =
	{
		{ "type", "material"},
		{ "baseAttributes", nlohmann::json::object()}
	};

}
void InitSounds() {
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
$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	//Initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();

	InitBlueprints();

	InitRecipes();

	InitSounds();
}