// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#ifndef DEBUGMRTTEXTURE_VS30_H
#define DEBUGMRTTEXTURE_VS30_H

#include "shaderapi/ishaderapi.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/imaterialvar.h"

class debugmrttexture_vs30_Static_Index
{
public:
	debugmrttexture_vs30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_debugmrttexture_vs30 1


class debugmrttexture_vs30_Dynamic_Index
{
public:
	debugmrttexture_vs30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_debugmrttexture_vs30 1


#endif	// DEBUGMRTTEXTURE_VS30_H