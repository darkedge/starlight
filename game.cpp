#include "game.h"
#include "input.h"
#include "log.h"
#include <process.h>

void game::Init() {
	input::Init();
	logger::Init();
	LogInfo("Сука блять!");
}

void MoveCamera() {

}

void game::Update() {
	input::BeginFrame();
	MoveCamera();
	input::EndFrame();
	logger::Render();
}
