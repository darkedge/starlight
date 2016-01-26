#include "game.h"
#include "input.h"

void game::Init() {
	input::Init();
}

void MoveCamera() {

}

void game::Update() {
	input::BeginFrame();
	MoveCamera();
	input::EndFrame();
}
