#include"stdafx.h"
#include <stdlib.h>
#include <string>
#include"NVDAInterface.h"

#ifndef NVDA_INTERFACE_CPP
#define NVDA_INTERFACE_CPP

/*
	NVDA's controller client (nvdaControllerClient.dll) is loaded the first time
	it's needed, from the folder Windows.dll is in, rather than linked: it's LGPL
	and ships as its own file, and a missing file then only turns NVDA off
	instead of stopping Windows.dll from loading. Each function is looked up on
	its own, so an older client or NVDA only loses the newer features:

	- 1.0, every NVDA: testIfRunning, speakText, cancelSpeech
	- 2.0, NVDA 2024.1 and later: speakSsml (synchronous, for blocking speech)
	- 3.0, NVDA 2026.3 and later: isSpeaking

	The calls return 0 on success. The newer ones return an error (1717) when
	the running NVDA is older than the client.
*/
typedef unsigned long (__stdcall *NoArgumentCall)(void);
typedef unsigned long (__stdcall *SpeakTextCall)(const wchar_t* text);
typedef unsigned long (__stdcall *SpeakSsmlCall)(const wchar_t* ssml, int symbolLevel, int priority, unsigned char asynchronous);
typedef unsigned long (__stdcall *IsSpeakingCall)(unsigned char* speaking);

// The 3.0 client's name, then the older one, in case an old jni folder still has it.
static const wchar_t* NVDA_FILE_NAMES[] = { L"nvdaControllerClient.dll", L"nvdaControllerClient64.dll" };

// Values from NVDA's nvdaController.h.
static const int SYMBOL_LEVEL_UNCHANGED = -1;
static const int SPEECH_PRIORITY_NORMAL = 0;

// How often a blocking speak checks whether NVDA is still speaking, and how
// long it waits for speech to start.
static const DWORD POLL_MILLISECONDS = 20;
static const DWORD START_LIMIT_MILLISECONDS = 1000;

static bool nvdaLookedFor = false;
static HMODULE nvdaLibrary = NULL;
static NoArgumentCall testIfRunningCall = NULL;
static NoArgumentCall cancelSpeechCall = NULL;
static SpeakTextCall speakTextCall = NULL;
static SpeakSsmlCall speakSsmlCall = NULL;
static IsSpeakingCall isSpeakingCall = NULL;

/* The folder this DLL (Windows.dll) is in, ending with a backslash. */
static std::wstring GetOwnFolder()
{
	HMODULE self = NULL;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(&GetOwnFolder), &self))
	{
		return std::wstring();
	}

	std::wstring path(32768, L'\0');
	DWORD length = GetModuleFileNameW(self, &path[0], (DWORD)path.size());
	if (length == 0 || length >= path.size())
	{
		return std::wstring();
	}
	path.resize(length);

	size_t slash = path.find_last_of(L"\\/");
	if (slash == std::wstring::npos)
	{
		return std::wstring();
	}
	return path.substr(0, slash + 1);
}

/* Loads NVDA's client the first time, and returns whether it's usable. */
static bool LoadNVDAClient()
{
	if (nvdaLookedFor)
	{
		return nvdaLibrary != NULL;
	}
	nvdaLookedFor = true;

	std::wstring folder = GetOwnFolder();
	for (int i = 0; i < 2 && nvdaLibrary == NULL; i++)
	{
		std::wstring path = folder + NVDA_FILE_NAMES[i];
		nvdaLibrary = LoadLibraryW(path.c_str());
	}
	if (nvdaLibrary == NULL)
	{
		return false;
	}

	testIfRunningCall = (NoArgumentCall)GetProcAddress(nvdaLibrary, "nvdaController_testIfRunning");
	cancelSpeechCall = (NoArgumentCall)GetProcAddress(nvdaLibrary, "nvdaController_cancelSpeech");
	speakTextCall = (SpeakTextCall)GetProcAddress(nvdaLibrary, "nvdaController_speakText");
	speakSsmlCall = (SpeakSsmlCall)GetProcAddress(nvdaLibrary, "nvdaController_speakSsml");
	isSpeakingCall = (IsSpeakingCall)GetProcAddress(nvdaLibrary, "nvdaController_isSpeaking");

	if (testIfRunningCall == NULL || speakTextCall == NULL)
	{
		FreeLibrary(nvdaLibrary);
		nvdaLibrary = NULL;
		return false;
	}
	return true;
}

