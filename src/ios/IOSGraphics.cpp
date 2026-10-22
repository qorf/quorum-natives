#ifdef __APPLE__
#include <TargetConditionals.h>

#ifdef TARGET_OS_IPHONE
#include "IOSGraphics.h"
//#include "gl2.h"
//#include "gl2ext.h"
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#include <stdio.h>

static jclass bufferClass;
static jclass byteBufferClass;
static jclass charBufferClass;
static jclass shortBufferClass;
static jclass intBufferClass;
static jclass longBufferClass;
static jclass floatBufferClass;
static jclass doubleBufferClass;
static jclass OOMEClass;
static jclass UOEClass;
static jclass IAEClass;

static jmethodID positionID;


static void
nativeClassInitBuffer(JNIEnv *_env)
{
    jclass bufferClassLocal = _env->FindClass("java/nio/Buffer");
    bufferClass = (jclass) _env->NewGlobalRef(bufferClassLocal);

    byteBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/ByteBuffer"));
    charBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/CharBuffer"));
    shortBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/ShortBuffer"));
    intBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/IntBuffer"));
    longBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/LongBuffer"));
    floatBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/FloatBuffer"));
    doubleBufferClass = (jclass) _env->NewGlobalRef(_env->FindClass("java/nio/DoubleBuffer"));

    positionID = _env->GetMethodID(bufferClass, "position","()I");
    if(positionID == 0) _env->ThrowNew(IAEClass, "Couldn't fetch position() method");
}

static void
nativeClassInit(JNIEnv *_env)
{
    nativeClassInitBuffer(_env);

    jclass IAEClassLocal =
        _env->FindClass("java/lang/IllegalArgumentException");
    jclass OOMEClassLocal =
         _env->FindClass("java/lang/OutOfMemoryError");
    jclass UOEClassLocal =
         _env->FindClass("java/lang/UnsupportedOperationException");

    IAEClass = (jclass) _env->NewGlobalRef(IAEClassLocal);
    OOMEClass = (jclass) _env->NewGlobalRef(OOMEClassLocal);
    UOEClass = (jclass) _env->NewGlobalRef(UOEClassLocal);
}

static jint getElementSizeShift(JNIEnv *_env, jobject buffer) {
	if(_env->IsInstanceOf(buffer, byteBufferClass)) return 0;
	if(_env->IsInstanceOf(buffer, floatBufferClass)) return 2;
	if(_env->IsInstanceOf(buffer, shortBufferClass)) return 1;

	if(_env->IsInstanceOf(buffer, charBufferClass)) return 1;
	if(_env->IsInstanceOf(buffer, intBufferClass)) return 2;
	if(_env->IsInstanceOf(buffer, longBufferClass)) return 3;
	if(_env->IsInstanceOf(buffer, doubleBufferClass)) return 3;

	_env->ThrowNew(IAEClass, "buffer type unknown! (Not a ByteBuffer, ShortBuffer, etc.)");
	return 0;
}

inline jint getBufferPosition(JNIEnv *env, jobject buffer)
{
	jint ret = env->CallIntMethodA(buffer, positionID, 0);
	return  ret;
}

static void *
getDirectBufferPointer(JNIEnv *_env, jobject buffer) {
    if (!buffer) {
        return NULL;
    }
    void* buf = _env->GetDirectBufferAddress(buffer);
    if (buf) {
        jint position = getBufferPosition(_env, buffer);
        jint elementSizeShift = getElementSizeShift(_env, buffer);
        buf = ((char*) buf) + (position << elementSizeShift);
    } else {
        _env->ThrowNew(IAEClass, "Must use a native order direct Buffer");
    }
    return buf;
}

static const char* getString( JNIEnv *env, jstring string )
{
	return (const char*)env->GetStringUTFChars(string, NULL);
}

