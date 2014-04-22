/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   ProgramProxy.h
//! \author Kleber Garcia 
//! \date   16 March 2014
//! \brief  Proxy interface, used by the editor and launcher to interact with the shader mgr

#ifndef PEGASUS_PROGRAMPROXY_H
#define PEGASUS_PROGRAMPROXY_H

#if PEGASUS_ENABLE_PROXIES

#include "Pegasus/Shader/Shared/IProgramProxy.h"
#include "Pegasus/Shader/ShaderProxy.h"

namespace Pegasus
{
namespace Shader
{
class ProgramLinkage;

//! Program proxy. To be used by the editor
class ProgramProxy : public IProgramProxy
{
public:
    ProgramProxy(){}
    virtual ~ProgramProxy() {}

    //! returns the number of shaders this program holds.
    //! \return number of shaders this program holds.
    virtual int GetShaderCount() const; 

    //! Gets the shader proxy by id.
    //! \param i the index of the shader proxy child
    //! \return null if shader is not found, otherwise a pointer to its proxy
    virtual IShaderProxy * GetShader(unsigned i);

    //! \return Gets the name of this shader program
    virtual const char * GetName() const;

    //! Sets the user data for this particular program
    //! \param userData. the user data to retrieve
    virtual void SetUserData(Pegasus::Shader::IUserData * userData);

    //! Gets the user data for this particular program 
    //! \return user data reference
    virtual Pegasus::Shader::IUserData * GetUserData() const;

    //! sets the internal implementation of shader linkage
    //! \param object the program linkage internal object.
    void Wrap(ProgramLinkage * object) { mObject = object; }

    //! \return Returns the internal program linkage owned.
    ProgramLinkage * Unwrap() const { return mObject; }

private:

    //! internal worker, obfuscated
    ProgramLinkage * mObject;
    
    //! pool of shader proxies.
    ShaderProxy mShaderProxyPool[SHADER_STAGES_COUNT];
    
};

}
}
#endif // Proxies 
#endif // ifdef guard