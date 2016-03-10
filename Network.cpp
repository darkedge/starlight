#include "Network.h"
#include "starlight_game.h"
#include "starlight_generated.h"
#include "starlight_glm.h"
#include <sstream>
#include <enet/enet.h>
#include "starlight_log.h"
#include <imgui.h>
// All the stuff needed for networking.

// We will create one of these every network tick.
// These should be tailored per client to prevent cheating.
// Keyword: Potential visible set
struct Snapshot {

};

// TODO (Client): Keep a ringbuffer of Snapshots (100 ms).


// Client -> Server.
// Sent every client frame.
struct Command {
	int32_t timestamp;
	// player euler view angles in fixed point
	//glm::tvec<int32_t> orientation;
	uint32_t inputFlags;
	uint32_t heldItem;
	// TODO: desired movement vector
};

void network::Update(GameInfo* gameInfo) {
	if (gameInfo->server) {
		ENetEvent event;
		std::stringstream ss;
		while (enet_host_service(gameInfo->server, &event, 0) > 0)
		{
			ss.clear();
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				ss << "[SERVER] A new client connected from " << event.peer->address.host << ':' << event.peer->address.port << '.';
				logger::LogInfo(ss.str());
				/* Store any relevant client information here. */
				event.peer->data = "Client information";
				break;

			case ENET_EVENT_TYPE_RECEIVE:
			{
				ss << "[SERVER] A packet of length " << std::to_string(event.packet->dataLength) << " was received on channel " << std::to_string(event.channelID) << '.';
				logger::LogInfo(ss.str());
				ss.str("");
				ss.clear();

				ss << "Contents: ";
				flatbuffers::FlatBufferBuilder builder;
				auto packet = network::GetPacket(event.packet->data);
				auto union_type = packet->message_type();
				if (union_type == MessageType_Chat) {
					auto chat = static_cast<const network::Chat*>(packet->message());
					ss << chat->message()->str();
				}
				else {
					ss << " Unknown!";
				}
				logger::LogInfo(ss.str());

				enet_packet_destroy(event.packet);
			}
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				ss << "[SERVER] " << (const char*)event.peer->data << " disconnected.";
				logger::LogInfo(ss.str());

				// TODO: Disconnect in-game

				event.peer->data = nullptr;
				break;

			default:
				break;
			}
		}
	}

	if (gameInfo->client) {
		ENetEvent event;
		std::stringstream ss;
		while (enet_host_service(gameInfo->client, &event, 0) > 0)
		{
			ss.clear();
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				ss << "[CLIENT] Connected to " << event.peer->address.host << ':' << event.peer->address.port << '.';
				logger::LogInfo(ss.str());
				/* Store any relevant client information here. */
				event.peer->data = "Client information";
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				ss << "[CLIENT] A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->data << " on channel " << event.channelID << '.';
				logger::LogInfo(ss.str());

				// TODO: Flatbuffer unpack

				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				ss << "[CLIENT] " << (const char*)event.peer->data << " disconnected.";
				logger::LogInfo(ss.str());

				// TODO: Disconnect in-game

				event.peer->data = nullptr;



				break;

			default:
				break;
			}
		}
	}
}

void network::DrawDebugMenu(GameInfo* gameInfo) {
	ImGui::Begin("Network");
	if (ImGui::Button("Create Server")) {
		if (gameInfo->server) {
			logger::LogInfo("Server already created.");
		}
		else {
			// TODO: Extract magic numbers
			ENetAddress address;
			address.host = ENET_HOST_ANY;
			address.port = 1234;
			gameInfo->server = enet_host_create(&address, 32, 2, 0, 0);
			if (gameInfo->server) {
				logger::LogInfo("Successfully created server.");
			}
			else {
				logger::LogInfo("Failed to create server!");
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Destroy Server")) {
		if (gameInfo->server) {
			enet_host_destroy(gameInfo->server);
			logger::LogInfo("Destroyed server.");
			gameInfo->server = nullptr;
		}
		else {
			logger::LogInfo("No server to destroy.");
		}
	}
	if (ImGui::Button("Create Client")) {
		if (gameInfo->client) {
			logger::LogInfo("Client already created.");
		}
		else {
			gameInfo->client = enet_host_create(nullptr, 1, 2, 57600 >> 3, 14400 >> 3);
			if (gameInfo->client) {
				logger::LogInfo("Successfully created client.");
			}
			else {
				logger::LogInfo("Failed to create client!");
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Destroy Client")) {
		if (gameInfo->client) {
			// Disconnect first
			if (gameInfo->peer) {
				enet_peer_disconnect_now(gameInfo->peer, 0);
				gameInfo->peer = nullptr;
			}
			enet_host_destroy(gameInfo->client);
			gameInfo->client = nullptr;
			logger::LogInfo("Destroyed client.");
		}
		else {
			logger::LogInfo("No client to destroy.");
		}
	}
	if (ImGui::Button("Connect to localhost")) {
		if (gameInfo->client) {
			ENetAddress address;
			enet_address_set_host(&address, "127.0.0.1");
			address.port = 1234;
			// Some DLLs are loaded the first time this line gets executed
			gameInfo->peer = enet_host_connect(gameInfo->client, &address, 1, 0);
			if (gameInfo->peer) {
				logger::LogInfo("Connecting...");
			}
			else {
				logger::LogInfo("Connection already in use.");
			}
		}
		else {
			logger::LogInfo("Create a client first.");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Disconnect")) {
		if (gameInfo->client && gameInfo->peer) {
			enet_peer_disconnect(gameInfo->peer, 0);
			gameInfo->peer = nullptr;
		}
	}
	ImGui::End();
}