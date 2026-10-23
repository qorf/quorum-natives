/*
    Text to speech on the Mac and iOS, for Quorum's Speech class and say
    statements. Called from Java through JNI by
    org.sodbeans.phonemic.tts.impl.AppleSpeak, which sits under SpeechBridge
    (it handles priorities and the speech queue).

    It uses AVSpeechSynthesizer, which is on macOS 10.14 and later and on iOS.
    It replaces CocoaSpeechServer, a separate helper app that Quorum talked
    to over a socket.

    AVSpeechSynthesizer tells its delegate when speech finishes, but on the
    main thread. A Quorum game on a Mac runs Java's main thread as the main
    thread, and a blocking say would then wait forever for a message that
    can't arrive. So no delegate is used: speaking and waiting are done by
    polling isSpeaking, which works on any thread.

    Compiled with ARC (see CMakeLists.txt).
*/

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include <jni.h>
#include <unistd.h>

// How often a blocking speak checks whether speech is still going.
#define POLL_MICROSECONDS 20000
// How long to wait for speech to start, or for stopped speech to end,
// before going on anyway. Both normally take about 10 milliseconds.
#define START_LIMIT_SECONDS 1.0

static AVSpeechSynthesizer* synthesizer = nil;
static NSObject* lock = nil;

static AVSpeechSynthesizer* GetSynthesizer(void)
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        lock = [[NSObject alloc] init];
        synthesizer = [[AVSpeechSynthesizer alloc] init];
    });
    return synthesizer;
}

static NSString* ToNSString(JNIEnv* env, jstring value)
{
    if (value == NULL)
        return @"";

    // Java strings are UTF-16, so copy the characters directly rather than
    // through JNI's modified UTF-8, which mangles emoji and other characters
    // outside the basic plane.
    jsize length = (*env)->GetStringLength(env, value);
    const jchar* characters = (*env)->GetStringChars(env, value, NULL);
    if (characters == NULL)
        return @"";

    NSString* result = [[NSString alloc] initWithCharacters:(const unichar*)characters length:(NSUInteger)length];
    (*env)->ReleaseStringChars(env, value, characters);
    return result;
}

/* Waits while isSpeaking matches the given value, up to the limit in seconds. A limit of 0 means no limit. */
static void WaitWhileSpeakingIs(BOOL speaking, double limit)
{
    NSDate* start = [NSDate date];
    while (synthesizer.isSpeaking == speaking)
    {
        if (limit > 0 && -[start timeIntervalSinceNow] > limit)
            return;
        usleep(POLL_MICROSECONDS);
    }
}

JNIEXPORT void JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_initializeNative(JNIEnv* env, jobject object)
{
    @autoreleasepool
    {
        GetSynthesizer();
    }
}

/*
    Speaks the text, stopping anything already being spoken, as SpeechBridge
    expects. The voice is a voice identifier, or empty for the system's
    default voice. Rate, pitch and volume are AVSpeechUtterance's own values.
    When block is true, returns only once the text has been spoken or stopped.
*/
JNIEXPORT jboolean JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_speakNative(JNIEnv* env, jobject object,
    jstring text, jstring voice, jfloat rate, jfloat pitch, jfloat volume, jboolean block)
{
    @autoreleasepool
    {
        AVSpeechSynthesizer* speech = GetSynthesizer();
        NSString* words = ToNSString(env, text);
        if ([words length] == 0)
            return JNI_FALSE;

        AVSpeechUtterance* utterance = [AVSpeechUtterance speechUtteranceWithString:words];
        utterance.rate = rate;
        utterance.pitchMultiplier = pitch;
        utterance.volume = volume;

        NSString* identifier = ToNSString(env, voice);
        if ([identifier length] > 0)
        {
            AVSpeechSynthesisVoice* chosen = [AVSpeechSynthesisVoice voiceWithIdentifier:identifier];
            if (chosen != nil)
                utterance.voice = chosen;
        }

        @synchronized (lock)
        {
            if (speech.isSpeaking)
            {
                [speech stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
                WaitWhileSpeakingIs(YES, START_LIMIT_SECONDS);
            }
            [speech speakUtterance:utterance];
        }

        if (block)
        {
            // Wait for speech to start, so a blocking call can't return
            // before it begins, then for it to end. The lock isn't held
            // here, so another thread can still stop the speech.
            WaitWhileSpeakingIs(NO, START_LIMIT_SECONDS);
            WaitWhileSpeakingIs(YES, 0);
        }
        return JNI_TRUE;
    }
}

JNIEXPORT jboolean JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_stopNative(JNIEnv* env, jobject object)
{
    @autoreleasepool
    {
        AVSpeechSynthesizer* speech = GetSynthesizer();
        @synchronized (lock)
        {
            if (!speech.isSpeaking)
                return JNI_TRUE;
            return [speech stopSpeakingAtBoundary:AVSpeechBoundaryImmediate] ? JNI_TRUE : JNI_FALSE;
        }
    }
}

JNIEXPORT jboolean JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_pauseNative(JNIEnv* env, jobject object)
{
    @autoreleasepool
    {
        AVSpeechSynthesizer* speech = GetSynthesizer();
        @synchronized (lock)
        {
            return [speech pauseSpeakingAtBoundary:AVSpeechBoundaryImmediate] ? JNI_TRUE : JNI_FALSE;
        }
    }
}

JNIEXPORT jboolean JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_resumeNative(JNIEnv* env, jobject object)
{
    @autoreleasepool
    {
        AVSpeechSynthesizer* speech = GetSynthesizer();
        @synchronized (lock)
        {
            return [speech continueSpeaking] ? JNI_TRUE : JNI_FALSE;
        }
    }
}

JNIEXPORT jboolean JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_isSpeakingNative(JNIEnv* env, jobject object)
{
    @autoreleasepool
    {
        return GetSynthesizer().isSpeaking ? JNI_TRUE : JNI_FALSE;
    }
}

/*
    The installed voices, one string each: the identifier, the name and the
    language, separated by tabs. Voices in the system's language come first.
*/
JNIEXPORT jobjectArray JNICALL Java_org_sodbeans_phonemic_tts_impl_AppleSpeak_getVoicesNative(JNIEnv* env, jobject object)
{
    @autoreleasepool
    {
        NSArray<AVSpeechSynthesisVoice*>* voices = [AVSpeechSynthesisVoice speechVoices];
        NSString* current = [AVSpeechSynthesisVoice currentLanguageCode];

        NSMutableArray<NSString*>* first = [NSMutableArray array];
        NSMutableArray<NSString*>* rest = [NSMutableArray array];
        for (AVSpeechSynthesisVoice* voice in voices)
        {
            NSString* entry = [NSString stringWithFormat:@"%@\t%@\t%@", voice.identifier, voice.name, voice.language];
            if (current != nil && [voice.language isEqualToString:current])
                [first addObject:entry];
            else
                [rest addObject:entry];
        }
        [first addObjectsFromArray:rest];

        jclass stringClass = (*env)->FindClass(env, "java/lang/String");
        jobjectArray result = (*env)->NewObjectArray(env, (jsize)[first count], stringClass, NULL);
        if (result == NULL)
            return NULL;

        for (NSUInteger i = 0; i < [first count]; i++)
        {
            jstring value = (*env)->NewStringUTF(env, [[first objectAtIndex:i] UTF8String]);
            (*env)->SetObjectArrayElement(env, result, (jsize)i, value);
            (*env)->DeleteLocalRef(env, value);
        }
        return result;
    }
}
