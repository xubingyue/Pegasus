/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	GeometryTestBlock.cpp
//! \author	Kleber Garcia
//! \date	16th June 2014
//! \brief	Geometry test

#include "Apps/TestApp1/TimelineBlocks/GeometryTestBlock.h"
#include "../Source/Pegasus/Render/GL/GLEWStaticInclude.h"

#include "Pegasus/Math/Quaternion.h"
#include "Pegasus/Mesh/Generator/IcosphereGenerator.h"

#if PEGASUS_GAPI_GL

#define GEOMTESTBLOCK_SHADER_ROOT "Shaders\\glsl"

#elif PEGASUS_GAPI_DX

#define GEOMTESTBLOCK_SHADER_ROOT "Shaders\\hlsl"

#endif


//Constructor
GeometryTestBlock::GeometryTestBlock(Pegasus::Alloc::IAllocator * allocator, Pegasus::Wnd::IWindowContext * appContext)
    : Pegasus::Timeline::Block(allocator, appContext)

{

}

//Destructor
GeometryTestBlock::~GeometryTestBlock()
{

}

//! Initializer
void GeometryTestBlock::Initialize()
{

    //setup raster states
    Pegasus::Render::RasterizerConfig rasterConfig;
    rasterConfig.mCullMode = Pegasus::Render::RasterizerConfig::CW_CM;
    rasterConfig.mDepthFunc = Pegasus::Render::RasterizerConfig::GREATER_DF;

    Pegasus::Render::CreateRasterizerState(rasterConfig, mCurrentBlockRasterState);
    rasterConfig.mCullMode = Pegasus::Render::RasterizerConfig::NONE_CM;
    rasterConfig.mDepthFunc = Pegasus::Render::RasterizerConfig::NONE_DF;

    Pegasus::Render::CreateRasterizerState(rasterConfig, mDefaultRasterState);
    
    // setup shaders
    Pegasus::Shader::ShaderManager * const shaderManager = GetShaderManager();
    mBlockProgram = shaderManager->CreateProgram("CubeProgram");
    
    Pegasus::Shader::ShaderStageFileProperties fileLoadProperties;
    fileLoadProperties.mLoader = GetIOManager();
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\Cube.vs";
    mBlockProgram->SetShaderStage(  
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );

    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\Cube.ps";
    mBlockProgram->SetShaderStage(  
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );

    mDiscoSpeaker = shaderManager->CreateProgram("DiscoSpeaker");
    
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\DiscoSpeaker.vs";
    mDiscoSpeaker->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\DiscoSpeaker.ps";
    mDiscoSpeaker->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );

    mBlurHorizontal = shaderManager->CreateProgram("BlurHorizontal");
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\DiscoSpeaker.vs";
    mBlurHorizontal->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\BlurHorizontal.ps";
    mBlurHorizontal->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );

    mBlurVertical = shaderManager->CreateProgram("BlurVertical");
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\DiscoSpeaker.vs";
    mBlurVertical->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\BlurVertical.ps";
    mBlurVertical->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );

    mComposite = shaderManager->CreateProgram("mComposite");
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\DiscoSpeaker.vs";
    mComposite->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );
    fileLoadProperties.mPath = GEOMTESTBLOCK_SHADER_ROOT"\\Composite.ps";
    mComposite->SetShaderStage(
        shaderManager->LoadShaderStageFromFile(fileLoadProperties)
    );

    bool updated = false; 
    mBlockProgram->GetUpdatedData(updated);
    updated = false;
    mDiscoSpeaker->GetUpdatedData(updated);

    // setup uniforms
    Pegasus::Render::GetUniformLocation(mBlockProgram, "cubeTex", mCubeTextureUniform);
    Pegasus::Render::GetUniformLocation(mBlockProgram, "uniformState", mUniformBlock);
    Pegasus::Render::CreateUniformBuffer(sizeof(mState), mUniformStateBuffer);
    Pegasus::Math::SetIdentity(mState.mRotation);

    Pegasus::Render::GetUniformLocation(mDiscoSpeaker, "aspect",   mAspectUniform);
    Pegasus::Render::GetUniformLocation(mDiscoSpeaker, "time",   mTimeUniform);

    Pegasus::Render::GetUniformLocation(mComposite, "inputTexture1", mCompositeInput1);
    Pegasus::Render::GetUniformLocation(mComposite, "inputTexture2", mCompositeInput2);
    Pegasus::Render::GetUniformLocation(mBlurHorizontal, "inputTexture", mHorizontalInput);
    Pegasus::Render::GetUniformLocation(mBlurVertical, "inputTexture", mVerticalInput);

    //setup meshes
    Pegasus::Mesh::MeshManager * const meshManager = GetMeshManager();
    mCubeMesh = meshManager->CreateMeshNode();
    mCubeMesh->SetGeneratorInput(
        meshManager->CreateMeshGeneratorNode("BoxGenerator")
    );

    mQuad = meshManager->CreateMeshNode();
    mQuad->SetGeneratorInput(
        meshManager->CreateMeshGeneratorNode("QuadGenerator")
    );
    int Degrees[MAX_SPHERES] = {3,2,1,1};
    for (int i = 0; i < MAX_SPHERES; ++i)
    {
        mSphereMeshes[i] = meshManager->CreateMeshNode();
        Pegasus::Mesh::MeshGeneratorRef generator = meshManager->CreateMeshGeneratorNode("IcosphereGenerator");
        Pegasus::Mesh::IcosphereGenerator * icosphere = static_cast<Pegasus::Mesh::IcosphereGenerator*>(generator);
        icosphere->SetDegree(Degrees[i]);
        icosphere->SetRadius( 0.45f);
        mSphereMeshes[i]->SetGeneratorInput(
            generator
        );
        //force update
        mSphereMeshes[i]->GetUpdatedMeshData();
    }

    //setup render targets
    Pegasus::Render::RenderTargetConfig c;
    c.mWidth = 512;
    c.mHeight = 512;
    Pegasus::Render::CreateRenderTarget(c, mCubeFaceRenderTarget);

    c.mWidth = 1280;
    c.mHeight = 720;
    Pegasus::Render::CreateRenderTarget(c, mTempTarget1);
    c.mWidth /= 4;
    c.mHeight /= 4;
    Pegasus::Render::CreateRenderTarget(c, mTempTarget2);
    Pegasus::Render::CreateRenderTarget(c, mTempTarget3);
}

