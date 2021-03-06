/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   GLEWStaticInclude.cpp
//! \author David Worsham
//! \date   02 Nov 2013
//! \brief  Static include header for GLEW.

#if PEGASUS_GAPI_GL
#include "../Source/Pegasus/Render/GL/GLEWStaticInclude.h"
#include "Pegasus/Libs/GLEW/glew.c"
#else
PEGASUS_AVOID_EMPTY_FILE_WARNING
#endif
