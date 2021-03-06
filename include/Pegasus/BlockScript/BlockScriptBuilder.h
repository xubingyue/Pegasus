/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   BlockScriptBuilder.h
//! \author Kleber Garcia
//! \date   28th August 2014
//! \brief  Blockscript abstract syntax tree builder, also type checker. 
//!         returns respective runtime ready structures to run the script.

#ifndef BLOCK_SCRIPT_BUILDER_H
#define BLOCK_SCRIPT_BUILDER_H

#include "Pegasus/BlockScript/SymbolTable.h"
#include "Pegasus/BlockScript/TypeTable.h"
#include "Pegasus/BlockScript/FunTable.h"
#include "Pegasus/BlockScript/StackFrameInfo.h"
#include "Pegasus/BlockScript/Container.h"
#include "Pegasus/BlockScript/Canonizer.h"
#include "Pegasus/BlockScript/IddStrPool.h"
#include "Pegasus/BlockScript/BlockScriptCanon.h"
#include "Pegasus/Memory/BlockAllocator.h"

namespace Pegasus
{


    //forward declarations begin
    namespace BlockScript
    {
        namespace Ast
        {
        //fwd declarations
        #define BS_PROCESS(N) class N;
        #include "Pegasus/BLockScript/Ast.inl"
        #undef BS_PROCESS
        union Variant;
        struct IddMetaData;
        }

    

        class IBlockScriptCompilerListener;
    }

    namespace Alloc
    {
    class IAllocator; 
    }
    //forward declarations end

namespace BlockScript
{


//! Block Script Builder, type checker and constructor of script abstract syntax tree
class BlockScriptBuilder
{
public:
    explicit BlockScriptBuilder() 
        : mCurrentFrame(nullptr)
        , mErrorCount(0)
        , mInFunBody(false)
        , mReturnTypeContext(nullptr)
        , mCurrAnnotations(nullptr)
        , mScanner(nullptr) {}
	
    struct CompilationResult
    {
        Ast::Program*    mAst;
        Assembly         mAsm;
    };

    void Initialize(Pegasus::Alloc::IAllocator* allocator);
    ~BlockScriptBuilder(){}

    //! Begins construction of abstract syntax tree
    void BeginBuild(const char* title);

    //! Ends construction of abstract syntax tree
    void EndBuild  (CompilationResult& r);

    //! destroys memory of compilation results
    void Reset ();

    //! true if we can start a new function. False otherwise
    bool StartNewFunction(const TypeDesc* returnType);

    //! Creation functions, of node general containers
    Ast::Program*  CreateProgram();
    Ast::ExpList*  CreateExpList();
    Ast::ArgList*  CreateArgList();
    Ast::StmtList* CreateStmtList();

    //! Node builders
    Ast::Exp* BuildBinop    (Ast::Exp* lhs, int op, Ast::Exp* rhs);
    Ast::Exp* BuildSetBinop (Ast::Exp* lhs, Ast::Exp* rhs, bool isExtern = false);
    Ast::Exp* BuildUnop  (int op, Ast::Exp* exp);
    Ast::Exp* BuildUnopPost  (Ast::Exp* exp, int op); //for post increment
    Ast::Exp* BuildExplicitCast  (Ast::Exp* exp, const TypeDesc* type);
    Ast::Exp* BuildFunCall(Ast::ExpList* args, const char * name, bool isMethod = false);
    Ast::Exp* BuildMethodCall(Ast::Exp* caller, const char * name, Ast::ExpList* args);
    Ast::Exp*   BuildImmFloat    (float v);
    Ast::Exp*   BuildImmInt      (int   v);
    Ast::Exp*   BuildIdd   (const char * name);
    Ast::StmtExp* BuildStmtExp(Ast::Exp* exp);
    Ast::StmtExp* BuildExternVariable(Ast::Exp* lhs, Ast::Exp* rhs);
    Ast::StmtExp* BuildDeclarationWithAnnotation(Ast::Annotations* ann, Ast::Exp* exp);
    Ast::StmtReturn* BuildStmtReturn(Ast::Exp* exp);
    Ast::StmtWhile*  BuildStmtWhile(Ast::Exp* exp, Ast::StmtList* stmtList);
    Ast::StmtFor*    BuildStmtFor(Ast::Exp* init, Ast::Exp* cond, Ast::Exp* update, Ast::StmtList* stmtList);
    Ast::StmtFunDec* BuildStmtFunDec(Ast::ArgList* argList, const TypeDesc* returnType, const char * nameIdd);
    Ast::StmtFunDec* BindFunImplementation(Ast::StmtFunDec* funDec, Ast::StmtList* stmts);
    Ast::StmtIfElse* BuildStmtIfElse(Ast::Exp* exp, Ast::StmtList* ifBlock, Ast::StmtIfElse* tail, StackFrameInfo* frame);
    Ast::Exp*        BuildStaticArrayDec(const TypeDesc* arrayType);
    Ast::StmtStructDef* BuildStmtStructDef(const char* name, Ast::ArgList* definitions);
    Ast::StmtEnumTypeDef* BuildStmtEnumTypeDef(const TypeDesc* type);
    Ast::ArgDec* BuildArgDec(const char* var, const TypeDesc* type);
    Ast::Exp* BuildStrImm(const char* strToCopy);

