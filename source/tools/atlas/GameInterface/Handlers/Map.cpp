#include "precompiled.h"

#include "MessageHandler.h"

#include "graphics/Patch.h"
#include "graphics/TextureManager.h"
#include "graphics/TextureEntry.h"
#include "ps/Game.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"

namespace AtlasMessage {

static void InitGame(std::wstring map)
{
	if (g_Game)
		delete g_Game;

	// Set attributes for the game:
	//  Start without a map
	g_GameAttributes.m_MapFile = map;
	//  Make all players locally controlled
	for (int i=1; i<8; ++i) 
		g_GameAttributes.GetSlot(i)->AssignLocal();

	// Start the game:
	g_Game = new CGame();
	PSRETURN ret = g_Game->StartGame(&g_GameAttributes);
	debug_assert(ret == PSRETURN_OK);
	LDR_NonprogressiveLoad();
	ret = g_Game->ReallyStartGame();
	debug_assert(ret == PSRETURN_OK);

	// Make sure entities get rendered in the correct location
	g_Game->GetSimulation()->Update(0.0);
}

MESSAGEHANDLER(GenerateMap)
{
	InitGame(L"");

	// Convert size in patches to number of vertices
	int vertices = msg->size * PATCH_SIZE + 1;

	// Generate flat heightmap
	u16* heightmap = new u16[vertices*vertices];
	for (int z = 0; z < vertices; ++z)
		for (int x = 0; x < vertices; ++x)
//			heightmap[x + z*vertices] = 32768 +(int)(2048.f*(rand()/(float)RAND_MAX-0.5f));
			heightmap[x + z*vertices] = 16384;

	// Initialise terrain using the heightmap
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	terrain->Initialize(msg->size, heightmap);

	delete[] heightmap;

	// Cover terrain with default texture
	// TODO: split into fCoverWithTexture
	CTextureEntry* texentry = g_TexMan.FindTexture("grass1_spring.dds"); // TODO: make default customisable
	Handle tex = texentry ? texentry->GetHandle() : 0;

	int patches = terrain->GetPatchesPerSide();
	for (int pz = 0; pz < patches; ++pz) {
		for (int px = 0; px < patches; ++px) {

			CPatch* patch = terrain->GetPatch(px, pz);

			for (int z = 0; z < PATCH_SIZE; ++z)
				for (int x = 0; x < PATCH_SIZE; ++x)
					patch->m_MiniPatches[z][x].Tex1 = tex;
		}
	}

}

MESSAGEHANDLER(LoadMap)
{
	InitGame(msg->filename);
}

}
