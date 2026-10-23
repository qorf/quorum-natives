/* This class implements an interface to Sapi. It inherits 
the SpeakInterface class

--1/26/10--

*/

#include "stdafx.h"
#include <string>
#include"SpeakInterface.h"

// Pragma is needed to circumvent a deprecated function error in sphelper.h
#pragma warning(disable:4996)
#include"sphelper.h"
#pragma warning(default: 4996)

#include <atlstr.h>
#include <string>
#include <sapi.h>

using std::string;

#ifndef SAPI_INTERFACE_H
#define SAPI_INTERFACE_H

class SapiInterface : public SpeakInterface
{
	public: 
		SapiInterface();
		~SapiInterface();

		virtual bool Speak(const wchar_t* StringToSpeak, bool Interrupt);
		virtual bool BlockSpeak(const wchar_t* StringToSpeak, bool Interrupt);
		virtual bool Load();
		virtual void Unload();
		virtual bool StopSpeech();
		virtual bool PauseSpeech();
		virtual bool ResumeSpeech();
		virtual bool SetVoiceNative(CString voice);
		virtual const char* GetSpeechEngineNative();
		virtual bool isSpeaking();
		virtual std::string GetVoiceNative();
		virtual bool SetVolume(long vol);
		virtual long GetVolume();
		virtual bool SetRate(long rate);
		virtual long GetRate();

	private:
		ISpVoice * pVoice;
		HRESULT engineLoaded;
		HRESULT hr;	
};
#endif //SAPI_INTERFACE_H
