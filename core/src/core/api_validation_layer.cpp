//
// Copyright 2017-2023 Valve Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Minimal build: only Context, HRTF, PanningEffect, BinauralEffect, AudioBuffer are validated.
// All other API surfaces delegate directly to CContext stubs (which return IPL_STATUS_FAILURE).

#include "phonon.h"

#define STEAMAUDIO_SKIP_API_FUNCTIONS
#include "phonon_interfaces.h"

#include "api_context.h"
#include "api_hrtf.h"
#include "api_panning_effect.h"
#include "api_binaural_effect.h"

namespace api {

// --------------------------------------------------------------------------------------------------------------------
// API Object Helpers
// --------------------------------------------------------------------------------------------------------------------

template <typename T, typename F, typename I, typename... Args>
IPLerror apiObjectAllocate(I** object, F* factory, Args&&... args)
{
    try
    {
        auto _object = reinterpret_cast<T*>(gMemory().allocate(sizeof(T), Memory::kDefaultAlignment));
        new (_object) T(factory, std::forward<Args>(args)...);
        *object = static_cast<I*>(_object);
    }
    catch (Exception exception)
    {
        return static_cast<IPLerror>(exception.status());
    }

    return IPL_STATUS_SUCCESS;
}


// --------------------------------------------------------------------------------------------------------------------
// Validation Helpers
// --------------------------------------------------------------------------------------------------------------------

template <typename T>
std::string to_string(T value)
{
    return std::to_string(value);
}

template <typename T>
std::string to_string(T* value)
{
    char result[32] = {0};
    snprintf(result, 32, "%p", value);
    return std::string(result);
}

#define VALIDATE(type, value, test) { \
    if (!(test)) { \
        gLog().message(MessageSeverity::Warning, "%s: invalid %s: %s = %s", __func__, #type, #value, to_string(value).c_str()); \
    } \
}

#define VALIDATE_IPLfloat32(value) { \
    VALIDATE(IPLfloat32, value, Math::isFinite(value)); \
}

#define VALIDATE_POINTER(value) { \
    VALIDATE(void*, value, (value != nullptr)); \
}

#define VALIDATE_ARRAY_IPLfloat32(value, size) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        for (auto iArray = 0; iArray < size; ++iArray) { \
            auto isFinite = Math::isFinite(value[iArray]); \
            VALIDATE(IPLfloat32, value[iArray], isFinite); \
            if (!isFinite) { \
                break; \
            } \
        } \
    } \
}

#define VALIDATE_IPLbool(value) { \
    VALIDATE(IPLbool, value, (IPL_FALSE <= value && value <= IPL_TRUE)); \
}

#define VALIDATE_IPLSIMDLevel(value) { \
    VALIDATE(IPLSIMDLevel, value, (IPL_SIMDLEVEL_SSE2 <= value && value <= IPL_SIMDLEVEL_AVX512)); \
}

#define VALIDATE_IPLHRTFType(value) { \
    VALIDATE(IPLHRTFType, value, (IPL_HRTFTYPE_DEFAULT <= value && value <= IPL_HRTFTYPE_SOFA)); \
}

#define VALIDATE_IPLHRTFNormType(value) { \
    VALIDATE(IPLHRTFNormType, value, (IPL_HRTFNORMTYPE_NONE <= value && value <= IPL_HRTFNORMTYPE_RMS)); \
}

#define VALIDATE_IPLAudioEffectState(value) { \
    VALIDATE(IPLAudioEffectState, value, (IPL_AUDIOEFFECTSTATE_TAILREMAINING <= value && value <= IPL_AUDIOEFFECTSTATE_TAILCOMPLETE)); \
}

#define VALIDATE_IPLSpeakerLayoutType(value) { \
    VALIDATE(IPLSpeakerLayoutType, value, (IPL_SPEAKERLAYOUTTYPE_MONO <= value && value <= IPL_SPEAKERLAYOUTTYPE_CUSTOM)); \
}

#define VALIDATE_IPLHRTFInterpolation(value) { \
    VALIDATE(IPLHRTFInterpolation, value, (IPL_HRTFINTERPOLATION_NEAREST <= value && value <= IPL_HRTFINTERPOLATION_BILINEAR)); \
}

#define VALIDATE_IPLAmbisonicsType(value) { \
    VALIDATE(IPLAmbisonicsType, value, (IPL_AMBISONICSTYPE_N3D <= value && value <= IPL_AMBISONICSTYPE_FUMA)); \
}

#define VALIDATE_IPLVector3(value) { \
    VALIDATE_IPLfloat32(value.x); \
    VALIDATE_IPLfloat32(value.y); \
    VALIDATE_IPLfloat32(value.z); \
}

