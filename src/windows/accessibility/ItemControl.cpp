#include "ItemControl.h"
#include "ItemProvider.h"
#include "ControlTImpl.h"

ItemControl::ItemControl(JNIEnv* env, std::wstring&& controlName, std::wstring&& controlDescription, jobject jItem) : ControlT(env, std::move(controlName), std::move(controlDescription), jItem)
{
}

// Generate ControlT's members here, so other files can link to them.
template class ControlT<ItemControl, ItemProvider>;
