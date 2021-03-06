/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	OutputNode.cpp
//! \author	Karolyn Boulanger
//! \date	01st November 2013
//! \brief	Base output node class, for the root of the graphs

#include "Pegasus/Graph/OutputNode.h"
#include "Pegasus/AssetLib/Asset.h"

namespace Pegasus {
namespace Graph {


BEGIN_IMPLEMENT_PROPERTIES(OutputNode)
END_IMPLEMENT_PROPERTIES(OutputNode)

//----------------------------------------------------------------------------------------

OutputNode::OutputNode(NodeManager* nodeManager, Alloc::IAllocator* nodeAllocator, Alloc::IAllocator* nodeDataAllocator)
:   Node(nodeAllocator, nodeDataAllocator), AssetLib::RuntimeAssetObject(this), mNodeManager(nodeManager)
{
    BEGIN_INIT_PROPERTIES(OutputNode)
    END_INIT_PROPERTIES()

#if PEGASUS_ENABLE_PROXIES
    mNodeType = NODETYPE_OUTPUT;
#endif  // PEGASUS_ENABLE_PROXIES
}

//----------------------------------------------------------------------------------------

bool OutputNode::Update()
{
    // Check that no data is allocated
    PG_ASSERTSTR(!IsDataAllocated(), "Invalid output node, it should not contain NodeData");

    // Check that the input node is defined
    if (GetNumInputs() == 1)
    {
        // Update the input node and return its dirty state
        return GetInput(0)->Update();
    }
    else
    {
        // If no input is found, we consider there is no node data to update
        PG_FAILSTR("Invalid output node, it does not have an input defined");
        return false;
    }
}

//----------------------------------------------------------------------------------------
    
NodeDataReturn OutputNode::GetUpdatedData(bool & updated)
{
    // Check that no data is allocated
    PG_ASSERTSTR(!IsDataAllocated(), "Invalid output node, it should not contain NodeData");

    // Check that the input node is defined
    if (GetNumInputs() == 1)
    {
        // Redirect the updated data from the input node
        return GetInput(0)->GetUpdatedData(updated);
    }
    else
    {
        PG_FAILSTR("Invalid output node, it does not have an input defined");
        return nullptr;
    }
}

//----------------------------------------------------------------------------------------

NodeDataReturn OutputNode::GetUpdatedData()
{
    bool updated = false;
    return GetUpdatedData(updated);
}

//----------------------------------------------------------------------------------------
    
OutputNode::~OutputNode()
{
}

//----------------------------------------------------------------------------------------

NodeData * OutputNode::AllocateData() const
{
    PG_FAILSTR("Output nodes do not have data, so there is nothing to allocate");
    return nullptr;
}

//----------------------------------------------------------------------------------------

void OutputNode::GenerateData()
{
    PG_FAILSTR("Output nodes do not have data, so there is nothing to generate");
}

//----------------------------------------------------------------------------------------

void OutputNode::AddInput(NodeIn inputNode)
{
    // Only one input node accepted
    if (GetNumInputs() != 0)
    {
        PG_FAILSTR("Output nodes are not allowed to have multiple input nodes");
        return;
    }

    Node::AddInput(inputNode);
}

//----------------------------------------------------------------------------------------

void OutputNode::ReplaceInput(unsigned int index, const Pegasus::Core::Ref<Node> & inputNode)
{
    // An input node must be present already, and at most one can be defined
    if (index != 0)
    {
        PG_FAILSTR("Only the first input node can be replaced for an output node");
        return;
    }
    if (GetNumInputs() != 1)
    {
        PG_FAILSTR("Cannot replace the input node as it has not been added yet");
        return;
    }

    Node::ReplaceInput(0, inputNode);
}

//----------------------------------------------------------------------------------------

void OutputNode::OnRemoveInput(unsigned int index)
{
}

//----------------------------------------------------------------------------------------

bool OutputNode::OnReadAsset(AssetLib::AssetLib* lib, const AssetLib::Asset* asset)
{
    return ReadFromObject(mNodeManager, asset, asset->Root());
}

//----------------------------------------------------------------------------------------

void OutputNode::OnWriteAsset(AssetLib::AssetLib* lib, AssetLib::Asset* asset)
{
    WriteToObject(asset, asset->Root());
}

}   // namespace Graph
}   // namespace Pegasus
