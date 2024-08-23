/* Not available on older SDKs */
typedef cc_uintptr _DWORD_PTR;

/* === BEGIN mmsyscom.h === */
#define CALLBACK_NULL  0x00000000l
typedef UINT           MMRESULT;
#define WINMMAPI       DECLSPEC_IMPORT
#define MMSYSERR_BADDEVICEID 2

/* === BEGIN mmeapi.h === */
typedef struct WAVEHDR_ {
	LPSTR       lpData;
	DWORD       dwBufferLength;
	DWORD       dwBytesRecorded;
	_DWORD_PTR  dwUser;
	DWORD       dwFlags;
	DWORD       dwLoops;
	struct WAVEHDR_* lpNext;
	DWORD_PTR   reserved;
} WAVEHDR;

typedef struct WAVEFORMATEX_ {
	WORD  wFormatTag;
	WORD  nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD  nBlockAlign;
	WORD  wBitsPerSample;
	WORD  cbSize;
} WAVEFORMATEX;
typedef void* HWAVEOUT;

#define WAVE_MAPPER     ((UINT)-1)
#define WAVE_FORMAT_PCM 1
#define WHDR_DONE       0x00000001
#define WHDR_PREPARED   0x00000002

WINMMAPI MMRESULT WINAPI waveOutOpen(HWAVEOUT* phwo, UINT deviceID, const WAVEFORMATEX* fmt, _DWORD_PTR callback, _DWORD_PTR instance, DWORD flags);
WINMMAPI MMRESULT WINAPI waveOutClose(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT hwo, WAVEHDR* hdr, UINT hdrSize);
WINMMAPI MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT hwo, WAVEHDR* hdr, UINT hdrSize);
WINMMAPI MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, WAVEHDR* hdr, UINT hdrSize);
WINMMAPI MMRESULT WINAPI waveOutReset(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutGetErrorTextA(MMRESULT err, LPSTR text, UINT textLen);
WINMMAPI UINT     WINAPI waveOutGetNumDevs(void);
/* === END mmeapi.h === */