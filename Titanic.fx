


//--------------------------------------------------------------------------------------
// File: Tutorial0510.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//DEBUG
//fxc /Od /Zi /T fx_4_0 /Fo BasicHLSL10.fxo BasicHLSL10.fx

Texture2D txDiffuse0;
Texture2D txDiffuse1;
Texture2D shadowMap;


Texture2D DiffuseTextures[20];
Texture2D NormalTextures[20];
int texSelect;
int isAlpha;

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix View;
    matrix Projection;
    matrix World;

}
	float4 vLightDir[10];
	float4 vLightColor[10];
	float4 vOutputColor;
	int		texSelectIndex;
	float4x4 lightViewProj;


SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos		: POSITION;
	float4 Normal	: NORMAL;
	float2 Tex		: TEXCOORD;
	int TexNum	    : TEXNUM;
	float4 Tangent	: TANGENT;
	float4 BiNormal	: BINORMAL;
};

struct PS_INPUT
{
    float4 Pos		: SV_POSITION;
	float4 Normal	: NORMAL;
	float2 Tex		: TEXCOORD0;
	int TexNum      : TEXNUM;
	float4 Tangent	: TANGENT;
	float4 BiNormal	: BINORMAL;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
	PS_INPUT output = (PS_INPUT)0;
	   
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Normal = mul( input.Normal, World );
    output.Tex    = input.Tex;
	output.TexNum = input.TexNum;
	output.Tangent = mul( input.Tangent, World );
	output.BiNormal = mul( input.BiNormal, World );



    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
		return float4(1.0, 1.0, 1.0, 1.0);
        float4 LightColor = 0;

		float4 textureFinal = float4( 1.0,1.0,1.0,1.0 );
        
		float3 normMap;
		for( int i = 0; i < 10; i++ )
		{
			if( i == input.TexNum )
			{
				normMap = NormalTextures[i].Sample( samLinear, input.Tex ).xyz;
				normMap = ( normMap ) * 2.0f - 1.0f;
				break;
			}
			//normMap = input.Normal;
		}

		float3 N = input.Normal;
		float3 T = input.Tangent;
		float3 B = input.BiNormal;

		float3x3 TBN = float3x3( T, B, N );

		normMap = mul( normMap, TBN );
		normMap = normalize(normMap) ;

		float3 light;
        for(int i=0; i<3; i++)
        {
            LightColor += saturate( dot( (float3)vLightDir[i],normMap) * vLightColor[i]);
        }


	//	if( texSelect == input.TexNum)
	//		return float4( 0.0, 1.0, 0.0, 0.0 );


		if( texSelect == -2 )
			textureFinal = float4( 0.0, (1.0 -( (float)input.TexNum * .10)), 0.0, 1.0 );

			int texnum = input.TexNum;
		
		//quick hack to make to expand it to large values. change 10 if more than 10 tex on an object
		for( int i = 0; i < 10; i++ )
		{
			if( i == input.TexNum )
			{
				textureFinal = DiffuseTextures[i].Sample( samLinear, input.Tex )*LightColor;
				//textureFinal = NormalTextures[i].Sample( samLinear, input.Tex )*LightColor;
			}
		}

		if( isAlpha )
			clip( textureFinal.a - .9f );

		//if this is white you got issues
		return textureFinal;
		return float4( 1.0, 1.0, 1.0, 1.0 );

}


float4 normPS( PS_INPUT input) : SV_Target
{
        float4 LightColor = 0;

		float4 textureFinal = float4( 1.0,1.0,1.0,1.0 );

		float3 light;
        for(int i=0; i<3; i++)
        {
            LightColor += saturate( dot( (float3)vLightDir[i],input.Normal) * vLightColor[i]);
        }


		if( texSelect == input.TexNum)
			return float4( 0.0, 1.0, 0.0, 0.0 );


		if( texSelect == -2 )
			textureFinal = float4( 0.0, (1.0 -( (float)input.TexNum * .10)), 0.0, 1.0 );

			int texnum = input.TexNum;
		
		//quick hack to make to expand it to large values. change 10 if more than 10 tex on an object
		for( int i = 0; i < 10; i++ )
		{
			if( i == input.TexNum )
			{
				textureFinal = DiffuseTextures[i].Sample( samLinear, input.Tex )*LightColor;
			}
		}

		if( isAlpha )
			clip( textureFinal.a - .9f );

		//if this is white you got issues
		return textureFinal;
		return float4( 1.0, 1.0, 1.0, 1.0 );

}

//--------------------------------------------------------------------------------------
//technique10 Render
//{
//    pass P0
//    {
//        SetVertexShader( CompileShader( vs_4_0, VS() ) );
//        SetGeometryShader( NULL );
//        SetPixelShader( CompileShader( ps_4_0, PS() ) );
//    }
//}
//
////--------------------------------------------------------------------------------------
//technique10 RenderNormalMap
//{
//    pass P0
//    {
//        SetVertexShader( CompileShader( vs_4_0, VS() ) );
//        SetGeometryShader( NULL );
//        SetPixelShader( CompileShader( ps_4_0, normPS() ) );
//    }
//}