    void IncErrorCount() { ++mErrorCount; }

    int GetErrorCount() const { return mErrorCount; }

    StackFrameInfo* StartNewFrame();

    void PopFrame();

    FunTable* GetFunTable() { return &mFunTable; }

    void BindIntrinsic(Ast::StmtFunDec* funDec, FunCallback callback);

    IddStrPool& GetStringPool() { return mStrPool; }

    char* AllocateBigString(int size);

    int GetCurrentLine() const;

    const char* GetCurrentCompilationUnitTitle() const;

    void PushFile(const char* newFileTitle);

    void PopFile();

    void IncrementLine(); 

    void AddEventListener(IBlockScriptCompilerListener* eventListener) { mEventListeners.PushEmpty() = eventListener; }

    Container<IBlockScriptCompilerListener*>& GetEventListeners() { return mEventListeners; }

    const TypeDesc* GetTypeByName(const char* name) const;

    TypeDesc* GetTypeByName(const char* name);

    FunDesc* FindFunctionDescription(Ast::FunCall* fcSignature);

    FunDesc* RegisterFunctionDeclaration(Ast::StmtFunDec* funDec);

    Ast::Annotations* BeginAnnotations();
    
    Ast::Annotations* EndAnnotations(Ast::Annotations* annotations, Ast::ExpList* exps);

    SymbolTable* GetSymbolTable() { return &mSymbolTable; }

    const SymbolTable* GetSymbolTable() const { return &mSymbolTable; }

    const char* AllocStrImm(const char* strToCpy);    

    void RegisterExternGlobal(Ast::Idd* var, Ast::Imm* defaultVal);

    //! creates an intrinsic function that can be called from blockscript
    //! \param the function name
    //! \param a string list with argument types definitions
    //! \param a string list of the argument names.
    //! \param a string with the return type
    //! \param callback the actual c++ callback
    //! \param isMethod - if true, it means that the function definition is a method (first artType must be an object).
    //!                   this means that the -> notation will be used                        
    //! \note  function asserts if it fails
    void CreateIntrinsicFunction(
        const char* funName, 
        const char* const* argTypes, 
        const char* const* argNames, 
        int argCount, 
        const char* returnType, 
        FunCallback callback,
        bool isMethod = false
    );

    //! copies a foreign string into the blockscripts script pool (memory allocation)
    //! \param the source string
    //! \return the return parameter
    char* CopyString(const char* source);

    void  SetScanner(void* scanner) { mScanner = scanner; }
    void* GetScanner() { return mScanner; }

    Pegasus::Alloc::IAllocator* GetAllocator() const { return mGeneralAllocator; }

private:

    // registers a member into the stack. Returns the offset of the current stack frame.
    //! returns the offset of such member
    int RegisterStackMember(const char* name, const TypeDesc* type);

    //! builds a binary operator for array access. Determines whether its an array or not.
    //! returns the expression corresponding to such array element.
    Ast::Exp* BuildBinopArrayAccess(Ast::Exp* lhs, int op, Ast::Exp* rhs); 

    //! \return true if operation is valid for this type, false otherwise
    bool IsBinopValid(const TypeDesc* type, int op);

    //! Is this in an annotation context?
    bool IsInAnnotation() const { return mCurrAnnotations != nullptr; }
    
    //! Attempts a type promotion. Creates an implicit cast if successful
    //! \param exp the expression
    //! \param targetType the target type to promote to.
    //! \return a new expression if success, otherwise returns the same expression passed.
    Ast::Exp* AttemptTypePromotion(Ast::Exp* exp, const TypeDesc* targetType);


    Pegasus::Alloc::IAllocator* mGeneralAllocator;
    Memory::BlockAllocator      mAllocator;
    FunTable           mFunTable;
	CompilationResult  mActiveResult;
    IddStrPool         mStrPool;
    SymbolTable        mSymbolTable;
    const TypeDesc*    mReturnTypeContext;

    void* mScanner;
    StackFrameInfo*    mCurrentFrame;
    int                mErrorCount;

    Canonizer mCanonizer;

    Container<IBlockScriptCompilerListener*> mEventListeners;
    Container<GlobalMapEntry> mGlobalsMap;
    Container<Ast::IddMetaData*> mGlobalsMetaData;

    Ast::Annotations* mCurrAnnotations;

    bool mInFunBody;

    struct FileState
    {
        const char* compilationUnitTitle;
        int lineNumber;

        FileState()
            : compilationUnitTitle("No-Title"), lineNumber(0)
        {
        }
    };

    Container<FileState> mFileStates;



};

}
}
#endif
