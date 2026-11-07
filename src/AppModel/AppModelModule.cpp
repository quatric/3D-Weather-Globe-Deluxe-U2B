// The AppModel module: its bundle, its start and end, the application's
// callbacks and the unlocking of the high resolution maps.

#include "AppModel.h"
#include "CZipReader.h"

static LConstantString _sEncryptedFile("TEXTURES.zip");
static LConstantString _sEncryptionPath("TEXTURES/CLOUDS/");

static CFBundleRef _sAppModuleBundle = 0;

class _AppModuleBundleCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (_sAppModuleBundle)
			CFRelease(_sAppModuleBundle);
	}
};
static _AppModuleBundleCleanup _appModuleBundleCleanup;

IModelCallbacks *IModelCallbacks::_callbacks = 0;

void IModelCallbacks::setCallbacks(IModelCallbacks *callbacks) { _callbacks = callbacks; }

void setAppModelModule(CFBundleRef bundle)
{
	if (_sAppModuleBundle)
		CFRelease(_sAppModuleBundle);
	_sAppModuleBundle = bundle;
	if (bundle)
		CFRetain(bundle);
}

CFBundleRef getAppModelModule()
{
	if (!_sAppModuleBundle)
		setAppModelModule(CFBundleGetBundleWithIdentifier(CFSTR("com.mackiev.3DWG2.AppModel")));
	return _sAppModuleBundle;
}

void activatePublicKey()
{
	LString *key = Preferences::publicActivationKey();
	if (!key)
		return;
	CZipFileManager *manager = dynamic_cast<CZipFileManager *>(LFileManager::manager());
	if (manager)
		manager->oneArgFunc(&_sEncryptedFile, key);
}

// The maps archive is there but still locked.
bool isPublicKeyActivationNeeded()
{
	LFileManager *manager = LFileManager::manager();
	return manager && manager->objectExistsAtPath(&_sEncryptedFile, 0) &&
	       !manager->objectExistsAtPath(&_sEncryptionPath, 0);
}

void finalizeAppModel()
{
	StAutoreleasePool pool;
	if (LUndoManager::classObject()->hasSingleton())
		LUndoManager::instance()->removeAllActions();
	if (TimeController::classObject()->hasSingleton()) {
		TimeController::instance()->invalidateTimer();
		TimeController::classObject()->killSingleton();
	}
	CShapeManager::classObject()->killSingleton();
	if (CWManager::classObject()->hasSingleton())
		CWManager::instance()->invalidate();
	if (DownloadManager::classObject()->hasSingleton())
		DownloadManager::instance()->invalidate();
	AtlasDatabase::classObject()->killSingleton();
	DynamicEnvironment::classObject()->killSingleton();
	RouteSetBreak::classObject()->killSingleton();
	InstallationController::classObject()->killSingleton();
}

// The intro demo plays without downloads competing for the network.
void initializeAppModel(LString *)
{
	if (Preferences::showIntroOnStartup())
		DownloadManager::instance()->pauseQueue();
}
