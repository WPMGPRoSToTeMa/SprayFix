#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

enum PLUG_LOADTIME
{
	PT_NEVER,
	PT_STARTUP,             // should only be loaded/unloaded at initial hlds execution
	PT_CHANGELEVEL,         // can be loaded/unloaded between maps
	PT_ANYTIME,             // can be loaded/unloaded at any time
	PT_ANYPAUSE,            // can be loaded/unloaded at any time, and can be "paused" during a map
};

struct plugin_info_t
{
	const char*     ifvers;     // meta_interface version
	const char*     name;       // full name of plugin
	const char*     version;    // version
	const char*     date;       // date
	const char*     author;     // author name/email
	const char*     url;        // URL
	const char*     logtag;     // log message prefix (unused right now)
	PLUG_LOADTIME   loadable;   // when loadable
	PLUG_LOADTIME   unloadable; // when unloadable
};

void* g_funcFromEngine;
void* g_isSafeFileToDownload;
unsigned char g_originalBytes[1 + sizeof(void*)];
unsigned char g_patchedBytes[sizeof(g_originalBytes)];
void* g_isSafeFileToDownloadAligned;

int FixedIsSafeFileToDownload(char const* fileName) {
	if (fileName != nullptr && fileName[0] == '!') {
		return 1;
	}
	
	mprotect(g_isSafeFileToDownloadAligned, reinterpret_cast<std::size_t>(g_isSafeFileToDownload) - reinterpret_cast<std::size_t>(g_isSafeFileToDownloadAligned) + 5, PROT_READ | PROT_WRITE | PROT_EXEC);
	memcpy(g_isSafeFileToDownload, g_originalBytes, sizeof(g_originalBytes));
	mprotect(g_isSafeFileToDownloadAligned, reinterpret_cast<std::size_t>(g_isSafeFileToDownload) - reinterpret_cast<std::size_t>(g_isSafeFileToDownloadAligned) + 5, PROT_READ | PROT_EXEC);
	
	auto result = reinterpret_cast<int (*)(char const*)>(g_isSafeFileToDownload)(fileName);
	
	mprotect(g_isSafeFileToDownloadAligned, reinterpret_cast<std::size_t>(g_isSafeFileToDownload) - reinterpret_cast<std::size_t>(g_isSafeFileToDownloadAligned) + 5, PROT_READ | PROT_WRITE | PROT_EXEC);
	memcpy(g_isSafeFileToDownload, g_patchedBytes, sizeof(g_patchedBytes));
	mprotect(g_isSafeFileToDownloadAligned, reinterpret_cast<std::size_t>(g_isSafeFileToDownload) - reinterpret_cast<std::size_t>(g_isSafeFileToDownloadAligned) + 5, PROT_READ | PROT_EXEC);
	
	return result;
}

extern "C" {
	void GiveFnptrsToDll(void** pengfuncsFromEngine, void* pGlobals) {
		g_funcFromEngine = pengfuncsFromEngine[0];
	}
	
	int Meta_Query(char* interfaceVersion, plugin_info_t** pinfo, void* pMetaUtilFuncs) {
		static plugin_info_t pluginInfo {
			nullptr,
			"6xxx spray fix",
			"1.0RC1",
			"2018.06.09",
			"PRoSToC0der",
			"",
			"SPRAYFIX",
			PT_ANYTIME,
			PT_NEVER
		};
		pluginInfo.ifvers = interfaceVersion;
		*pinfo = &pluginInfo;
		return 1;
	}
	
	int Meta_Attach(PLUG_LOADTIME now, void* pFunctionTable, void* pMGlobals, void* pGamedllFuncs) {
		Dl_info info;
		dladdr(g_funcFromEngine, &info);
		
		auto engine = dlopen(info.dli_fname, RTLD_NOW);
		g_isSafeFileToDownload = dlsym(engine, "IsSafeFileToDownload");
		dlclose(engine);
		
		memcpy(g_originalBytes, g_isSafeFileToDownload, sizeof(g_originalBytes));
		
		g_patchedBytes[0] = static_cast<unsigned char>(0xE9);
		*reinterpret_cast<void**>(&g_patchedBytes[1]) = reinterpret_cast<void*>(reinterpret_cast<std::size_t>(&FixedIsSafeFileToDownload) - (reinterpret_cast<std::size_t>(g_isSafeFileToDownload) + 5));
		
		g_isSafeFileToDownloadAligned = reinterpret_cast<void*>(reinterpret_cast<std::size_t>(g_isSafeFileToDownload) & ~static_cast<std::size_t>(sysconf(_SC_PAGE_SIZE) - 1));
		
		mprotect(g_isSafeFileToDownloadAligned, reinterpret_cast<std::size_t>(g_isSafeFileToDownload) - reinterpret_cast<std::size_t>(g_isSafeFileToDownloadAligned) + 5, PROT_READ | PROT_WRITE | PROT_EXEC);
		memcpy(g_isSafeFileToDownload, g_patchedBytes, sizeof(g_patchedBytes));
		mprotect(g_isSafeFileToDownloadAligned, reinterpret_cast<std::size_t>(g_isSafeFileToDownload) - reinterpret_cast<std::size_t>(g_isSafeFileToDownloadAligned) + 5, PROT_READ | PROT_EXEC);
		
		return 1;
	}
	
	int Meta_Detach(PLUG_LOADTIME now, int reason) {
		return 1;
	}
}