#define VALIDATE_IPLCoordinateSpace3(value) { \
    VALIDATE_IPLVector3(value.origin); \
    VALIDATE_IPLVector3(value.right); \
    VALIDATE_IPLVector3(value.up); \
    VALIDATE_IPLVector3(value.ahead); \
}

#define VALIDATE_IPLAudioBuffer(value, validateData) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE(IPLint32, value->numChannels, (value->numChannels > 0)); \
        VALIDATE(IPLint32, value->numSamples, (value->numSamples > 0)); \
        VALIDATE_POINTER(value->data); \
        if ((validateData) && value->data) { \
            for (auto iChannel = 0; iChannel < value->numChannels; ++iChannel) { \
                VALIDATE_IPLfloat32(value->data[iChannel][value->numSamples - 1]); \
            } \
        } \
    } \
}

#define VALIDATE_IPLContextSettings(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        if (value->allocateCallback) { \
            VALIDATE_POINTER(value->freeCallback); \
        } \
        VALIDATE_IPLSIMDLevel(value->simdLevel); \
    } \
}

#define VALIDATE_IPLAudioSettings(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE(IPLint32, value->samplingRate, (value->samplingRate > 0)); \
        VALIDATE(IPLint32, value->frameSize, (value->frameSize > 0)); \
    } \
}

#define VALIDATE_IPLHRTFSettings(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE_IPLHRTFType(value->type); \
        if (value->type == IPL_HRTFTYPE_SOFA) { \
            VALIDATE_POINTER(value->sofaFileName); \
        } \
        VALIDATE_IPLHRTFNormType(value->normType); \
    } \
}

#define VALIDATE_IPLSpeakerLayout(value) { \
    VALIDATE_IPLSpeakerLayoutType(value.type); \
    if (value.type == IPL_SPEAKERLAYOUTTYPE_CUSTOM) { \
        VALIDATE(IPLint32, value.numSpeakers, (value.numSpeakers > 0)); \
        VALIDATE_POINTER(value.speakers); \
    } \
}

#define VALIDATE_IPLPanningEffectSettings(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE_IPLSpeakerLayout(value->speakerLayout); \
    } \
}

#define VALIDATE_IPLPanningEffectParams(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE_IPLVector3(value->direction); \
    } \
}

#define VALIDATE_IPLBinauralEffectSettings(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE_POINTER(value->hrtf); \
    } \
}

#define VALIDATE_IPLBinauralEffectParams(value) { \
    VALIDATE_POINTER(value); \
    if (value) { \
        VALIDATE_IPLVector3(value->direction); \
        VALIDATE_IPLHRTFInterpolation(value->interpolation); \
        VALIDATE(IPLfloat32, value->spatialBlend, (0.0f <= value->spatialBlend && value->spatialBlend <= 1.0f)); \
        VALIDATE_POINTER(value->hrtf); \
    } \
}


// --------------------------------------------------------------------------------------------------------------------
// Forward declarations
// --------------------------------------------------------------------------------------------------------------------

class CValidatedHRTF;
class CValidatedPanningEffect;
class CValidatedBinauralEffect;


// --------------------------------------------------------------------------------------------------------------------
// CValidatedContext
// --------------------------------------------------------------------------------------------------------------------

class CValidatedContext : public CContext
{
public:
    CValidatedContext(IPLContextSettings* settings)
        : CContext(settings)
    {}

    virtual void setProfilerContext(void* profilerContext) override
    {
        VALIDATE_POINTER(profilerContext);
        CContext::setProfilerContext(profilerContext);
    }

    virtual IPLVector3 calculateRelativeDirection(IPLVector3 sourcePosition, IPLVector3 listenerPosition, IPLVector3 listenerAhead, IPLVector3 listenerUp) override
    {
        VALIDATE_IPLVector3(sourcePosition);
        VALIDATE_IPLVector3(listenerPosition);
        VALIDATE_IPLVector3(listenerAhead);
        VALIDATE_IPLVector3(listenerUp);
        auto result = CContext::calculateRelativeDirection(sourcePosition, listenerPosition, listenerAhead, listenerUp);
        VALIDATE_IPLVector3(result);
        return result;
    }

    // --- Retained features: audio buffer, HRTF, panning, binaural ---

    virtual IPLerror allocateAudioBuffer(IPLint32 numChannels, IPLint32 numSamples, IPLAudioBuffer* audioBuffer) override
    {
        VALIDATE(IPLint32, numChannels, (numChannels > 0));
        VALIDATE(IPLint32, numSamples, (numSamples > 0));
        VALIDATE_POINTER(audioBuffer);
        return CContext::allocateAudioBuffer(numChannels, numSamples, audioBuffer);
    }

