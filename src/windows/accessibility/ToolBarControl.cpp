#include "ToolBarControl.h"
#include "ToolBarProvider.h"
#include "ControlTImpl.h"

ToolBarControl::ToolBarControl(JNIEnv* env, std::wstring&& name, jobject jItem)
	: ControlT(env, std::move(name), L"", jItem)
{
}

// Generate ControlT's members here, so other files can link to them.
template class ControlT<ToolBarControl, ToolBarProvider>;