static void releaseString( JNIEnv *env, jstring string, const char* cString )
{
	env->ReleaseStringUTFChars(string, cString);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_init
  (JNIEnv *env, jclass)
{
	nativeClassInit( env );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glActiveTexture
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glActiveTexture
  (JNIEnv *, jobject, jint texture)
{
	glActiveTexture( texture );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glAttachShader
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glAttachShader
  (JNIEnv *, jobject, jint program, jint shader)
{
	glAttachShader( program, shader );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBindAttribLocation
 * Signature: (IILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBindAttribLocation
  (JNIEnv *env, jobject, jint program, jint index, jstring name)
{
	const char* namePtr = getString( env, name );
	glBindAttribLocation( program, index, namePtr );
	releaseString( env, name, namePtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBindBuffer
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBindBuffer
  (JNIEnv *env, jobject, jint target, jint buffer)
{
	glBindBuffer( target, buffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBindFramebuffer
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBindFramebuffer
  (JNIEnv *env, jobject, jint target, jint framebuffer)
{
	glBindFramebuffer( target, framebuffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBindRenderbuffer
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBindRenderbuffer
  (JNIEnv *env, jobject, jint target, jint renderbuffer)
{
	glBindRenderbuffer( target, renderbuffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBindTexture
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBindTexture
  (JNIEnv *env, jobject, jint target, jint texture)
{
	glBindTexture( target, texture );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBlendColor
 * Signature: (FFFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBlendColor
  (JNIEnv *env, jobject, jfloat red, jfloat green, jfloat blue, jfloat alpha)
{
	glBlendColor( red, green, blue, alpha );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBlendEquation
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBlendEquation
  (JNIEnv *env, jobject, jint mode)
{
	glBlendEquation( mode );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBlendEquationSeparate
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBlendEquationSeparate
  (JNIEnv *env, jobject, jint modeRGB, jint modeAlpha)
{
	glBlendEquationSeparate( modeRGB, modeAlpha );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBlendFunc
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBlendFunc
  (JNIEnv *env, jobject, jint sfactor, jint dfactor)
{
	glBlendFunc( sfactor, dfactor );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBlendFuncSeparate
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBlendFuncSeparate
  (JNIEnv *env, jobject, jint srcRGB, jint dstRGB, jint srcAlpha, jint dstAlpha)
{
	glBlendFuncSeparate( srcRGB, dstRGB, srcAlpha, dstAlpha);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBufferData
 * Signature: (IILjava/nio/Buffer;I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBufferData
  (JNIEnv * env, jobject, jint target, jint size, jobject data, jint usage)
{
	void* dataPtr = getDirectBufferPointer( env, data );
	glBufferData( target, size, dataPtr, usage );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glBufferSubData
 * Signature: (IIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glBufferSubData
  (JNIEnv *env, jobject, jint target, jint offset, jint size, jobject data)
{
	void* dataPtr = getDirectBufferPointer( env, data );
	glBufferSubData( target, offset, size, dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCheckFramebufferStatus
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCheckFramebufferStatus
  (JNIEnv *env, jobject, jint target)
{
	return glCheckFramebufferStatus( target );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glClear
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glClear
  (JNIEnv *env, jobject, jint mask)
{
	glClear( mask );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glClearColor
 * Signature: (FFFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glClearColor
  (JNIEnv *env, jobject, jfloat red, jfloat green, jfloat blue, jfloat alpha)
{
	glClearColor( red, green, blue, alpha );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glClearDepthf
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glClearDepthf
  (JNIEnv *env, jobject, jfloat depth)
{
	glClearDepthf( depth );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glClearStencil
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glClearStencil
  (JNIEnv *env, jobject, jint s)
{
	glClearStencil( s );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glColorMask
 * Signature: (ZZZZ)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glColorMask
  (JNIEnv *env, jobject, jboolean red, jboolean green, jboolean blue, jboolean alpha)
{
	glColorMask( red, green, blue, alpha );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCompileShader
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCompileShader
  (JNIEnv *env, jobject, jint shader)
{
	glCompileShader( shader );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCompressedTexImage2D
 * Signature: (IIIIIIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCompressedTexImage2D
  (JNIEnv *env, jobject, jint target, jint level, jint internalFormat, jint width, jint height, jint border, jint imageSize, jobject data)
{
	void* dataPtr = getDirectBufferPointer( env, data );
	glCompressedTexImage2D( target, level, internalFormat, width, height, border, imageSize, dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCompressedTexSubImage2D
 * Signature: (IIIIIIIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCompressedTexSubImage2D
  (JNIEnv *env, jobject, jint target, jint level, jint xoffset, jint yoffset, jint width, jint height, jint format, jint imageSize, jobject data)
{
	void* dataPtr = getDirectBufferPointer( env, data );
	glCompressedTexSubImage2D( target, level, xoffset, yoffset, width, height, format, imageSize, dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCopyTexImage2D
 * Signature: (IIIIIIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCopyTexImage2D
  (JNIEnv *env, jobject, jint target, jint level, jint  internalFormat, jint x, jint y, jint width, jint height, jint border)
{
	glCopyTexImage2D( target, level, internalFormat, x, y, width, height, border );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCopyTexSubImage2D
 * Signature: (IIIIIIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCopyTexSubImage2D
  (JNIEnv *env, jobject, jint target, jint level, jint xoffset, jint yoffset, jint x, jint y, jint width, jint height)
{
	glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCreateProgram
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCreateProgram
  (JNIEnv *env, jobject)
{
	return glCreateProgram( );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCreateShader
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCreateShader
  (JNIEnv *env, jobject, jint type)
{
	return glCreateShader( type );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glCullFace
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glCullFace
  (JNIEnv *env, jobject, jint mode)
{
	glCullFace( mode );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteBuffers
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteBuffers
  (JNIEnv *env, jobject, jint n, jobject buffers)
{
	void* dataPtr = getDirectBufferPointer( env, buffers );
	glDeleteBuffers( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteBuffer
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteBuffer
  (JNIEnv *, jobject, jint buffer)
{
	GLuint b = buffer;
	glDeleteBuffers (1, &b);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteFramebuffers
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteFramebuffers
  (JNIEnv *env, jobject, jint n, jobject framebuffers)
{
	void* dataPtr = getDirectBufferPointer( env, framebuffers );
	glDeleteFramebuffers( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteFramebuffer
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteFramebuffer
  (JNIEnv *, jobject, jint buffer)
{
	GLuint b = buffer;
	glDeleteFramebuffers(1, &b);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteProgram
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteProgram
  (JNIEnv *env, jobject, jint program)
{
	glDeleteProgram( program );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteRenderbuffers
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteRenderbuffers
  (JNIEnv *env, jobject, jint n, jobject renderbuffers)
{
	void* dataPtr = getDirectBufferPointer( env, renderbuffers );
	glDeleteRenderbuffers( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteRenderbuffer
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteRenderbuffer
  (JNIEnv *, jobject, jint buffer)
{
	GLuint b = buffer;
	glDeleteRenderbuffers( 1, &b );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteShader
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteShader
  (JNIEnv *env, jobject, jint shader)
{
	glDeleteShader( shader );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteTextures
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteTextures
  (JNIEnv *env, jobject, jint n, jobject textures)
{
	void* dataPtr = getDirectBufferPointer( env, textures );
	glDeleteTextures( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDeleteTexture
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDeleteTexture
  (JNIEnv *, jobject, jint buffer)
{
	GLuint b = buffer;
	glDeleteTextures(1, &b);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDepthFunc
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDepthFunc
  (JNIEnv *env, jobject, jint func)
{
	glDepthFunc( func );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDepthMask
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDepthMask
  (JNIEnv *env, jobject, jboolean flag)
{
	glDepthMask( flag );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDepthRangef
 * Signature: (FF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDepthRangef
  (JNIEnv *env, jobject, jfloat zNear, jfloat zFar)
{
	glDepthRangef( zNear, zFar );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDetachShader
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDetachShader
  (JNIEnv *env, jobject, jint program, jint shader)
{
	glDetachShader( program, shader );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDisable
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDisable
  (JNIEnv *env, jobject, jint cap)
{
	glDisable( cap );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDisableVertexAttribArray
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDisableVertexAttribArray
  (JNIEnv *env, jobject, jint index)
{
	glDisableVertexAttribArray( index );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDrawArrays
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDrawArrays
  (JNIEnv *env, jobject, jint mode, jint first, jint count)
{
	glDrawArrays( mode, first, count );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glDrawElements
 * Signature: (IIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDrawElements__IIILjava_nio_Buffer_2
(JNIEnv *env, jobject, jint mode, jint count, jint type, jobject indices)
{
	void* dataPtr = getDirectBufferPointer( env, indices );
	//__android_log_print(ANDROID_LOG_INFO, "GL2", "drawelements");
	glDrawElements( mode, count, type, dataPtr );
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glDrawElements__IIII
  (JNIEnv *, jobject, jint mode, jint count, jint type, jint indices)
{
	glDrawElements( mode, count, type, (const void*)indices );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glEnable
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glEnable
  (JNIEnv *env, jobject, jint cap)
{
	glEnable( cap );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glEnableVertexAttribArray
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glEnableVertexAttribArray
  (JNIEnv *env, jobject, jint index)
{
	glEnableVertexAttribArray( index );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glFinish
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glFinish
  (JNIEnv *env, jobject)
{
	glFinish();
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glFlush
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glFlush
  (JNIEnv *env, jobject)
{
	glFlush();
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glFramebufferRenderbuffer
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glFramebufferRenderbuffer
  (JNIEnv *env, jobject, jint target, jint attachment, jint renderbuffertarget, jint renderbuffer)
{
	glFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glFramebufferTexture2D
 * Signature: (IIIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glFramebufferTexture2D
  (JNIEnv *env, jobject, jint target, jint attachment, jint textarget, jint texture, jint level)
{
	glFramebufferTexture2D( target, attachment, textarget, texture, level );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glFrontFace
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glFrontFace
  (JNIEnv *env, jobject, jint mode)
{ //XXXX
	glFrontFace( mode );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenBuffers
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenBuffers
  (JNIEnv *env, jobject, jint n, jobject buffers)
{
	void* dataPtr = getDirectBufferPointer( env, buffers );
	glGenBuffers( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenBuffer
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenBuffer
  (JNIEnv *, jobject)
{
	GLuint result;
	glGenBuffers( 1, &result );
	return result;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenerateMipmap
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenerateMipmap
  (JNIEnv *env, jobject, jint target)
{
	glGenerateMipmap( target );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenFramebuffers
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenFramebuffers
  (JNIEnv *env, jobject, jint n, jobject framebuffers)
{
	void* dataPtr = getDirectBufferPointer( env, framebuffers );
	glGenFramebuffers( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenFramebuffer
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenFramebuffer
  (JNIEnv *, jobject)
{
	GLuint result;
	glGenFramebuffers( 1, &result );
	return result;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenRenderbuffers
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenRenderbuffers
  (JNIEnv *env, jobject, jint n, jobject renderbuffers)
{
	void* dataPtr = getDirectBufferPointer( env, renderbuffers );
	glGenRenderbuffers( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenRenderbuffer
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenRenderbuffer
  (JNIEnv *, jobject)
{
	GLuint result;
	glGenRenderbuffers( 1, &result );
	return result;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenTextures
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenTextures
  (JNIEnv *env, jobject, jint n, jobject textures)
{
	void* dataPtr = getDirectBufferPointer( env, textures );
	glGenTextures( n, (GLuint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGenTexture
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGenTexture
  (JNIEnv *, jobject)
{
	GLuint result;
	glGenTextures(1, &result);
	return result;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetActiveAttrib
 * Signature: (IIILjava/nio/Buffer;Ljava/nio/IntBuffer;Ljava/nio/Buffer;Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetActiveAttrib
  (JNIEnv *env, jobject, jint program, jint index, jobject size, jobject type )
{
	// FIXME is this wrong?
	char cname[2048];
	void* sizePtr = getDirectBufferPointer( env, size );
	void* typePtr = getDirectBufferPointer( env, type );
	glGetActiveAttrib( program, index, 2048, NULL, (GLint*)sizePtr, (GLenum*)typePtr, cname );

	return env->NewStringUTF( cname );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetActiveUniform
 * Signature: (IIILjava/nio/Buffer;Ljava/nio/IntBuffer;Ljava/nio/Buffer;Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetActiveUniform
  (JNIEnv *env, jobject, jint program, jint index, jobject size, jobject type)
{
	// FIXME is this wrong?
	char cname[2048];
	void* sizePtr = getDirectBufferPointer( env, size );
	void* typePtr = getDirectBufferPointer( env, type );
	glGetActiveUniform( program, index, 2048, NULL, (GLint*)sizePtr, (GLenum*)typePtr, cname );

	return env->NewStringUTF( cname );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetAttachedShaders
 * Signature: (IILjava/nio/Buffer;Ljava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetAttachedShaders
  (JNIEnv *env, jobject, jint program, jint maxcount, jobject count, jobject shaders)
{
	void* countPtr = getDirectBufferPointer( env, count );
	void* shaderPtr = getDirectBufferPointer( env, shaders );
	glGetAttachedShaders( program, maxcount, (GLsizei*)countPtr, (GLuint*)shaderPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetAttribLocation
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetAttribLocation
  (JNIEnv *env, jobject, jint program, jstring name)
{
	const char* cname = getString( env, name );
	int loc = glGetAttribLocation( program, cname );
	releaseString( env, name, cname );
	return loc;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetBooleanv
 * Signature: (ILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetBooleanv
  (JNIEnv *env, jobject, jint program, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetBooleanv( program, (GLboolean*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetBufferParameteriv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetBufferParameteriv
  (JNIEnv *env, jobject, jint target, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetBufferParameteriv( target, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetError
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetError
  (JNIEnv *env, jobject)
{
	return glGetError();
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetFloatv
 * Signature: (ILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetFloatv
  (JNIEnv *env, jobject, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetFloatv( pname, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetFramebufferAttachmentParameteriv
 * Signature: (IIILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetFramebufferAttachmentParameteriv
  (JNIEnv *env, jobject, jint target, jint attachment, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetFramebufferAttachmentParameteriv( target, attachment, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetIntegerv
 * Signature: (ILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetIntegerv
  (JNIEnv *env, jobject, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetIntegerv( pname, (GLint*)dataPtr);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetProgramiv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetProgramiv
  (JNIEnv *env, jobject, jint program, jint pname, jobject params)
{
	void *dataPtr = getDirectBufferPointer( env, params );
	glGetProgramiv( program, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetProgramInfoLog
 * Signature: (IILjava/nio/Buffer;Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetProgramInfoLog
  (JNIEnv *env, jobject, jint program )
{
	char info[1024*10]; // FIXME 10k limit should suffice
	int length = 0;
	glGetProgramInfoLog( program, 1024*10, &length, info );
	return env->NewStringUTF( info );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetRenderbufferParameteriv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetRenderbufferParameteriv
  (JNIEnv *env, jobject, jint target, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetRenderbufferParameteriv( target, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetShaderiv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetShaderiv
  (JNIEnv *env, jobject, jint shader, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetShaderiv( shader, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetShaderInfoLog
 * Signature: (IILjava/nio/Buffer;Ljava/lang/String;)V
 */
JNIEXPORT jstring JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetShaderInfoLog
  (JNIEnv *env, jobject, jint shader )
{
	char info[1024*10]; // FIXME 10k limit should suffice
	int length = 0;
	glGetShaderInfoLog( shader, 1024*10, &length, info );
	return env->NewStringUTF( info );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetShaderPrecisionFormat
 * Signature: (IILjava/nio/IntBuffer;Ljava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetShaderPrecisionFormat
  (JNIEnv *env, jobject, jint shadertype, jint precisiontype, jobject range, jobject precision)
{
	void* rangePtr = getDirectBufferPointer( env, range );
	void* precisionPtr = getDirectBufferPointer( env, precision );
	glGetShaderPrecisionFormat( shadertype, precisiontype, (GLint*)rangePtr, (GLint*)precisionPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetShaderSource
 * Signature: (IILjava/nio/Buffer;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetShaderSource
  (JNIEnv *env, jobject, jint shader, jint bufsize, jobject length, jstring source)
{
	env->ThrowNew(UOEClass, "This method is not supported"); // FIXME won't implement this shit.
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetString
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetString
  (JNIEnv *env, jobject, jint name)
{
	const char * chars = (const char *)glGetString((GLenum)name);
	jstring output = env->NewStringUTF(chars);
	return output;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetTexParameterfv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetTexParameterfv
  (JNIEnv *env, jobject, jint target, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetTexParameterfv( target, pname, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetTexParameteriv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetTexParameteriv
  (JNIEnv *env, jobject, jint target, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetTexParameteriv( target, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetUniformfv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetUniformfv
  (JNIEnv *env, jobject, jint program, jint location, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetUniformfv( program, location, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetUniformiv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetUniformiv
  (JNIEnv *env, jobject, jint program, jint location, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetUniformiv( program, location, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetUniformLocation
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetUniformLocation
  (JNIEnv *env, jobject, jint program, jstring name)
{
	const char* cname = getString( env, name );
	int location = glGetUniformLocation( program, cname );
	releaseString( env, name, cname );
	return location;
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetVertexAttribfv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetVertexAttribfv
  (JNIEnv *env, jobject, jint index, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetVertexAttribfv( index, pname, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetVertexAttribiv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetVertexAttribiv
  (JNIEnv *env, jobject, jint index, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glGetVertexAttribiv( index, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glGetVertexAttribPointerv
 * Signature: (IILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glGetVertexAttribPointerv
  (JNIEnv *env, jobject, jint index, jint pname, jobject pointer)
{
	env->ThrowNew(UOEClass, "This method is not supported"); // FIXME won't implement this shit
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glHint
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glHint
  (JNIEnv *env, jobject, jint target, jint mode)
{
	glHint( target, mode );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsBuffer
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsBuffer
  (JNIEnv *env, jobject, jint buffer)
{
	return glIsBuffer( buffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsEnabled
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsEnabled
  (JNIEnv *env, jobject, jint cap)
{
	return glIsEnabled( cap );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsFramebuffer
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsFramebuffer
  (JNIEnv *env, jobject, jint framebuffer)
{
	return glIsFramebuffer( framebuffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsProgram
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsProgram
  (JNIEnv *env, jobject, jint program)
{
	return glIsProgram( program );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsRenderbuffer
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsRenderbuffer
  (JNIEnv *env, jobject, jint renderbuffer)
{
	return glIsRenderbuffer( renderbuffer );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsShader
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsShader
  (JNIEnv *env, jobject, jint shader)
{
	return glIsShader( shader );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glIsTexture
 * Signature: (I)C
 */
JNIEXPORT jboolean JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glIsTexture
  (JNIEnv *env, jobject, jint texture)
{
	return glIsTexture( texture );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glLineWidth
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glLineWidth
  (JNIEnv *env, jobject, jfloat width)
{
	glLineWidth( width );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glLinkProgram
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glLinkProgram
  (JNIEnv *env, jobject, jint program)
{
	glLinkProgram( program );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glPixelStorei
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glPixelStorei
  (JNIEnv *env, jobject, jint pname, jint param)
{
	glPixelStorei( pname, param );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glPolygonOffset
 * Signature: (FF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glPolygonOffset
  (JNIEnv *env, jobject, jfloat factor, jfloat units)
{
	glPolygonOffset( factor, units );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glReadPixels
 * Signature: (IIIIIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glReadPixels
  (JNIEnv *env, jobject, jint x, jint y, jint width, jint height, jint format, jint type, jobject pixels)
{
	void* dataPtr = getDirectBufferPointer( env, pixels );
	glReadPixels( x, y, width, height, format, type, dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glReleaseShaderCompiler
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glReleaseShaderCompiler
  (JNIEnv *env, jobject)
{
	glReleaseShaderCompiler();
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glRenderbufferStorage
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glRenderbufferStorage
  (JNIEnv *env, jobject, jint target, jint internalFormat, jint width, jint height)
{
	glRenderbufferStorage( target, internalFormat, width, height );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glSampleCoverage
 * Signature: (FZ)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glSampleCoverage
  (JNIEnv *env, jobject, jfloat value, jboolean inver)
{
	glSampleCoverage( value, inver );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glScissor
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glScissor
  (JNIEnv *env, jobject, jint x, jint y, jint width, jint height)
{
	glScissor( x, y, width, height );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glShaderBinary
 * Signature: (ILjava/nio/IntBuffer;ILjava/nio/Buffer;I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glShaderBinary
  (JNIEnv *env, jobject, jint n, jobject shaders, jint binaryformat, jobject binary, jint length)
{
	void* shaderPtr = getDirectBufferPointer( env, shaders );
	void* binaryPtr = getDirectBufferPointer( env, binary );
	glShaderBinary( n, (GLuint*)shaderPtr, binaryformat, binaryPtr, length );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glShaderSource
 * Signature: (IILjava/lang/String;Ljava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glShaderSource
  (JNIEnv *env, jobject, jint shader, jstring string )
{
	const char* cstring = getString( env, string );
	glShaderSource( shader, 1, &cstring, NULL );
	releaseString( env, string, cstring );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glStencilFunc
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glStencilFunc
  (JNIEnv *env, jobject, jint func, jint ref, jint mask)
{
	glStencilFunc( func, ref, mask );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glStencilFuncSeparate
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glStencilFuncSeparate
  (JNIEnv *env, jobject, jint face, jint func, jint ref, jint mask)
{
	glStencilFuncSeparate( face, func, ref, mask );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glStencilMask
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glStencilMask
  (JNIEnv *env, jobject, jint mask)
{
	glStencilMask( mask );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glStencilMaskSeparate
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glStencilMaskSeparate
  (JNIEnv *env, jobject, jint face, jint mask)
{
	glStencilMaskSeparate( face, mask );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glStencilOp
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glStencilOp
  (JNIEnv *env, jobject, jint fail, jint zFail, jint zpass)
{
	glStencilOp( fail, zFail, zpass );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glStencilOpSeparate
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glStencilOpSeparate
  (JNIEnv *env, jobject, jint face, jint fail, jint zFail, jint zPass)
{
	glStencilOpSeparate( face, fail, zFail, zPass );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glTexImage2D
 * Signature: (IIIIIIIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glTexImage2D
  (JNIEnv *env, jobject, jint target, jint level, jint internalformat, jint width, jint height, jint border, jint format, jint type, jobject pixels)
{
	void* dataPtr = getDirectBufferPointer( env, pixels );
	glTexImage2D( target, level, internalformat, width, height, border, format, type, dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glTexParameterf
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glTexParameterf
  (JNIEnv *env, jobject, jint target, jint pname, jfloat param)
{
	glTexParameterf( target, pname, param );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glTexParameterfv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glTexParameterfv
  (JNIEnv *env, jobject, jint target, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glTexParameterfv( target, pname, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glTexParameteri
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glTexParameteri
  (JNIEnv *env, jobject, jint target, jint pname, jint param)
{
	glTexParameteri( target, pname, param );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glTexParameteriv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glTexParameteriv
  (JNIEnv *env, jobject, jint target, jint pname, jobject params)
{
	void* dataPtr = getDirectBufferPointer( env, params );
	glTexParameteriv( target, pname, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glTexSubImage2D
 * Signature: (IIIIIIIILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glTexSubImage2D
  (JNIEnv *env, jobject, jint target, jint level, jint xoffset, jint yoffset, jint width, jint height, jint format, jint type, jobject pixels)
{
	void* dataPtr = getDirectBufferPointer( env, pixels );
	glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform1f
 * Signature: (IF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform1f
  (JNIEnv *env, jobject, jint location, jfloat x)
{
	glUniform1f( location, x );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform1fv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform1fv__IILjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform1fv( location, count, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform1fv
 * Signature: (II[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform1fv__II_3FI
  (JNIEnv *env, jobject, jint location, jint count, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform1fv( location, count, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform1i
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform1i
  (JNIEnv *env, jobject, jint location, jint x)
{
	glUniform1i( location, x );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform1iv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform1iv__IILjava_nio_IntBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform1iv( location, count, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform1iv
 * Signature: (II[II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform1iv__II_3II
	(JNIEnv *env, jobject, jint location, jint count, jintArray value, jint offset)
{
	int* v = (int*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform1iv( location, count, (GLint*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform2f
 * Signature: (IFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform2f
  (JNIEnv *env, jobject, jint location, jfloat x, jfloat y)
{
	glUniform2f( location, x, y );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform2fv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform2fv__IILjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform2fv( location, count, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform2fv
 * Signature: (II[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform2fv__II_3FI
	(JNIEnv *env, jobject, jint location, jint count, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform2fv( location, count, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform2i
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform2i
  (JNIEnv *env, jobject, jint location, jint x, jint y)
{
	glUniform2i( location, x, y );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform2iv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform2iv__IILjava_nio_IntBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform2iv( location, count, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform2iv
 * Signature: (II[II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform2iv__II_3II
	(JNIEnv *env, jobject, jint location, jint count, jintArray value, jint offset)
{
	int* v = (int*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform2iv( location, count, (GLint*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform3f
 * Signature: (IFFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform3f
  (JNIEnv *env, jobject, jint location, jfloat x, jfloat y, jfloat z)
{
	glUniform3f( location, x, y, z );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform3fv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform3fv__IILjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform3fv( location, count, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform3fv
 * Signature: (II[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform3fv__II_3FI
	(JNIEnv *env, jobject, jint location, jint count, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform3fv( location, count, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform3i
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform3i
  (JNIEnv *env, jobject, jint location, jint x, jint y, jint z)
{
	glUniform3i( location, x, y, z );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform3iv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform3iv__IILjava_nio_IntBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform3iv( location, count, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform3iv
 * Signature: (II[II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform3iv__II_3II
	(JNIEnv *env, jobject, jint location, jint count, jintArray value, jint offset)
{
	int* v = (int*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform3iv( location, count, (GLint*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform4f
 * Signature: (IFFFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform4f
  (JNIEnv *env, jobject, jint location, jfloat x, jfloat y, jfloat z, jfloat w)
{
	glUniform4f( location, x, y, z, w );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform4fv
 * Signature: (IILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform4fv__IILjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform4fv( location, count, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform4fv
 * Signature: (II[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform4fv__II_3FI
	(JNIEnv *env, jobject, jint location, jint count, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform4fv( location, count, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform4i
 * Signature: (IIIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform4i
  (JNIEnv *env, jobject, jint location, jint x, jint y, jint z, jint w)
{
	glUniform4i( location, x, y, z, w );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform4iv
 * Signature: (IILjava/nio/IntBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform4iv__IILjava_nio_IntBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jobject v)
{
	void* dataPtr = getDirectBufferPointer( env, v );
	glUniform4iv( location, count, (GLint*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniform4iv
 * Signature: (II[II)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniform4iv__II_3II
	(JNIEnv *env, jobject, jint location, jint count, jintArray value, jint offset)
{
	int* v = (int*)env->GetPrimitiveArrayCritical(value, 0);
	glUniform4iv( location, count, (GLint*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniformMatrix2fv
 * Signature: (IIZLjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniformMatrix2fv__IIZLjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jboolean transpose, jobject value)
{
	void* dataPtr = getDirectBufferPointer( env, value );
	glUniformMatrix2fv( location, count, transpose, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniformMatrix2fv
 * Signature: (IIZ[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniformMatrix2fv__IIZ_3FI
  (JNIEnv *env, jobject, jint location, jint count, jboolean transpose, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniformMatrix2fv( location, count, transpose, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniformMatrix3fv
 * Signature: (IIZLjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniformMatrix3fv__IIZLjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jboolean transpose, jobject value)
{
	void* dataPtr = getDirectBufferPointer( env, value );
	glUniformMatrix3fv( location, count, transpose, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniformMatrix3fv
 * Signature: (IIZ[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniformMatrix3fv__IIZ_3FI
	(JNIEnv *env, jobject, jint location, jint count, jboolean transpose, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniformMatrix3fv( location, count, transpose, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniformMatrix4fv
 * Signature: (IIZLjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniformMatrix4fv__IIZLjava_nio_FloatBuffer_2
  (JNIEnv *env, jobject, jint location, jint count, jboolean transpose, jobject value)
{
	void* dataPtr = getDirectBufferPointer( env, value );
	glUniformMatrix4fv( location, count, transpose, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUniformMatrix4fv
 * Signature: (IIZ[FI)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUniformMatrix4fv__IIZ_3FI
	(JNIEnv *env, jobject, jint location, jint count, jboolean transpose, jfloatArray value, jint offset)
{
	float* v = (float*)env->GetPrimitiveArrayCritical(value, 0);
	glUniformMatrix4fv( location, count, transpose, (GLfloat*)&v[offset] );
	env->ReleasePrimitiveArrayCritical(value, v, 0);
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glUseProgram
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glUseProgram
  (JNIEnv *env, jobject, jint program)
{
	glUseProgram( program );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glValidateProgram
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glValidateProgram
  (JNIEnv *env, jobject, jint program)
{
	glValidateProgram( program );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib1f
 * Signature: (IF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib1f
  (JNIEnv *env, jobject, jint indx, jfloat x)
{
	glVertexAttrib1f( indx, x );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib1fv
 * Signature: (ILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib1fv
  (JNIEnv *env, jobject, jint indx, jobject values)
{
	void* dataPtr = getDirectBufferPointer( env, values );
	glVertexAttrib1fv( indx, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib2f
 * Signature: (IFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib2f
  (JNIEnv *env, jobject, jint indx, jfloat x, jfloat y)
{
	glVertexAttrib2f( indx, x, y );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib2fv
 * Signature: (ILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib2fv
  (JNIEnv *env, jobject, jint indx, jobject values)
{
	void* dataPtr = getDirectBufferPointer( env, values );
	glVertexAttrib2fv( indx, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib3f
 * Signature: (IFFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib3f
  (JNIEnv *env, jobject, jint indx, jfloat x, jfloat y, jfloat z)
{
	glVertexAttrib3f( indx, x, y, z );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib3fv
 * Signature: (ILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib3fv
  (JNIEnv *env, jobject, jint indx, jobject values)
{
	void* dataPtr = getDirectBufferPointer( env, values );
	glVertexAttrib3fv( indx, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib4f
 * Signature: (IFFFF)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib4f
  (JNIEnv *env, jobject, jint indx, jfloat x, jfloat y, jfloat z, jfloat w)
{
	glVertexAttrib4f( indx, x, y, z, w );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttrib4fv
 * Signature: (ILjava/nio/FloatBuffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttrib4fv
  (JNIEnv *env, jobject, jint indx, jobject values)
{
	void* dataPtr = getDirectBufferPointer( env, values );
	glVertexAttrib4fv( indx, (GLfloat*)dataPtr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glVertexAttribPointer
 * Signature: (IIIZILjava/nio/Buffer;)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttribPointer__IIIZILjava_nio_Buffer_2
  (JNIEnv *env, jobject, jint indx, jint size, jint type, jboolean normalized, jint stride, jobject ptr)
{
	void* dataPtr = getDirectBufferPointer( env, ptr );
	glVertexAttribPointer( indx, size, type, normalized, stride, dataPtr );
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glVertexAttribPointer__IIIZII
  (JNIEnv *, jobject, jint indx, jint size, jint type, jboolean normalized, jint stride, jint ptr)
{
	glVertexAttribPointer( indx, size, type, normalized, stride, (const void*)ptr );
}

/*
 * Class:     plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL
 * Method:    glViewportJni
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_glViewportJni
  (JNIEnv *env, jobject, jint x, jint y, jint width, jint height)
{
	glViewport( x, y, width, height );
}

JNIEXPORT int JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_CreateVertexArray
(JNIEnv *env, jobject obj) {
    GLuint buffer = 0;
    glGenVertexArrays(1, &buffer);
    return buffer;
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_DeleteVertexArray
  (JNIEnv *env, jobject obj, jint vao)
{
    GLuint array = vao;
    glDeleteVertexArrays(1, &array);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Game_Graphics_OpenGL_IOSOpenGL_BindVertexArray
  (JNIEnv *env, jobject obj, jint array)
{
    glBindVertexArray(array);
}

#endif
#endif