//! Shutdown used by block
void GeometryTestBlock::Shutdown()
{
    Pegasus::Render::DeleteBuffer(mUniformStateBuffer);
    Pegasus::Render::DeleteRenderTarget(mTempTarget1);
    Pegasus::Render::DeleteRenderTarget(mTempTarget2);
    Pegasus::Render::DeleteRenderTarget(mTempTarget3);
    Pegasus::Render::DeleteRenderTarget(mCubeFaceRenderTarget);
    Pegasus::Render::DeleteRasterizerState(mCurrentBlockRasterState);
    Pegasus::Render::DeleteRasterizerState(mDefaultRasterState);
}

//! Render function
void GeometryTestBlock::Render(float beat, Pegasus::Wnd::Window * window)
{

    //figure out aspect ratio
    unsigned int viewportWidth = 0;
    unsigned int viewportHeight = 0;
    window->GetDimensions(viewportWidth, viewportHeight);
    float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

    ///////////////////////////////////////////////
    //******** PASS 1: texture of box ***********//
    ///////////////////////////////////////////////
    // rendering to the face of the disco a normal map and a detail map
    Pegasus::Render::Dispatch(mCubeFaceRenderTarget); //use the fractal surface, no depth
    Pegasus::Render::Clear(/*color*/true, /*depth*/false, /*stencil*/false); //clear the surface
    Pegasus::Render::Dispatch(mDiscoSpeaker); // dispatch the shader
    Pegasus::Render::Dispatch(mQuad); // screen space

    Pegasus::Render::SetUniform(mAspectUniform, aspect);
    Pegasus::Render::SetUniform(mTimeUniform, beat);

    Pegasus::Render::Draw();

    ///////////////////////////////////////////////
    //******** PASS 2: scene          ***********//
    ///////////////////////////////////////////////
    Pegasus::Render::Dispatch(mTempTarget1);
    Pegasus::Render::Clear(/*color*/true, /*depth*/false, /*stencil*/false); //clear the surface
    Pegasus::Render::Dispatch(mBlockProgram);

    Pegasus::Math::Vec3 rotAxis(0.3f, 0.4f, 0.01f);
    Pegasus::Math::SetRotation(mState.mRotation, rotAxis, (float)beat * 0.92f);

    // do aspect ratio correctness on y axis
    mState.mRotation.m21 *= aspect;
    mState.mRotation.m22 *= aspect;
    mState.mRotation.m23 *= aspect;

    Pegasus::Render::SetBuffer(mUniformStateBuffer, &mState, sizeof(mState));
    Pegasus::Render::SetUniform(mUniformBlock, mUniformStateBuffer);
    Pegasus::Render::SetUniform(mCubeTextureUniform, mCubeFaceRenderTarget);

    Pegasus::Render::SetRasterizerState(mCurrentBlockRasterState);

    Pegasus::Render::Dispatch(mCubeMesh);
    Pegasus::Render::Draw();

    for (int i = 0; i < MAX_SPHERES; ++i)
    {
        Pegasus::Render::Dispatch(mSphereMeshes[i]);

        float fi = 2.0f * ((float)i / (float)MAX_SPHERES) - 1.0f;
        
        static const int maxDepth = 4;
        for (int j = 0; j < maxDepth; ++j)
        {
            float fj = 2.0f * ((float)j / (float)maxDepth) - 1.0f; 
            float ox = 2.7f*Pegasus::Math::Cos(beat * 0.6f + 4.7f*fj * fi);
            float oy = 2.7f*Pegasus::Math::Sin(beat * 0.6f - 6.0f*fj + 3.0f*fi);
            mState.mRotation.m14 = 6.0f * fi + ox;
            mState.mRotation.m24 = 6.0f * fj + oy;
            mState.mRotation.m34 = 0.5;
            mState.mRotation.m44 = 1.0f;
            Pegasus::Render::SetBuffer(mUniformStateBuffer, &mState, sizeof(mState));
            Pegasus::Render::Draw();
        }
    }
    
    ///////////////////////////////////////////////
    //******** PASS 3: horizontal blur***********//
    ///////////////////////////////////////////////
    Pegasus::Render::Dispatch(mTempTarget2);
    Pegasus::Render::Dispatch(mBlurHorizontal);
    Pegasus::Render::Dispatch(mQuad);
    Pegasus::Render::SetUniform(mHorizontalInput, mTempTarget1);
    Pegasus::Render::Draw();
    
    ///////////////////////////////////////////////
    //******** PASS 4: vertical   blur***********//
    ///////////////////////////////////////////////
    Pegasus::Render::Dispatch(mTempTarget3);
    Pegasus::Render::Dispatch(mBlurVertical);
    Pegasus::Render::Dispatch(mQuad);
    Pegasus::Render::SetUniform(mVerticalInput, mTempTarget2);
    Pegasus::Render::Draw();

    ///////////////////////////////////////////////
    //******** PASS 5: final compositing ********//
    ///////////////////////////////////////////////
    Pegasus::Render::DispatchDefaultRenderTarget(Pegasus::Render::Viewport(viewportWidth, viewportHeight));
    Pegasus::Render::Dispatch(mComposite);
    Pegasus::Render::Dispatch(mQuad);
    Pegasus::Render::SetUniform(mCompositeInput1, mTempTarget1); //original scene
    Pegasus::Render::SetUniform(mCompositeInput2, mTempTarget3); //bloomed scene
    Pegasus::Render::Draw();

    Pegasus::Render::SetRasterizerState(mDefaultRasterState);

}
