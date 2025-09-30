// When compiling external plugins, functions/variables need to be imported from ClassiCube exe instead of exporting them
#ifdef _WIN32
	// need to specifically declare as imported for MSVC
	#define CC_API __declspec(dllimport)
	#define CC_VAR __declspec(dllimport)
#else
	#define CC_API
	#define CC_VAR
#endif

#ifdef _WIN32
	// special attribute to export symbols with MSVC/MinGW
	#define PLUGIN_EXPORT __declspec(dllexport)
#else
	// public symbols are usually exported when compiling a shared library with GCC
	// but just to be on the safe side, ensure that it's always exported
	#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
