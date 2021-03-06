/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   DeferredRendererSystem.h
//! \author Kleber Garcia
//! \date   June 23rd, 2016
//! \brief  deferred renderer system definition file
                               
#ifndef PEGASUS_RENDER_SYSTEM_DEFERREDRENDERER_H
#define PEGASUS_RENDER_SYSTEM_DEFERREDRENDERER_H
#include "Pegasus/RenderSystems/Config.h"
#if RENDER_SYSTEM_CONFIG_ENABLE_DEFERREDRENDERER

#include "Pegasus/RenderSystems/System/RenderSystem.h"

namespace Pegasus 
{
namespace RenderSystems
{

//! Deferred renderer system implementation. Adds a blockscript library.
class DeferredRendererSystem : public RenderSystem
{
public:
    //! Constructor
    explicit DeferredRendererSystem(Alloc::IAllocator* allocator) : RenderSystem(allocator) {}

    //! destructor
    virtual ~DeferredRendererSystem() {}

    virtual bool CanCreateBlockScriptApi() const { return true; }

    virtual const char* GetSystemName() const { return "DeferredRendererSystem"; }

    virtual void OnRegisterBlockscriptApi(BlockScript::BlockLib* blocklib, Core::IApplicationContext* appContext);

};
}
}

#endif
#endif