    virtual void freeAudioBuffer(IPLAudioBuffer* audioBuffer) override
    {
        VALIDATE_POINTER(audioBuffer);
        CContext::freeAudioBuffer(audioBuffer);
    }

    virtual void interleaveAudioBuffer(IPLAudioBuffer* src, IPLfloat32* dst) override
    {
        VALIDATE_IPLAudioBuffer(src, true);
        VALIDATE_POINTER(dst);
        CContext::interleaveAudioBuffer(src, dst);
    }

    virtual void deinterleaveAudioBuffer(IPLfloat32* src, IPLAudioBuffer* dst) override
    {
        VALIDATE_POINTER(src);
        VALIDATE_IPLAudioBuffer(dst, false);
        CContext::deinterleaveAudioBuffer(src, dst);
    }

    virtual void mixAudioBuffer(IPLAudioBuffer* in, IPLAudioBuffer* mix) override
    {
        VALIDATE_IPLAudioBuffer(in, true);
        VALIDATE_IPLAudioBuffer(mix, false);
        CContext::mixAudioBuffer(in, mix);
    }

    virtual void downmixAudioBuffer(IPLAudioBuffer* in, IPLAudioBuffer* out) override
    {
        VALIDATE_IPLAudioBuffer(in, true);
        VALIDATE_IPLAudioBuffer(out, false);
        CContext::downmixAudioBuffer(in, out);
    }

    virtual void convertAmbisonicAudioBuffer(IPLAmbisonicsType inType, IPLAmbisonicsType outType, IPLAudioBuffer* in, IPLAudioBuffer* out) override
    {
        VALIDATE_IPLAmbisonicsType(inType);
        VALIDATE_IPLAmbisonicsType(outType);
        VALIDATE_IPLAudioBuffer(in, true);
        VALIDATE_IPLAudioBuffer(out, false);
        CContext::convertAmbisonicAudioBuffer(inType, outType, in, out);
    }

    virtual IPLerror createHRTF(IPLAudioSettings* audioSettings, IPLHRTFSettings* hrtfSettings, IHRTF** hrtf) override
    {
        VALIDATE_IPLAudioSettings(audioSettings);
        VALIDATE_IPLHRTFSettings(hrtfSettings);
        VALIDATE_POINTER(hrtf);
        return apiObjectAllocate<CValidatedHRTF, CContext, IHRTF>(hrtf, this, audioSettings, hrtfSettings);
    }

    virtual IPLerror createPanningEffect(IPLAudioSettings* audioSettings, IPLPanningEffectSettings* effectSettings, IPanningEffect** effect) override
    {
        VALIDATE_IPLAudioSettings(audioSettings);
        VALIDATE_IPLPanningEffectSettings(effectSettings);
        VALIDATE_POINTER(effect);
        return apiObjectAllocate<CValidatedPanningEffect, CContext, IPanningEffect>(effect, this, audioSettings, effectSettings);
    }

    virtual IPLerror createBinauralEffect(IPLAudioSettings* audioSettings, IPLBinauralEffectSettings* effectSettings, IBinauralEffect** effect) override
    {
        VALIDATE_IPLAudioSettings(audioSettings);
        VALIDATE_IPLBinauralEffectSettings(effectSettings);
        VALIDATE_POINTER(effect);
        return apiObjectAllocate<CValidatedBinauralEffect, CContext, IBinauralEffect>(effect, this, audioSettings, effectSettings);
    }

    // --- Stripped features: delegate to CContext stubs (return IPL_STATUS_FAILURE) ---

    virtual IPLerror createSerializedObject(IPLSerializedObjectSettings* settings, ISerializedObject** serializedObject) override
    { return CContext::createSerializedObject(settings, serializedObject); }

    virtual IPLerror createEmbreeDevice(IPLEmbreeDeviceSettings* settings, IEmbreeDevice** device) override
    { return CContext::createEmbreeDevice(settings, device); }

    virtual IPLerror createOpenCLDeviceList(IPLOpenCLDeviceSettings* settings, IOpenCLDeviceList** deviceList) override
    { return CContext::createOpenCLDeviceList(settings, deviceList); }

    virtual IPLerror createOpenCLDevice(IOpenCLDeviceList* deviceList, IPLint32 index, IOpenCLDevice** device) override
    { return CContext::createOpenCLDevice(deviceList, index, device); }

    virtual IPLerror createOpenCLDeviceFromExisting(void* convolutionQueue, void* irUpdateQueue, IOpenCLDevice** device) override
    { return CContext::createOpenCLDeviceFromExisting(convolutionQueue, irUpdateQueue, device); }

