/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy */

#ifndef _Included_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategys_FreeTypeStrategy
#define _Included_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    InitFreeType
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_InitFreeType
  (JNIEnv *, jobject);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    LoadFontC
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_LoadFontC
  (JNIEnv *, jobject, jlong, jstring);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    SetSizeC
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_SetSizeC
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    SetAngleC
 * Signature: (JD)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_SetAngleC
  (JNIEnv *, jobject, jlong, jdouble);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    LoadBitmap
 * Signature: ([JC)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_LoadBitmap
  (JNIEnv *, jobject, jlongArray, jchar, jlong);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    LoadSDFBitmap
 * Signature: ([JC)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_LoadSDFBitmap
  (JNIEnv *, jobject, jlongArray, jchar, jlong);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    GetLineHeightNative
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_GetLineHeightNative
  (JNIEnv *, jobject, jlong);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    GetMaximumAscentNative
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_GetMaximumAscentNative
  (JNIEnv*, jobject, jlong);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    GetMaximumDescentNative
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_GetMaximumDescentNative
  (JNIEnv*, jobject, jlong);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    GetUnderlinePositionNative
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_GetUnderlinePositionNative
  (JNIEnv*, jobject, jlong);

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:    GetUnderlineThicknessNative
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_GetUnderlineThicknessNative
  (JNIEnv*, jobject, jlong);

/*
 * Class:   plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:  DisposeC
 * Signature: (JL)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_DisposeC
  ( JNIEnv*, jobject, jlong);

/*
 * Class:   plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy
 * Method:  GetLastError
 * Signature: (JL)I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_Fonts_FreeTypeStrategy_GetLastError
  ( JNIEnv*, jobject);

#ifdef __cplusplus
}
#endif
#endif
