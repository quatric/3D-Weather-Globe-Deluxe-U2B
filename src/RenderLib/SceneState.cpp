// SceneState: the camera parameters a recorded movie replays (the scene's
// storeSceneStateInStream / loadSceneStateFromStream).

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(SceneState, 0)

void SceneState::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	_scene->storeSceneStateInStream(stream, flags);
}

const void *SceneState::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	if (_scene == 0)
		return (const void *)(uintptr_t)flags;	// as the original
	return _scene->loadSceneStateFromStream(data, flags);
}
