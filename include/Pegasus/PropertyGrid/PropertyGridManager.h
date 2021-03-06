/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	PropertyGridManager.h
//! \author	Karolyn Boulanger
//! \date	28th January 2015
//! \brief	Manager for all classes using a property grid, with all the member information

#ifndef PEGASUS_PROPERTYGRID_PROPERTYGRIDMANAGER_H
#define PEGASUS_PROPERTYGRID_PROPERTYGRIDMANAGER_H

#include "Pegasus/PropertyGrid/PropertyGridStaticAllocator.h"
#include "Pegasus/PropertyGrid/PropertyGridClassInfo.h"
#include "Pegasus/PropertyGrid/Proxy/PropertyGridManagerProxy.h"
#include "Pegasus/Utils/DependsOnStatic.h"
#include "Pegasus/Utils/Vector.h"
#include "Pegasus/PropertyGrid/PropertyGridEnumType.h"

namespace Pegasus {
namespace PropertyGrid {

class BaseEnumType;

//! Manager for all classes using a property grid, with all the member information.
//! This manager is a static singleton, and contains all the information
//! about all the classes that use a property grid
class PropertyGridManager : public Utils::DependsOnStatic<PropertyGridStaticAllocator, PropertyGridStaticAllocator::GetInstance>
{
public:

    //! Constructor
    PropertyGridManager();

    //! Get the unique instance of the property grid manager
    //! \return Reference to the unique instance of the property grid manager
    static PropertyGridManager & GetInstance();

    //! Register a class, and tell to the manager that all following property declarations
    //! are for that class
    //! \param className Name of the class containing the properties
    //! \param parentClassName Name of the parent of the class, empty string if the class is a base class
    //! \note Called only by the \a BEGIN_DECLARE_PROPERTIES() macro
    //! \warning \a EndDeclareProperties() has to be called once the properties are done declaring
    //!          for the current class
    void BeginDeclareProperties(const char * className, const char * parentClassName);

    //! Declare a new property for the current class
    //! \param type Type of the property, PROPERTYTYPE_xxx constant
    //! \param size Size in bytes of the property (> 0)
    //! \param name Name of the property, starting with an uppercase letter (non-empty)
    //! \param defaultValuePtr Pointer to the default value of the property
    //! \note Called only by the \a DECLARE_PROPERTY() macro
    //! \warning \a BeginDeclareProperties() has to be called before this function
    void DeclareProperty(PropertyType type, int size, const char * name, const char* typeName, void * defaultValuePtr);

    //! Finish registering a class properties
    //! \param className Name of the class containing the properties
    //! \note Called only by the \a END_DECLARE_PROPERTIES() macro
    //! \warning Has to be called after a set of calls to \a DeclareProperty()
    void EndDeclareProperties();


    //! Get the number of registered classes that contain a property grid
    //! \return Number of successfully registered classes
    inline unsigned int GetNumRegisteredClasses() const { return mClassInfos.GetSize(); }

    //! Get one of the registered classes by index
    //! \param index Index of the class to get info from (< GetNumRegisteredClasses())
    //! \return Information about the registered class
    const PropertyGridClassInfo & GetClassInfo(unsigned int index) const;

    //! Get one of the registered classes by name
    //! \param className Name of the class to get info from (non-empty string)
    //! \return Information about the registered class, nullptr if not found
    //! \note An assertion is thrown if the class is not found
    //! \note That function is slower than the index-based one,
    //!       as it has to perform a search
    const PropertyGridClassInfo * GetClassInfo(const char * className) const;

    //! Must get called at the initialization of main() once. This will ensure all the metadata of class
    //! dependencies are linked properly. Otherwise, no parent-child relationship will exist
    void ResolveInternalClassHierarchy();

    //! Called when an enumeration is started for registration.
    //! \param enumName, the enumeration name to hold the record values.
    void BeginDeclareEnum(const char* enumName);

    //! Called when an enumeration is registered.
    //! \param enumValue the enumeration value stored.
    void RegisterEnum(const BaseEnumType* enumValue);

    //! Ends enumeration registration
    void EndDeclareEnum();
    
    //! Gets a pointer to the enumeration info.
    //! \param name of the enumeration type
    //! \return name of the enumeration type info.
    const EnumTypeInfo* GetEnumInfo(const char* enumName) const;

    //! Gets a pointer to the enumeration info.
    //! \param index of the enumeration type
    //! \return name of the enumeration type info.
    const EnumTypeInfo* GetEnumInfo(int index) const { return &mEnumInfos[index]; }

    //! Gets the enumeration records
    //! \return the enumeration type records
    unsigned int GetNumRegisteredEnumInfos() const { return mEnumInfos.GetSize(); }

#if PEGASUS_ENABLE_PROXIES

    //! Get the proxy associated with the property grid manager
    //! \return Proxy associated with the property grid manager
    //@{
    inline PropertyGridManagerProxy * GetProxy() { return &mProxy; }
    inline const PropertyGridManagerProxy * GetProxy() const { return &mProxy; }
    //@}

#endif  // PEGASUS_ENABLE_PROXIES

    //------------------------------------------------------------------------------------
    
private:

    //! Destructor
    virtual ~PropertyGridManager();

    // No copies allowed
    PG_DISABLE_COPY(PropertyGridManager);

    //! List of information structures about registered classes
    Utils::Vector<PropertyGridClassInfo> mClassInfos;

    //! List of information structures of enumerations
    Utils::Vector<EnumTypeInfo> mEnumInfos;

    //! Class information currently being edited
    //! \note Set by \a BeginDeclareProperties(), unset by \a EndDeclareProperties()
    PropertyGridClassInfo * mCurrentClassInfo;
    
    //! Current enum info to fill in when registering enums
    EnumTypeInfo* mCurrentEnumInfo;


#if PEGASUS_ENABLE_PROXIES

    //! Proxy associated with the property grid manager
    PropertyGridManagerProxy mProxy;

#endif  // PEGASUS_ENABLE_PROXIES
};


}   // namespace PropertyGrid
}   // namespace Pegasus

#endif  // PEGASUS_PROPERTYGRID_PROPERTYGRIDMANAGER_H
