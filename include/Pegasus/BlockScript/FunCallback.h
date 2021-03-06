/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   FunCallback.h
//! \author Kleber Garcia
//! \date   October 28th 2014
//! \brief  Runtime utilities to register c++ callback functions and globals into blockscript

#ifndef BLOCKSCRIPT_FUNCALLBACK_H
#define BLOCKSCRIPT_FUNCALLBACK_H

#include "Pegasus/BlockScript/TypeDesc.h"

namespace Pegasus
{
namespace BlockScript
{

namespace Ast
{
    class ExpList;
}

class BlockLib;

typedef int (*PrintStringCallbackType)(const char *);
typedef int (*PrintIntCallbackType)(int);
typedef int (*PrintFloatCallbackType)(float);

class SystemCallbacks
{
public:
static PrintStringCallbackType gPrintStrCallback;
static PrintIntCallbackType    gPrintIntCallback;
static PrintFloatCallbackType  gPrintFloatCallback;
};

//fwd declarations
class BlockScriptBuilder;
class FunDesc;
class BsVmState;
class BsVm;
struct Assembly;

class FunCallbackContext
{
public:

    //! constructor
    //! \param state the virtual machine state, containing all the memory and register values required.
    //! \param funDesc piece of data containing function description
    //! \param inputBuffer the raw buffer (packed) of the function arguments.
    //! \param inputBufferSize the raw buffer size of the input arguments
    //! \param outputBuffer, the return value buffer.
    //! \param outputBufferSize, the buffer size of the return value
    FunCallbackContext(
        BsVmState* state, 
        const FunDesc* funDesc,
        const Ast::ExpList* argumentExpressions,
        void* inputBuffer,
        int inputBufferSize,
        void* outputBuffer,
        int outputBufferSize
    ) : mState(state), 
        mFunDesc(funDesc), 
        mArgExps(argumentExpressions),
        mInputBuffer(inputBuffer), 
        mInputBufferSize(inputBufferSize), 
        mOutputBuffer(outputBuffer), 
        mOutputBufferSize(outputBufferSize) 
        {
        }
    ~FunCallbackContext(){}


    //! \return the virtual machine state.
    BsVmState* GetVmState() { return mState; }

    //! \return gets the argument expressions used in the function call
    const Ast::ExpList* GetArgExps() { return mArgExps; }

    //! \return the raw input buffer. Use offsets manually and reinterpret_cast to the correct types.
    void* GetRawInputBuffer() { return mInputBuffer; }

    //! \return the input buffer size of the packed inputs
    int  GetInputBufferSize() const { return mInputBufferSize; }

    //! \return the output buffer.
    void* GetRawOutputBuffer() { return mOutputBuffer; }

    //! \return the output buffer size
    int   GetOutputBufferSize() const { return mOutputBufferSize; }

    //! \return the function description of the caller.
    const FunDesc* GetFunDesc() { return mFunDesc; }

private:

    BsVmState* mState;
    const FunDesc* mFunDesc;
    const Ast::ExpList* mArgExps;
    void* mInputBuffer;
    int   mInputBufferSize;
    void* mOutputBuffer;
    int   mOutputBufferSize;
};

//class that serves as a convinience function to parse arguments from a blockscript vm runtime
//and to return values 
class FunParamStream
{
public:
    //! Constructor
    //\param ctx the callback context, this context contains all the raw buffers where arguments are located,
    explicit FunParamStream(FunCallbackContext& ctx) : mContext(&ctx), mBufferPos(0) {}

    //! destructor
    ~FunParamStream(){}

    //! Resets the reader to the begining
    void Reset() { mBufferPos = 0; }
   
    //! Reads the next argument passed from block script function (starts from leftmost value to right most)
    //! \param outArgument a pointer to a pointer of the argument to read.
    template <class T>
    T& NextArgument()
    { 
        return *static_cast<T*>(NextArgument(sizeof(T))); 
    }
    
    //! This is for the blockscript string type. Any type is passed, dont use NextArgument, use this function instead to retrieve it.
    //! \return c string of the blockscript string
    const char* NextBsStringArgument();
    
    //! equivalent to a block script call to return. Writes to the return buffer, asserts on error
    //! \param retVal passed by reference, the value that will be returned back up to blockscript
    template <class T>
    void SubmitReturn(const T& retVal)
    {
        PG_ASSERTSTR(sizeof(T) == mContext->GetOutputBufferSize(), "Incompatible return type from signature.");
        *static_cast<T*>(mContext->GetRawOutputBuffer()) = retVal;
    }
    

private:
    
