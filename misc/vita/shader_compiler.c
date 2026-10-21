/*
 * This file is part of vitaShaRK
 * Copyright 2017, 2018, 2019, 2020 Rinnegatamante
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <psp2/shacccg.h>
#include <psp2/gxm.h>
#include <stdlib.h>
#include <psp2/kernel/modulemgr.h>
// Settings
//#include <shacccg_ext.h>
#include <vitasdk.h>
#include <stdio.h>
#include <string.h>

static int stdout_fd;
void Platform_Log(const char* msg) {
	if (!stdout_fd) stdout_fd = sceKernelGetStdout();
	sceIoWrite(stdout_fd, msg, strlen(msg));
}


typedef enum shark_opt {
	SHARK_OPT_SLOW,      //!< Equivalent to O0
	SHARK_OPT_SAFE,      //!< Equivalent to O1
	SHARK_OPT_DEFAULT,   //!< Equivalent to O2
	SHARK_OPT_FAST,      //!< Equivalent to O3
	SHARK_OPT_UNSAFE     //!< Equivalent to Ofast
} shark_opt;

typedef enum shark_type {
	SHARK_VERTEX_SHADER,
	SHARK_FRAGMENT_SHADER
} shark_type;

typedef enum shark_log_level {
	SHARK_LOG_INFO,
	SHARK_LOG_WARNING,
	SHARK_LOG_ERROR
} shark_log_level;

typedef enum shark_warn_level {
	SHARK_WARN_SILENT,
	SHARK_WARN_LOW,
	SHARK_WARN_MEDIUM,
	SHARK_WARN_HIGH,
	SHARK_WARN_MAX
} shark_warn_level;

#define SHARK_DISABLE 0
#define SHARK_ENABLE  1


// Default path for SceShaccCg module location
#define DEFAULT_SHACCCG_PATH "ur0:/data/libshacccg.suprx"

static SceUID shark_module_id = 0;
static const SceShaccCgCompileOutput *shark_output = NULL;
static SceShaccCgSourceFile shark_input;
static SceShaccCgCallbackList shark_callbacks;
static SceShaccCgCompileOptions shark_options;

// Dummy Open File callback
static SceShaccCgSourceFile *shark_open_file_cb(const char *fileName,
	const SceShaccCgSourceLocation *includedFrom,
	const SceShaccCgCompileOptions *compileOptions,
	const char **errorString)
{
	return &shark_input;
}

int shark_init(const char *path) {
	// Initializing sceShaccCg module
	shark_module_id = sceKernelLoadStartModule(path ? path : DEFAULT_SHACCCG_PATH, 0, NULL, 0, NULL, NULL);
	if (shark_module_id < 0) return shark_module_id;
	//sceShaccCgExtEnableExtensions();

	sceShaccCgSetDefaultAllocator(malloc, free);
	sceShaccCgInitializeCallbackList(&shark_callbacks, SCE_SHACCCG_TRIVIAL);
	shark_callbacks.openFile = shark_open_file_cb;
	return 0;
}

void shark_clear_output() {
	// Clearing sceShaccCg output
	if (shark_output) {
		sceShaccCgDestroyCompileOutput(shark_output);
		shark_output = NULL;
	}
}

static void log_msg(const char *msg, shark_log_level msg_level, int line) {
	char buf[4096];
	switch (msg_level) {
	case SHARK_LOG_INFO:
		sprintf(buf, "INFO: %s at line %d", msg, line);
		break;
	case SHARK_LOG_WARNING:
		sprintf(buf, "WARNING: %s at line %d", msg, line);
		break;
	case SHARK_LOG_ERROR:
		sprintf(buf, "ERROR: %s at line %d", msg, line);
		break;
	default:
		sprintf(buf, "MISC: %s at line %d", msg, line);
		break;
	}
	Platform_Log(buf);
}

SceGxmProgram *shark_compile_shader(const char *src, uint32_t *size, shark_type type) {
	// Forcing usage for memory source for the shader to compile
	shark_input.fileName = "<built-in>";
	shark_input.text = src;
	shark_input.size = *size;
	
	// Properly configuring SceShaccCg with requested settings
	sceShaccCgInitializeCompileOptions(&shark_options);
	shark_options.mainSourceFile      = shark_input.fileName;
	shark_options.targetProfile       = type;
	shark_options.entryFunctionName   = "main";
	shark_options.macroDefinitions    = NULL;
	shark_options.useFx               = true;
	shark_options.locale              = SCE_SHACCCG_ENGLISH;
	shark_options.warningLevel        = SHARK_WARN_MAX;
	shark_options.optimizationLevel   = SHARK_OPT_DEFAULT;
	shark_options.useFastmath         = false;
	shark_options.useFastint          = false;
	shark_options.useFastprecision    = false;
	shark_options.pedantic            = true;
	shark_options.performanceWarnings = true;
	
	shark_output = sceShaccCgCompileProgram(&shark_options, &shark_callbacks, 0);
	// Executing logging
	for (int i = 0; i < shark_output->diagnosticCount; i++) 
	{
		const SceShaccCgDiagnosticMessage *log = &shark_output->diagnostics[i];
		log_msg(log->message, log->level, log->location ? log->location->lineNumber : -1);
	}
	
	// Returning output
	if (shark_output->programData) *size = shark_output->programSize;
	
	return (SceGxmProgram *)shark_output->programData;
}


// Compiling all shaders in a given directory
#define SHADERS_PATH "ux0:data/shaders"
char out_dir[256];

void saveGXP(SceGxmProgram *p, uint32_t size, const char *fname) {
	FILE *f = fopen(fname, "wb");
	fwrite(p, 1, size, f);
	fclose(f);
}

void compileShader(const char *fname, int type) {
	// Reading the shader from file
	char full_name[256], out_name[256];
	sprintf(full_name, "%s/%s", SHADERS_PATH, fname);

	FILE *f = fopen(full_name, "rb");
	fseek(f, 0, SEEK_END);
	uint32_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = (char*)malloc(size);
	fread(buf, 1, size, f);
	fclose(f);
	
	Platform_Log(full_name);
	// Compiling and saving the resulting GXP file
	SceGxmProgram *p = shark_compile_shader(buf, &size, type);
	if (p) {
		char extless_name[256];
		strncpy(extless_name, fname, strstr(fname, ".cg") - fname);
		extless_name[strstr(fname, ".cg") - fname] = 0;
		sprintf(out_name, "%s/%s.gxp", out_dir, extless_name);
		saveGXP(p, size, out_name);
	}
	
	shark_clear_output();
	free(buf);
}

int main() {
	Platform_Log("begin");
	// Initializing vitaShaRK
	if (shark_init(NULL) < 0) // NOTE: libshacccg.suprx will need to be placed in ur0:data
		return -1;
	Platform_Log("begin 2..");
	
	// Creating dir for gxp output files
	sprintf(out_dir, "%s/gxp", SHADERS_PATH);
	sceIoMkdir(out_dir, 0777);
	
	// Scanning input folder
	SceIoDirent g_dir;
	int fd = sceIoDopen(SHADERS_PATH);
	while (sceIoDread(fd, &g_dir) > 0) 
	{
		if (!SCE_S_ISDIR(g_dir.d_stat.st_mode)) {
			if (strstr(g_dir.d_name, "_v.cg")) {
				compileShader(g_dir.d_name, SHARK_VERTEX_SHADER);
			} else if (strstr(g_dir.d_name, "_f.cg")) {
				compileShader(g_dir.d_name, SHARK_FRAGMENT_SHADER);
			}
		}
	}

	exit(0);
	return 0;
}