/* Escapes text for use inside SSML. */
static std::wstring EscapeXml(const wchar_t* text)
{
	std::wstring result;
	for (const wchar_t* c = text; *c != L'\0'; c++)
	{
		switch (*c)
		{
			case L'&': result += L"&amp;"; break;
			case L'<': result += L"&lt;"; break;
			case L'>': result += L"&gt;"; break;
			case L'"': result += L"&quot;"; break;
			case L'\'': result += L"&apos;"; break;
			default: result += *c; break;
		}
	}
	return result;
}

/* Whether NVDA reports it's speaking. False if the client or NVDA is older than 3.0 / 2026.3. */
static bool NVDAIsSpeaking()
{
	if (isSpeakingCall == NULL)
	{
		return false;
	}
	unsigned char speaking = 0;
	return isSpeakingCall(&speaking) == 0 && speaking != 0;
}

bool NVDAInterface::Speak(const wchar_t* StringToSpeak, bool Interrupt)
{
	if (!LoadNVDAClient())
	{
		return false;
	}
	return speakTextCall(StringToSpeak) == 0;
}

/*
	Speaks and waits until NVDA has finished: with synchronous SSML (NVDA 2024.1
	and later), or else by checking isSpeaking (NVDA 2026.3 and later). With an
	older NVDA, it speaks without waiting, as before.
*/
bool NVDAInterface::BlockSpeak(const wchar_t* StringToSpeak, bool Interrupt)
{
	if (!LoadNVDAClient())
	{
		return false;
	}

	if (speakSsmlCall != NULL)
	{
		std::wstring ssml = L"<speak>" + EscapeXml(StringToSpeak) + L"</speak>";
		if (speakSsmlCall(ssml.c_str(), SYMBOL_LEVEL_UNCHANGED, SPEECH_PRIORITY_NORMAL, 0) == 0)
		{
			return true;
		}
		// An older NVDA (error 1717): speak plainly below.
	}

	if (speakTextCall(StringToSpeak) != 0)
	{
		return false;
	}

	if (isSpeakingCall != NULL)
	{
		// Wait for speech to start, then for it to end.
		DWORD waited = 0;
		while (!NVDAIsSpeaking() && waited < START_LIMIT_MILLISECONDS)
		{
			Sleep(POLL_MILLISECONDS);
			waited += POLL_MILLISECONDS;
		}
		while (NVDAIsSpeaking())
		{
			Sleep(POLL_MILLISECONDS);
		}
	}
	return true;
}

bool NVDAInterface::Load()
{
	return LoadNVDAClient() && testIfRunningCall() == 0;
}

// The client DLL stays loaded for the life of the program.
void NVDAInterface::Unload() {}

bool NVDAInterface::StopSpeech()
{
	if (!LoadNVDAClient() || cancelSpeechCall == NULL)
	{
		return false;
	}
	return cancelSpeechCall() == 0;
}

bool NVDAInterface::SetVoiceNative(CString voice) {return false;}

std::string NVDAInterface::GetVoiceNative()
{
	return std::string("");
}

const char* NVDAInterface::GetSpeechEngineNative()
{
	return "NVDA";
}

bool NVDAInterface::isSpeaking()
{
	return LoadNVDAClient() && NVDAIsSpeaking();
}

bool NVDAInterface::PauseSpeech() {
	// not supported
	return false;
}
bool NVDAInterface::ResumeSpeech() {
	// not supported
	return false;
}

bool NVDAInterface::SetVolume(long vol) {
	// not supported
	return false;
}
long NVDAInterface::GetVolume() {
	// not supported.
	return 0;
}

bool NVDAInterface::SetRate(long rate) {
	// not supported
	return false;
}

long NVDAInterface::GetRate() {
	// not supported.
	return 0;
}

#endif //NVDA_INTERFACE_CPP