    //! reads the next argument, internal implementation
    void* NextArgument(int sz);

    int mBufferPos;
    FunCallbackContext* mContext;
};

//block script callback for c++
typedef void (*FunCallback)(FunCallbackContext& context);

//! Function bind point (handle)
typedef int FunBindPoint;

//! Global bind point (handle) 
typedef int GlobalBindPoint;

const FunBindPoint FUN_INVALID_BIND_POINT = -1;

const GlobalBindPoint GLOBAL_INVALID_BIND_POINT = -1;

#define MAX_FUN_ARG_LIST 20

struct FunctionDeclarationDesc
{
    const char* functionName;
    const char* returnType;
    const char* argumentTypes[MAX_FUN_ARG_LIST];
    const char* argumentNames[MAX_FUN_ARG_LIST];
    FunCallback callback;
};

#define MAX_OBJ_PROPERTY_LIST 20

struct ObjectPropertyDesc
{
    const char* propertyTypeName;
    const char* propertyName;
    int propertyUniqueId;
};

#define MAX_ENUM_MEMBER_LIST 50

struct EnumDeclarationDesc
{
    const char*  typeName;
    struct enumData{
        const char* enumName;
        int enumVal;         
    } enumList[MAX_ENUM_MEMBER_LIST];
    int   count;
};

#define MAX_MEMBER_LIST 20

struct StructDeclarationDesc
{
    const char* structTypeName;
    const char* memberTypes[MAX_MEMBER_LIST];
    const char* memberNames[MAX_MEMBER_LIST];
};

#define MAX_OBJECT_METHOD_DESCRIPTORS 50

struct ClassTypeDesc
{
    const char* classTypeName;
    FunctionDeclarationDesc methodDescriptors[MAX_OBJECT_METHOD_DESCRIPTORS];
    int methodsCount;
    const ObjectPropertyDesc* propertyDescriptors;
    int propertyCount;
    ObjectPropertyAccessorCallback getPropertyCallback;
};

//! Gets a function bind point to be used to call.
//! \param builder - the ast builder, containing necessary meta-data
//! \param assembly - the assembly code, containing the mapping with function bind points
//! \param funName - the string name of the function
//! \param argTypes - the argument definitions of the function 
//! \param argumentListCount - the count of the arguments of this function
//! \return FUN_INVALID_BIND_POINT if the function does not exist, otherwise a valid bind point.
FunBindPoint GetFunctionBindPoint(
    const BlockScriptBuilder* builder, 
    const Assembly& assembly,
    const char* funName,
    const char*const* argTypes,
    int argumentListCount
);


//! Executes a function from a specific bind point.
//! \param bindPoint - the function bind point. If an invalid bind point is passed, we return false.
//! \param builder - the ast builder, containing necessary meta-data
//! \param assembly - the assembly instruction set.
//! \param bsVmState - the vm state.
//! \param vm - the vm that will run the function.
//! \param inputBuffer - the input buffer. Pack all function arguments on this structure. For heap arguments, such as strings,
//!                      register them manually on the vm state and then get an identifier int. Pass this int then in the buffer.
//! \param inputBufferSize - the size of the input argument buffer. If this size does not match the input buffer size of the function then this function returns false.
//! \param outputBuffer - the output buffer to be used. 
//! \param outputBufferSize - the size of the return buffer. If this size does not match the return value size, then this function returns false.
bool ExecuteFunction(
    FunBindPoint bindPoint,
    BlockScriptBuilder* builder, 
    const Assembly& assembly,
    BsVmState& state,
    BsVm& vm,
    const void* inputBuffer,
    int   inputBufferSize,
    void* outputBuffer,
    int   outputBufferSize
);

//! Reads a global value from the VM state.
//! \param bindPoint the bind point of the global value to read from
//! \param assembly the assembly instruction set with global metadata
//! \param state the VM state to read from
//! \param destBuffer the destination buffer
//! \param destBufferSize the size of the destination buffer
int ReadGlobalValue(
    GlobalBindPoint bindPoint,
    const Assembly& assembly,
    BsVmState& state,
    void* destBuffer,
    int   destBufferSize
);

//! Write a global to the VM state.
//! \param bindPoint the bind point of the global value to write to
//! \param assembly the assembly instruction set with global metadata
//! \param state the VM state to read from
//! \param srcBuffer the source of the buffer
//! \param srcBufferSize the size of the source buffer to read from
void WriteGlobalValue(
    GlobalBindPoint bindPoint,
    const Assembly& assembly,
    BsVmState& state,
    const void* srcBuffer,
    int srcBufferSize
);

}
}

#endif