    virtual IPLerror createScene(IPLSceneSettings* settings, IScene** scene) override
    { return CContext::createScene(settings, scene); }

    virtual IPLerror loadScene(IPLSceneSettings* settings, ISerializedObject* serializedObject, IPLProgressCallback progressCallback, void* userData, IScene** scene) override
    { return CContext::loadScene(settings, serializedObject, progressCallback, userData, scene); }

    virtual IPLerror createVirtualSurroundEffect(IPLAudioSettings* audioSettings, IPLVirtualSurroundEffectSettings* effectSettings, IVirtualSurroundEffect** effect) override
    { return CContext::createVirtualSurroundEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createAmbisonicsEncodeEffect(IPLAudioSettings* audioSettings, IPLAmbisonicsEncodeEffectSettings* effectSettings, IAmbisonicsEncodeEffect** effect) override
    { return CContext::createAmbisonicsEncodeEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createAmbisonicsPanningEffect(IPLAudioSettings* audioSettings, IPLAmbisonicsPanningEffectSettings* effectSettings, IAmbisonicsPanningEffect** effect) override
    { return CContext::createAmbisonicsPanningEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createAmbisonicsBinauralEffect(IPLAudioSettings* audioSettings, IPLAmbisonicsBinauralEffectSettings* effectSettings, IAmbisonicsBinauralEffect** effect) override
    { return CContext::createAmbisonicsBinauralEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createAmbisonicsRotationEffect(IPLAudioSettings* audioSettings, IPLAmbisonicsRotationEffectSettings* effectSettings, IAmbisonicsRotationEffect** effect) override
    { return CContext::createAmbisonicsRotationEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createAmbisonicsDecodeEffect(IPLAudioSettings* audioSettings, IPLAmbisonicsDecodeEffectSettings* effectSettings, IAmbisonicsDecodeEffect** effect) override
    { return CContext::createAmbisonicsDecodeEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createDirectEffect(IPLAudioSettings* audioSettings, IPLDirectEffectSettings* effectSettings, IDirectEffect** effect) override
    { return CContext::createDirectEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createReflectionEffect(IPLAudioSettings* audioSettings, IPLReflectionEffectSettings* effectSettings, IReflectionEffect** effect) override
    { return CContext::createReflectionEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createReflectionMixer(IPLAudioSettings* audioSettings, IPLReflectionEffectSettings* effectSettings, IReflectionMixer** mixer) override
    { return CContext::createReflectionMixer(audioSettings, effectSettings, mixer); }

    virtual IPLerror createPathEffect(IPLAudioSettings* audioSettings, IPLPathEffectSettings* effectSettings, IPathEffect** effect) override
    { return CContext::createPathEffect(audioSettings, effectSettings, effect); }

    virtual IPLerror createProbeArray(IProbeArray** probeArray) override
    { return CContext::createProbeArray(probeArray); }

    virtual IPLerror createProbeBatch(IProbeBatch** probeBatch) override
    { return CContext::createProbeBatch(probeBatch); }

    virtual IPLerror loadProbeBatch(ISerializedObject* serializedObject, IProbeBatch** probeBatch) override
    { return CContext::loadProbeBatch(serializedObject, probeBatch); }

    virtual void bakeReflections(IPLReflectionsBakeParams* params, IPLProgressCallback progressCallback, void* userData) override
    { CContext::bakeReflections(params, progressCallback, userData); }

    virtual void cancelBakeReflections() override
    { CContext::cancelBakeReflections(); }

    virtual void bakePaths(IPLPathBakeParams* params, IPLProgressCallback progressCallback, void* userData) override
    { CContext::bakePaths(params, progressCallback, userData); }

    virtual void cancelBakePaths() override
    { CContext::cancelBakePaths(); }

    virtual IPLerror createSimulator(IPLSimulationSettings* settings, ISimulator** simulator) override
    { return CContext::createSimulator(settings, simulator); }

    virtual IPLfloat32 calculateDistanceAttenuation(IPLVector3 source, IPLVector3 listener, IPLDistanceAttenuationModel* model) override
    { return CContext::calculateDistanceAttenuation(source, listener, model); }

    virtual void calculateAirAbsorption(IPLVector3 source, IPLVector3 listener, IPLAirAbsorptionModel* model, IPLfloat32* airAbsorption) override
    { CContext::calculateAirAbsorption(source, listener, model, airAbsorption); }

    virtual IPLfloat32 calculateDirectivity(IPLCoordinateSpace3 source, IPLVector3 listener, IPLDirectivity* model) override
    { return CContext::calculateDirectivity(source, listener, model); }

    virtual IPLerror createEnergyField(const IPLEnergyFieldSettings* settings, IEnergyField** energyField) override
    { return CContext::createEnergyField(settings, energyField); }

    virtual IPLerror createImpulseResponse(const IPLImpulseResponseSettings* settings, IImpulseResponse** impulseResponse) override
    { return CContext::createImpulseResponse(settings, impulseResponse); }

    virtual IPLerror createReconstructor(const IPLReconstructorSettings* settings, IReconstructor** reconstructor) override
    { return CContext::createReconstructor(settings, reconstructor); }
};


// --------------------------------------------------------------------------------------------------------------------
// CContext::createContext
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createContext(IPLContextSettings* settings,
                                 IContext** context)
{
    if (!settings || !context)
        return IPL_STATUS_FAILURE;

    if (!isVersionCompatible(settings->version))
        return IPL_STATUS_FAILURE;

    Context::sAPIVersion = settings->version;

    auto _allocateCallback = reinterpret_cast<AllocateCallback>(settings->allocateCallback);
    auto _freeCallback = reinterpret_cast<FreeCallback>(settings->freeCallback);
    Context::sMemory.init(_allocateCallback, _freeCallback);

    auto _enableValidation = false;
    if (Context::isCallerAPIVersionAtLeast(4, 5))
    {
        _enableValidation = (settings->flags & IPL_CONTEXTFLAGS_VALIDATION);
    }

    if (_enableValidation)
    {
        VALIDATE_IPLContextSettings(settings);

        try
        {
            auto _context = reinterpret_cast<CValidatedContext*>(gMemory().allocate(sizeof(CValidatedContext), Memory::kDefaultAlignment));
            new (_context) CValidatedContext(settings);
            *context = _context;
        }
        catch (Exception exception)
        {
            return static_cast<IPLerror>(exception.status());
        }
    }
    else
    {
        try
        {
            auto _context = reinterpret_cast<CContext*>(gMemory().allocate(sizeof(CContext), Memory::kDefaultAlignment));
            new (_context) CContext(settings);
            *context = _context;
        }
        catch (Exception exception)
        {
            return static_cast<IPLerror>(exception.status());
        }
    }

    return IPL_STATUS_SUCCESS;
}


// --------------------------------------------------------------------------------------------------------------------
// CValidatedHRTF
// --------------------------------------------------------------------------------------------------------------------

class CValidatedHRTF : public CHRTF
{
public:
    CValidatedHRTF(CContext* context, IPLAudioSettings* audioSettings, IPLHRTFSettings* hrtfSettings)
        : CHRTF(context, audioSettings, hrtfSettings)
    {}
};


// --------------------------------------------------------------------------------------------------------------------
// CValidatedPanningEffect
// --------------------------------------------------------------------------------------------------------------------

class CValidatedPanningEffect : public CPanningEffect
{
public:
    CValidatedPanningEffect(CContext* context, IPLAudioSettings* audioSettings, IPLPanningEffectSettings* effectSettings)
        : CPanningEffect(context, audioSettings, effectSettings)
    {}

    virtual IPLAudioEffectState apply(IPLPanningEffectParams* params, IPLAudioBuffer* in, IPLAudioBuffer* out) override
    {
        VALIDATE_IPLPanningEffectParams(params);
        VALIDATE_IPLAudioBuffer(in, true);
        VALIDATE_IPLAudioBuffer(out, false);

        auto result = CPanningEffect::apply(params, in, out);

        VALIDATE_IPLAudioEffectState(result);
        VALIDATE_IPLAudioBuffer(out, true);

        return result;
    }
};


// --------------------------------------------------------------------------------------------------------------------
// CValidatedBinauralEffect
// --------------------------------------------------------------------------------------------------------------------

class CValidatedBinauralEffect : public CBinauralEffect
{
public:
    CValidatedBinauralEffect(CContext* context, IPLAudioSettings* audioSettings, IPLBinauralEffectSettings* effectSettings)
        : CBinauralEffect(context, audioSettings, effectSettings)
    {}

    virtual IPLAudioEffectState apply(IPLBinauralEffectParams* params, IPLAudioBuffer* in, IPLAudioBuffer* out) override
    {
        VALIDATE_IPLBinauralEffectParams(params);
        VALIDATE_IPLAudioBuffer(in, true);
        VALIDATE_IPLAudioBuffer(out, false);

        auto result = CBinauralEffect::apply(params, in, out);

        VALIDATE_IPLAudioEffectState(result);
        VALIDATE_IPLAudioBuffer(out, true);

        return result;
    }
};

}
