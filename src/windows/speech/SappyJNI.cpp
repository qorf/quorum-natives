// This is the main DLL file.

#include "stdafx.h"
#include "SpeakInterface.h"
#include "SapiInterface.h"
#include "JawsInterface.h"
#include "NVDAInterface.h"
#include "SappyJNI.h"
#include <sapi.h>
#include <atlstr.h>
#include <string>

// If true, the default engine used by the TextToSpeech factory will be SAPI.
bool SappyJNI::defaultToSAPI = true;

//Create an interface for potential screen readers. NULL when no engine could be loaded.
SpeakInterface *Interface = NULL;
std::wstring Engine; // the last requested engine, or empty.

/*
 * Copies a Java string into a wide string. GetStringChars' text has no
 * terminating zero, so it can't be passed to the speech engines directly.
 */
static std::wstring ToWide(JNIEnv *env, jstring value)
{
	if (value == NULL) {
		return std::wstring();
	}
	jsize length = env->GetStringLength(value);
	std::wstring result(length, L'\0');
	if (length > 0) {
		env->GetStringRegion(value, 0, length, reinterpret_cast<jchar*>(&result[0]));
	}
	return result;
}

/*
 * Unloads and deletes the current interface, leaving it NULL.
 */
static void ClearInterface()
{
	if (Interface != NULL) {
		Interface->Unload();
		delete Interface;
		Interface = NULL;
	}
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    speakNative
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_sapi_TextToSpeech_speakNative
(JNIEnv *env, jobject obj, jstring str) {
	if (Interface == NULL) {
		return;
	}
	std::wstring sayThis = ToWide(env, str);
	Interface->Speak(sayThis.c_str(), true);
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    isSpeaking
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_isSpeaking
  (JNIEnv *, jobject)
{
	return Interface != NULL && Interface->isSpeaking();
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    stopNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_sapi_TextToSpeech_stopNative
(JNIEnv * env, jobject obj) {
	if (Interface != NULL) {
		Interface->StopSpeech();
	}
}

/*
*	Gets a string representation of the text to speech engine.
*/
JNIEXPORT jstring JNICALL Java_org_sapi_TextToSpeech_getTextToSpeechEngine
(JNIEnv *env,jobject obj) {
	if(Interface != NULL) {
		const char *ch = Interface->GetSpeechEngineNative();
		return env->NewStringUTF(ch);
	}
	else {
		const char *ch = "NULL";
		return env->NewStringUTF(ch);
	}
}

bool setEngine(const std::wstring &requested) {
	CString input = requested.c_str();
	CString jaws_string = L"JAWS";
	CString nvda_string = L"NVDA";
	CString sapi_string = L"MICROSOFT_SAPI";
	SpeakInterface *NewInterface = NULL;

	bool jaws_request = input.Compare(jaws_string) == 0;
	bool nvda_request = input.Compare(nvda_string) == 0;
	bool sapi_request = input.Compare(sapi_string) == 0;

	if(!jaws_request && !nvda_request && !sapi_request) {
		return false;
	}

	if(jaws_request) {
		JawsInterface JawsTest;
		if(JawsTest.Load() && JawsTest.Speak(L"",true)){
			NewInterface = new JawsInterface();
			NewInterface->Load();
			NewInterface->Speak(L"", true);
		}
	}
	else if(nvda_request) {
		NVDAInterface NVDATest;
		if(NVDATest.Load()){
			NewInterface = new NVDAInterface();
			NewInterface->Speak(L"", true);
		}
	}
	else if(sapi_request) {
		NewInterface = new SapiInterface();
		NewInterface->Load();
		NewInterface->Speak(L"", true);
	}

	if(NewInterface != NULL) {
		ClearInterface();
		Interface = NewInterface;
		return true;
	}

	return false;
}
/*
 * Attempts to set the text to speech engine.
 */
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_setTextToSpeechEngine
(JNIEnv *env,jobject obj,jstring str) {
	// Set the last requested engine.
	Engine = ToWide(env, str);
	return setEngine(Engine);
}

/*
 * Loads the default engine: SAPI when defaultToSAPI is true; otherwise JAWS
 * if it can speak, else NVDA if it's running, else SAPI. Interface is NULL if
 * the chosen engine doesn't load.
 */
static void loadDefaultEngine()
{
	if (SappyJNI::defaultToSAPI)
	{
		Interface = new SapiInterface();
	}
	else
	{
		//These Jaws and NVDA objects are created to test whether
		//Jaws or NVDA is currently running on the system
		JawsInterface JawsTest;
		NVDAInterface NVDATest;

		//If jaws can speak load the jaws interface
		if(JawsTest.Load() && JawsTest.Speak(L"",true)){
			Interface = new JawsInterface();
		}

		//If JAWS cannot speak and NVDA can, load the NVDA interface.
		else if(NVDATest.Load()){
			Interface = new NVDAInterface();
		}

		//If jaws and NVDA cannot, load the sapi interface
		else{
			Interface = new SapiInterface();
		}
	}

	if(!Interface->Load()){
		delete Interface;
		Interface = NULL;
	}
	// Have the interface speak an empty line--if you don't,
	// SAPI doesn't speak when you call the speakNative function
	else{
		Interface->Speak(L"", true);
	}
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    createTTSNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_sapi_TextToSpeech_createTTSNative
(JNIEnv *, jobject) {
	ClearInterface();
	loadDefaultEngine();
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    destroyTTSNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_sapi_TextToSpeech_destroyTTSNative
(JNIEnv *, jobject) {
	ClearInterface();
}

//patched together with pieces from the Quadmore project
//http://www.quadmore.com/JAVA_to_SAPI/
//but also from some of the SAPI documentation
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_setVoiceNative(JNIEnv *env,jobject obj,jstring strInput)
{
	if (Interface == NULL || strInput == NULL) {
		return false;
	}

	const char *str = env->GetStringUTFChars(strInput, 0);
	if (str == NULL)
	{
		return false; /* OutOfMemoryError already thrown */
	}
	CString strVoiceSelected(str);
	env->ReleaseStringUTFChars(strInput, str);

	return(Interface->SetVoiceNative(strVoiceSelected));
}

//Copied almost verbatim, with modifications for compiling on Visual Studio 2008 (outside of MFC)
//from the quadmore project
//http://www.quadmore.com/JAVA_to_SAPI/
JNIEXPORT jstring JNICALL Java_org_sapi_TextToSpeech_getVoicesNative(JNIEnv *env, jobject obj)
{
	if (Interface == NULL) {
		return env->NewStringUTF("");
	}
	return env->NewStringUTF(Interface->GetVoiceNative().c_str());
}

// This uses a blocking call for speaking; interfaces that support blocking calls will not be
// interrupted while speaking.
JNIEXPORT void JNICALL Java_org_sapi_TextToSpeech_speakBlockNative
	(JNIEnv *env,jobject obj,jstring str)
{
	if (Interface == NULL) {
		return;
	}
	std::wstring sayThis = ToWide(env, str);
	Interface->BlockSpeak(sayThis.c_str(), true);
}

JNIEXPORT void JNICALL Java_org_sapi_TextToSpeech_reinitializeNative
	(JNIEnv *env,jobject obj)
{
	if (!Engine.empty()) {
		// Re-load by changing the engine again.
		setEngine(Engine);
		return;
	}

	// No engine is selected--use the default method.
	ClearInterface();
	loadDefaultEngine();
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    pauseNative
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_pauseNative
	(JNIEnv *env,jobject obj)
{
	return Interface != NULL && Interface->PauseSpeech();
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    resumeNative
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_resumeNative
	(JNIEnv *env,jobject obj)
{
	return Interface != NULL && Interface->ResumeSpeech();
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    setRateNative
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_setRateNative
	(JNIEnv *env,jobject obj, jlong rate)
{
	return Interface != NULL && Interface->SetRate((long)rate);
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    getRateNative
 * Signature: ()V
 */
JNIEXPORT jlong JNICALL Java_org_sapi_TextToSpeech_getRateNative
(JNIEnv *env,jobject obj)
{
	return Interface != NULL ? Interface->GetRate() : 0;
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    setVolumeNative
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_org_sapi_TextToSpeech_setVolumeNative
	(JNIEnv *env,jobject obj, jlong vol)
{
	return Interface != NULL && Interface->SetVolume((long)vol);
}

/*
 * Class:     org_sapi_TextToSpeech
 * Method:    getVolumeNative
 * Signature: ()V
 */
JNIEXPORT jlong JNICALL Java_org_sapi_TextToSpeech_getVolumeNative
	(JNIEnv *env,jobject obj)
{
	return Interface != NULL ? Interface->GetVolume() : 0;
}
