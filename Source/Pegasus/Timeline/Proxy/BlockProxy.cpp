/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	BlockProxy.cpp
//! \author	Karolyn Boulanger
//! \date	09th November 2013
//! \brief	Proxy object, used by the editor to interact with the timeline blocks

//! \todo Why do we need this in Rel-Debug? LaneProxy should not even be compiled in REL mode
PEGASUS_AVOID_EMPTY_FILE_WARNING

#if PEGASUS_ENABLE_PROXIES

#include "Pegasus/PegasusAssetTypes.h"
#include "Pegasus/Timeline/Proxy/BlockProxy.h"
#include "Pegasus/Timeline/Proxy/LaneProxy.h"
#include "Pegasus/Timeline/Block.h"
#include "Pegasus/Timeline/Lane.h"
#include "Pegasus/Timeline/TimelineScript.h"
#include "Pegasus/AssetLib/Shared/IAssetProxy.h"
#include "Pegasus/AssetLib/Proxy/AssetProxy.h"
#include "Pegasus/Core/Shared/ISourceCodeProxy.h"
#include "Pegasus/Math/Color.h"
#include "Pegasus/PropertyGrid/Shared/PropertyEventDefs.h"
#include "Pegasus/PropertyGrid/PropertyGridObject.h"

namespace Pegasus {
namespace Timeline {

PropertyFlusherPropertyGridObjectDecorator::PropertyFlusherPropertyGridObjectDecorator(TimelineScriptRunner* runner, PropertyGrid::IPropertyGridObjectProxy* propertyGrid)
    : mRunner(runner)
    , mDecorated(propertyGrid)
{
}

PropertyFlusherPropertyGridObjectDecorator::~PropertyFlusherPropertyGridObjectDecorator()
{
    PEGASUS_EVENT_DISPATCH(static_cast<Pegasus::PropertyGrid::PropertyGridObjectProxy*>(mDecorated)->GetObject(), Pegasus::PropertyGrid::PropertyGridDestroyed);
}

void PropertyFlusherPropertyGridObjectDecorator::WriteObjectProperty(unsigned int index, const void * inputBuffer, unsigned int inputBufferSize, bool sendMessage)
{
    mDecorated->WriteObjectProperty(index, inputBuffer, inputBufferSize, sendMessage);

    //notify now the block that an internal property has now been updated.
    mRunner->NotifyInternalObjectPropertyUpdated(index);
}


BlockProxy::BlockProxy(Block * block)
:   mBlock(block),
    mPropertyGridDecorator(&block->GetScriptRunner(), block->GetPropertyGridProxy())
{
    PG_ASSERTSTR(block != nullptr, "Trying to create a timeline block proxy from an invalid timeline block object");
}

//----------------------------------------------------------------------------------------

BlockProxy::~BlockProxy()
{
}

//----------------------------------------------------------------------------------------

PropertyGrid::IPropertyGridObjectProxy * BlockProxy::GetPropertyGridProxy()
{
    return &mPropertyGridDecorator;
}

//----------------------------------------------------------------------------------------

const PropertyGrid::IPropertyGridObjectProxy * BlockProxy::GetPropertyGridProxy() const
{
    return &mPropertyGridDecorator;
}

//----------------------------------------------------------------------------------------

void BlockProxy::Initialize()
{
    mBlock->Initialize();
}

void BlockProxy::Shutdown()
{
    mBlock->Shutdown();
}

//----------------------------------------------------------------------------------------

Beat BlockProxy::GetBeat() const
{
    return mBlock->GetBeat();
}

//----------------------------------------------------------------------------------------

Duration BlockProxy::GetDuration() const
{
    return mBlock->GetDuration();
}

//----------------------------------------------------------------------------------------

ILaneProxy * BlockProxy::GetLane() const
{
    Lane * const lane = mBlock->GetLane();
    if (lane != nullptr)
    {
        return lane->GetProxy();
    }
    else
    {
        return nullptr;
    }
}

//----------------------------------------------------------------------------------------

void BlockProxy::SetColor(unsigned char red, unsigned char green, unsigned char blue)
{
    Math::Color8RGB c(red, green, blue);
    mBlock->SetColor(c);
}

//----------------------------------------------------------------------------------------

void BlockProxy::GetColor(unsigned char & red, unsigned char & green, unsigned char & blue) const
{
    Math::Color8RGB c = mBlock->GetColor();
    red = c.red;
    green = c.green;
    blue = c.blue;
}

//----------------------------------------------------------------------------------------

Core::ISourceCodeProxy* BlockProxy::GetScript() const
{
    TimelineScript* helper = mBlock->GetScript();
    if (helper != nullptr)
    {
        return static_cast<Core::ISourceCodeProxy*>(helper->GetProxy());
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------

unsigned BlockProxy::GetGuid() const
{
    return mBlock->GetGuid();
}

//----------------------------------------------------------------------------------------

void BlockProxy::OverrideGuid(unsigned newGuid)
{
    return mBlock->OverrideGuid(newGuid);
}

//----------------------------------------------------------------------------------------

const char * BlockProxy::GetInstanceName() const
{
    return mBlock->GetName();
}

//----------------------------------------------------------------------------------------

const char* BlockProxy::GetClassName() const
{
    return mBlock->GetClassName();
}

void BlockProxy::AttachScript(Core::ISourceCodeProxy* code)
{
    PG_ASSERT(code->GetOwnerAsset()->GetTypeDesc()->mTypeGuid == ASSET_TYPE_BLOCKSCRIPT.mTypeGuid);
    if (code->GetOwnerAsset()->GetTypeDesc()->mTypeGuid == ASSET_TYPE_BLOCKSCRIPT.mTypeGuid)
    {
        Pegasus::Timeline::TimelineScriptProxy* scriptProxy = static_cast<Pegasus::Timeline::TimelineScriptProxy*>(code);
        Pegasus::Timeline::TimelineScriptRef timelineScript = static_cast<Pegasus::Timeline::TimelineScript*>(scriptProxy->GetObject());
        mBlock->AttachScript(timelineScript); 
    }
}

void BlockProxy::ClearScript()
{
    mBlock->ShutdownScript();
}

void BlockProxy::DumpToAsset(Pegasus::AssetLib::IAssetProxy* assetProxy)
{
    Pegasus::AssetLib::AssetProxy* asset = static_cast<Pegasus::AssetLib::AssetProxy*>(assetProxy);
    mBlock->DumpToAsset(asset->GetObject());
}

void BlockProxy::LoadFromAsset(const Pegasus::AssetLib::IAssetProxy* assetProxy)
{
    const Pegasus::AssetLib::AssetProxy* asset = static_cast<const Pegasus::AssetLib::AssetProxy*>(assetProxy);
    mBlock->LoadFromAsset(asset->GetObject());
}

Pegasus::AssetLib::ICategoryProxy* BlockProxy::GetAssetCategory()
{
    return mBlock->GetAssetCategory()->GetProxy();
}

}   // namespace Timeline
}   // namespace Pegasus


#endif  // PEGASUS_ENABLE_PROXIES
