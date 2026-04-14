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

// Stub implementations for CContext virtual methods whose real implementations
// have been removed in the minimal (HRTF + BinauralEffect only) build.

#include "vector.h"
using namespace ipl;

#include "phonon.h"

#define STEAMAUDIO_SKIP_API_FUNCTIONS
#include "phonon_interfaces.h"
#include "api_context.h"

namespace api {

// --------------------------------------------------------------------------------------------------------------------
// Geometry (was api_geometry.cpp) - kept as real implementation since it's useful and lightweight
// --------------------------------------------------------------------------------------------------------------------

IPLVector3 CContext::calculateRelativeDirection(IPLVector3 sourcePosition,
                                                IPLVector3 listenerPosition,
                                                IPLVector3 listenerAhead,
                                                IPLVector3 listenerUp)
{
    auto _sourcePosition = Vector3f(sourcePosition.x, sourcePosition.y, sourcePosition.z);
    auto _listenerPosition = Vector3f(listenerPosition.x, listenerPosition.y, listenerPosition.z);
    auto _listenerAhead = Vector3f(listenerAhead.x, listenerAhead.y, listenerAhead.z);
    auto _listenerUp = Vector3f(listenerUp.x, listenerUp.y, listenerUp.z);
    auto _listenerRight = Vector3f::unitVector(Vector3f::cross(_listenerAhead, _listenerUp));

    auto listenerToSource = _sourcePosition - _listenerPosition;
    auto _relativeDirection = Vector3f::kYAxis;

    if (listenerToSource.length() > 1e-5f)
    {
        listenerToSource = Vector3f::unitVector(listenerToSource);
        _relativeDirection.x() = (Vector3f::dot(listenerToSource, _listenerRight));
        _relativeDirection.y() = (Vector3f::dot(listenerToSource, _listenerUp));
        _relativeDirection.z() = (-Vector3f::dot(listenerToSource, _listenerAhead));
    }

    IPLVector3 relativeDirection = {_relativeDirection.x(), _relativeDirection.y(), _relativeDirection.z()};
    return relativeDirection;
}

// --------------------------------------------------------------------------------------------------------------------
// Serialization stubs (was api_serialized_object.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createSerializedObject(IPLSerializedObjectSettings* /*settings*/,
                                          ISerializedObject** /*serializedObject*/)
{
    return IPL_STATUS_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// Device stubs (was api_embree_device.cpp, api_opencl_device.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createEmbreeDevice(IPLEmbreeDeviceSettings* /*settings*/,
                                      IEmbreeDevice** /*device*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createOpenCLDeviceList(IPLOpenCLDeviceSettings* /*settings*/,
                                          IOpenCLDeviceList** /*deviceList*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createOpenCLDevice(IOpenCLDeviceList* /*deviceList*/,
                                      IPLint32 /*index*/,
                                      IOpenCLDevice** /*device*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createOpenCLDeviceFromExisting(void* /*convolutionQueue*/,
                                                  void* /*irUpdateQueue*/,
                                                  IOpenCLDevice** /*device*/)
{
    return IPL_STATUS_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// Scene stubs (was api_scene.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createScene(IPLSceneSettings* /*settings*/,
                               IScene** /*scene*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::loadScene(IPLSceneSettings* /*settings*/,
                             ISerializedObject* /*serializedObject*/,
                             IPLProgressCallback /*progressCallback*/,
                             void* /*userData*/,
                             IScene** /*scene*/)
{
    return IPL_STATUS_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// Effect stubs (was api_virtual_surround_effect.cpp, api_ambisonics_*.cpp, api_direct_effect.cpp,
//               api_indirect_effect.cpp, api_path_effect.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createVirtualSurroundEffect(IPLAudioSettings* /*audioSettings*/,
                                               IPLVirtualSurroundEffectSettings* /*effectSettings*/,
                                               IVirtualSurroundEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createAmbisonicsEncodeEffect(IPLAudioSettings* /*audioSettings*/,
                                                IPLAmbisonicsEncodeEffectSettings* /*effectSettings*/,
                                                IAmbisonicsEncodeEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createAmbisonicsPanningEffect(IPLAudioSettings* /*audioSettings*/,
                                                 IPLAmbisonicsPanningEffectSettings* /*effectSettings*/,
                                                 IAmbisonicsPanningEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createAmbisonicsBinauralEffect(IPLAudioSettings* /*audioSettings*/,
                                                  IPLAmbisonicsBinauralEffectSettings* /*effectSettings*/,
                                                  IAmbisonicsBinauralEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createAmbisonicsRotationEffect(IPLAudioSettings* /*audioSettings*/,
                                                  IPLAmbisonicsRotationEffectSettings* /*effectSettings*/,
                                                  IAmbisonicsRotationEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createAmbisonicsDecodeEffect(IPLAudioSettings* /*audioSettings*/,
                                                IPLAmbisonicsDecodeEffectSettings* /*effectSettings*/,
                                                IAmbisonicsDecodeEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createDirectEffect(IPLAudioSettings* /*audioSettings*/,
                                      IPLDirectEffectSettings* /*effectSettings*/,
                                      IDirectEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createReflectionEffect(IPLAudioSettings* /*audioSettings*/,
                                          IPLReflectionEffectSettings* /*effectSettings*/,
                                          IReflectionEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createReflectionMixer(IPLAudioSettings* /*audioSettings*/,
                                         IPLReflectionEffectSettings* /*effectSettings*/,
                                         IReflectionMixer** /*mixer*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createPathEffect(IPLAudioSettings* /*audioSettings*/,
                                    IPLPathEffectSettings* /*effectSettings*/,
                                    IPathEffect** /*effect*/)
{
    return IPL_STATUS_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// Probe stubs (was api_probes.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createProbeArray(IProbeArray** /*probeArray*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createProbeBatch(IProbeBatch** /*probeBatch*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::loadProbeBatch(ISerializedObject* /*serializedObject*/,
                                  IProbeBatch** /*probeBatch*/)
{
    return IPL_STATUS_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// Baking stubs (was api_baking.cpp)
// --------------------------------------------------------------------------------------------------------------------

void CContext::bakeReflections(IPLReflectionsBakeParams* /*params*/,
                               IPLProgressCallback /*progressCallback*/,
                               void* /*userData*/)
{
}

void CContext::cancelBakeReflections()
{
}

void CContext::bakePaths(IPLPathBakeParams* /*params*/,
                         IPLProgressCallback /*progressCallback*/,
                         void* /*userData*/)
{
}

void CContext::cancelBakePaths()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Simulator stubs (was api_simulator.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createSimulator(IPLSimulationSettings* /*settings*/,
                                   ISimulator** /*simulator*/)
{
    return IPL_STATUS_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// Advanced simulation stubs (was api_advanced_simulation.cpp)
// --------------------------------------------------------------------------------------------------------------------

IPLfloat32 CContext::calculateDistanceAttenuation(IPLVector3 /*source*/,
                                                  IPLVector3 /*listener*/,
                                                  IPLDistanceAttenuationModel* /*model*/)
{
    return 1.0f;
}

void CContext::calculateAirAbsorption(IPLVector3 /*source*/,
                                      IPLVector3 /*listener*/,
                                      IPLAirAbsorptionModel* /*model*/,
                                      IPLfloat32* airAbsorption)
{
    if (airAbsorption)
    {
        for (auto i = 0; i < 3; ++i)
            airAbsorption[i] = 1.0f;
    }
}

IPLfloat32 CContext::calculateDirectivity(IPLCoordinateSpace3 /*source*/,
                                          IPLVector3 /*listener*/,
                                          IPLDirectivity* /*model*/)
{
    return 1.0f;
}

IPLerror CContext::createEnergyField(const IPLEnergyFieldSettings* /*settings*/,
                                     IEnergyField** /*energyField*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createImpulseResponse(const IPLImpulseResponseSettings* /*settings*/,
                                         IImpulseResponse** /*impulseResponse*/)
{
    return IPL_STATUS_FAILURE;
}

IPLerror CContext::createReconstructor(const IPLReconstructorSettings* /*settings*/,
                                       IReconstructor** /*reconstructor*/)
{
    return IPL_STATUS_FAILURE;
}

}
