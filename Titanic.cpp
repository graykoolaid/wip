//--------------------------------------------------------------------------------------
// File: Titanic.cpp
//
// This application demonstrates animation using matrix transformations
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <xnamath.h>
#include <vector>
#include "resource.h"
#include "importer.h"
#include "fbxImporter.cpp"

using namespace std;

//--------------------------------------------------------------------------------------
// structures
//--------------------------------------------------------------------------------------


struct Camera
{
	D3DXVECTOR3 eye;
	D3DXVECTOR3 at;
	D3DXVECTOR3 up;
}Cam;

struct Object
{
	int numMeshes;
	int alpha;
	vector<Vertex>		vertices;
	ID3D10Buffer*		vertexBuffer;
	vector<ID3D10ShaderResourceView*> texArray;
	vector<ID3D10ShaderResourceView*> NormArray;
};
// Setup our lighting parameters
vector<D3DXVECTOR4> vLightDirs;
vector<D3DXVECTOR4> vLightColor;

vector<Vertex> vertices_vec;
vector<int> indices_vec;

vector<Object> characters;
Object terrain;

XMFLOAT3 tempVec;
XMFLOAT4 tempCol;

Vertex temp;

int gWidth;
int gHeight;
int texdex = -1;
int characterIndex = -1;

float moveUnit = .25;

UINT stride = sizeof( Vertex );
UINT offset = 0;
HRESULT hr = S_OK;


D3DXVECTOR4 COLOR = D3DXVECTOR4( .5, .5, .5, 1.0 );
//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                   g_hInst = NULL;
HWND                        g_hWnd = NULL;
D3D10_DRIVER_TYPE           g_driverType = D3D10_DRIVER_TYPE_NULL;
D3D10_RASTERIZER_DESC		raster;
ID3D10RasterizerState*		pState;
ID3D10Device*               g_pd3dDevice = NULL;
IDXGISwapChain*             g_pSwapChain = NULL;
ID3D10RenderTargetView*     g_pRenderTargetView = NULL;
ID3D10Texture2D*            g_pDepthStencil = NULL;
ID3D10DepthStencilView*     g_pDepthStencilView = NULL;
ID3D10Effect*               g_pEffect = NULL;
ID3D10EffectTechnique*      g_pTechnique = NULL;
ID3D10EffectTechnique*      g_pShadowMapTechnique = NULL;
ID3D10InputLayout*          g_pVertexLayout = NULL;
ID3D10Buffer*               g_pVertexBuffer = NULL;
ID3D10Buffer*               g_pIndexBuffer = NULL;

ID3D10ShaderResourceView*   g_pTextureRV = NULL;
ID3D10ShaderResourceView*   g_pTexture2RV = NULL;
ID3D10ShaderResourceView**  g_pTexArray = NULL;
ID3D10ShaderResourceView**  g_pNormArray = NULL;

ID3D10EffectMatrixVariable* g_pWorldVariable = NULL;
ID3D10EffectMatrixVariable* g_pViewVariable = NULL;
ID3D10EffectMatrixVariable* g_pProjectionVariable = NULL;

ID3D10EffectScalarVariable* g_ptextureSelectVariable = NULL;
ID3D10EffectScalarVariable* g_ptextureAlphaVariable = NULL;

ID3D10EffectVectorVariable* g_pLightDirVariable = NULL;
ID3D10EffectVectorVariable* g_pLightColorVariable = NULL;
ID3D10EffectVectorVariable* g_pOutputColorVariable = NULL;


ID3D10EffectShaderResourceVariable* g_pDiffuseVariable = NULL;
ID3D10EffectShaderResourceVariable* g_pDiffuseVariable2 = NULL;
ID3D10EffectShaderResourceVariable* g_ptextureArrayPtr = NULL;
ID3D10EffectShaderResourceVariable* g_pNormalArrayPtr = NULL;


D3DXMATRIX                  g_World1;
D3DXMATRIX                  g_View;
D3DXMATRIX                  g_Projection;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );

	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
		return 0;

	if( FAILED( InitDevice() ) )
	{
		CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			Render();
		}
	}

	CleanupDevice();

	return ( int )msg.wParam;
}


//----------------------------------------------------------------------------------
// we do camera math here eventually
//-----------------------------------------------------------------------------------
void camfunc()
{
	// Initialize the view matrix
	D3DXVECTOR3 Eye( Cam.eye.x, Cam.eye.y, Cam.eye.z );
	D3DXVECTOR3 At ( Cam.at.x,  Cam.at.y,  Cam.at.z  );
	D3DXVECTOR3 Up ( Cam.up.x,  Cam.up.y,  Cam.up.z  );
	D3DXMatrixLookAtLH( &g_View, &Eye, &At, &Up );

	// Initialize the projection matrix
	D3DXMatrixPerspectiveFovLH( &g_Projection, ( float )D3DX_PI * 0.25f, gWidth / ( FLOAT )gHeight, 0.1f, 1000.0f );

	return;
}

//---------------------------------------------------------------------------------------
// Terrain in flat plane form
//---------------------------------------------------------------------------------------
void createPlane()
{
	vector<D3DXVECTOR3> vertices;
	vector<D3DXVECTOR4> normals;
	vector<int> texNo;

	Object tempO;
	Vertex tempV;

	vector<Vertex> vertexs;
	vector<D3DXVECTOR2> texes;

	//for( int i = -100; i < 100; i++ )
	//{	
	//	for( int j = -100; j < 100; j++ )
	//	{
	//			vertices.push_back( D3DXVECTOR3( (float)i, -0.01, (float)j ) );
	//			vertices.push_back( D3DXVECTOR3( (float)i, -0.01, (float)j+1. ) );
	//			vertices.push_back( D3DXVECTOR3( (float)i+1., -0.01, (float)j ) );
	//			
	//			vertices.push_back( D3DXVECTOR3( (float)i, -0.01, (float)j+1. ) );
	//			vertices.push_back( D3DXVECTOR3( (float)i+1., -0.01, (float)j+1. ) );
	//			vertices.push_back( D3DXVECTOR3( (float)i+1., -0.01, (float)j ) );

	//			texes.push_back( D3DXVECTOR2( ((float)i+100.)/200., ((float)j+100.)/200. ) );
	//			texes.push_back( D3DXVECTOR2( ((float)i+1+100.)/200., ((float)j+100.)/200. ) );
	//			texes.push_back( D3DXVECTOR2( ((float)i+100.)/200., ((float)j+1+100.)/200. ) );

	//			texes.push_back( D3DXVECTOR2( ((float)i+100.)/200., ((float)j+1+100.)/200. ) );
	//			texes.push_back( D3DXVECTOR2( ((float)i+1+100.)/200., ((float)j+1+100.)/200. ) );
	//			texes.push_back( D3DXVECTOR2( ((float)i+1+100.)/200., ((float)j+100.)/200. ) );
	//	}
	//}


	vertices.push_back( D3DXVECTOR3( (float)-100, -0.01, (float)-100 ) );
	vertices.push_back( D3DXVECTOR3( (float)-100, -0.01, (float)100 ) );
	vertices.push_back( D3DXVECTOR3( (float)100., -0.01, (float)-100 ) );

	vertices.push_back( D3DXVECTOR3( (float)-100, -0.01, (float)100 ) );
	vertices.push_back( D3DXVECTOR3( (float)100, -0.01, (float)100 ) );
	vertices.push_back( D3DXVECTOR3( (float)100, -0.01, (float)-100 ) );

	texes.push_back( D3DXVECTOR2( 0., 0.) );
	texes.push_back( D3DXVECTOR2( 0., 1. ) );
	texes.push_back( D3DXVECTOR2( 1., 0. ) );

	texes.push_back( D3DXVECTOR2( 0., 1.) );
	texes.push_back( D3DXVECTOR2( 1., 1. ) );
	texes.push_back( D3DXVECTOR2( 1., 0. ) );

	for( int i = 0; i < vertices.size(); i++ )
	{
		tempV.Pos = vertices[i];
		tempV.Tex = texes[i];
		tempV.Normal = D3DXVECTOR3( 0.0,1.0,0.0);
		tempV.texNum = -2;

		terrain.vertices.push_back(tempV);
	}

	terrain.numMeshes = 1;

	terrain.texArray.push_back( NULL );

	// Load the Vertex Buffer
	D3D10_BUFFER_DESC bd;

	ZeroMemory(&bd, sizeof(D3D10_BUFFER_DESC));
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( Vertex ) * terrain.vertices.size();
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &terrain.vertices[0];
	hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
	if( FAILED( hr ) )
		return;

	// Save the Vertex Buffer for easy access
	terrain.vertexBuffer = g_pVertexBuffer;

}

//---------------------------------------------------------------------------------------
// Character Loader here
//---------------------------------------------------------------------------------------
void charLoad( char* filename, vector<const wchar_t *> *textures, vector<const wchar_t *> *NormTextures )
{
	vector<D3DXVECTOR3> vertexices;
	vector<D3DXVECTOR3> normexices;
	vector<D3DXVECTOR2> texexices;
	vector<WORD> indexices;
	vector<int> texNo;
	int tempTexCount;

	vector<Vertex> verts;

	indices_vec.resize(0);
	vertices_vec.resize(0);
	
	Object tempO;
	characters.push_back( tempO );
	characterIndex = characters.size() - 1;
	tempTexCount = Import( filename, &characters[characterIndex].vertices );

	//characters[characterIndex].vertices = verts[0];
	
//	tempO.indices = indexices;
//	tempO.vertices  = tempO;

	// Load the Vertex Buffer
	D3D10_BUFFER_DESC bd;

	ZeroMemory(&bd, sizeof(D3D10_BUFFER_DESC));
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( Vertex ) * characters[characterIndex].vertices.size();
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &characters[characterIndex].vertices[0];
	hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
	if( FAILED( hr ) )
		return;

	// Save the Vertex Buffer for easy access
	characters[characterIndex].vertexBuffer = g_pVertexBuffer;

	//// Load the Indice Buffer
	//bd.Usage = D3D10_USAGE_DEFAULT;
	//bd.ByteWidth = sizeof( int ) * indices_vec.size();
	//bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	//bd.CPUAccessFlags = 0;
	//bd.MiscFlags = 0;
	//InitData.pSysMem = &indices_vec[0];
	//hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
	//if( FAILED( hr ) )
	//	return;

	// Save the Index Buffer for easy access
//	characters[characterIndex].indexBuffer = g_pIndexBuffer;

	for( int i = 0; i < textures->size(); i++ )
	{
		hr = D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, textures[0][i], NULL, NULL, &g_pTextureRV, NULL );
		characters[characterIndex].texArray.push_back( g_pTextureRV );
		hr = D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, NormTextures[0][i], NULL, NULL, &g_pTextureRV, NULL );
		characters[characterIndex].NormArray.push_back( g_pTextureRV );
	}
	
	characters[characterIndex].numMeshes = 1;//tempTexCount;
	characters[characterIndex].alpha = 0;
	textures->resize(0);
	NormTextures->resize(0);
	return;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	g_hWnd = CreateWindow( L"TutorialWindowClass", L"FBX Loader Demo", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL );
	if( !g_hWnd )
		return E_FAIL;

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect( g_hWnd, &rc );
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	gHeight = height;
	gWidth = width;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	D3D10_DRIVER_TYPE driverTypes[] =
	{
		D3D10_DRIVER_TYPE_HARDWARE,
		D3D10_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D10CreateDeviceAndSwapChain( NULL, g_driverType, NULL, 0,
			D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice );
		if( SUCCEEDED( hr ) )
			break;
	}
	if( FAILED( hr ) )
		return hr;

	// Create a render target view
	ID3D10Texture2D* pBuffer;
	hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer );
	if( FAILED( hr ) )
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView( pBuffer, NULL, &g_pRenderTargetView );
	pBuffer->Release();
	if( FAILED( hr ) )
		return hr;

	// Create depth stencil texture
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
	if( FAILED( hr ) )
		return hr;

	// Create the depth stencil view
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
	if( FAILED( hr ) )
		return hr;

	g_pd3dDevice->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );


	// Setup the viewport
	D3D10_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Create the effect
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif
	hr = D3DX10CreateEffectFromFile( L"Titanic.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL,
		NULL, &g_pEffect, NULL, NULL );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			L"The FX file cannot be located.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
		return hr;
	}

	// Obtain the technique
	g_pTechnique = g_pEffect->GetTechniqueByName( "Render" );

	// Shadow Map technique
	g_pShadowMapTechnique = g_pEffect->GetTechniqueByName( "RenderShadowMap" );

	// Obtain the variables
	g_pWorldVariable			= g_pEffect->GetVariableByName( "World" )->AsMatrix();
	g_pViewVariable				= g_pEffect->GetVariableByName( "View" )->AsMatrix();
	g_pProjectionVariable		= g_pEffect->GetVariableByName( "Projection" )->AsMatrix();

	g_pLightDirVariable			= g_pEffect->GetVariableByName( "vLightDir" )->AsVector();
	g_pLightColorVariable		= g_pEffect->GetVariableByName( "vLightColor" )->AsVector();
	g_pOutputColorVariable		= g_pEffect->GetVariableByName( "vOutputColor" )->AsVector();

	g_ptextureSelectVariable	= g_pEffect->GetVariableByName( "texSelect")->AsScalar();
	g_ptextureAlphaVariable		= g_pEffect->GetVariableByName( "isAlpha" )->AsScalar();
    
	g_pDiffuseVariable			= g_pEffect->GetVariableByName( "txDiffuse0" )->AsShaderResource();
    g_pDiffuseVariable2			= g_pEffect->GetVariableByName( "txDiffuse1" )->AsShaderResource();
	g_ptextureArrayPtr			= g_pEffect->GetVariableByName( "DiffuseTextures")->AsShaderResource();
	g_pNormalArrayPtr			= g_pEffect->GetVariableByName( "NormalTextures")->AsShaderResource();
	

	// Define the input layout
	D3D10_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 }, 
		{ "TEXNUM",   0, DXGI_FORMAT_R32_FLOAT,		  0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof( layout ) / sizeof( layout[0] );

	// Create the input layout
	D3D10_PASS_DESC PassDesc;
	g_pTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
		PassDesc.IAInputSignatureSize, &g_pVertexLayout );
	if( FAILED( hr ) )
		return hr;

	// Set the input layout
	g_pd3dDevice->IASetInputLayout( g_pVertexLayout );



	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Here is where I load my shit. textures get reset to size = 0 in charLoad function
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	vector<const wchar_t *> textures;
	vector<const wchar_t *> normalMap;

	textures.push_back( L"../assets/Textures/CommandoArmor_DM.dds" );
	textures.push_back( L"../assets/Textures/Commando_DM.dds" );
	normalMap.push_back( L"../assets/Textures/CommandoArmor_NM.dds" );
	normalMap.push_back( L"../assets/Textures/Commando_NM.dds" );
	charLoad( "../assets/Models/bigbadman.fbx", &textures, &normalMap );

	textures.push_back( L"../assets/Textures/PRP_Log_A_DM.dds" );
	textures.push_back( L"../assets/Textures/PRP_TreeLeaves_A.dds" );
	normalMap.push_back( L"../assets/Textures/PRP_Log_A_NM.dds" );
	normalMap.push_back( L"../assets/Textures/PRP_Log_A_NM.dds" );
	charLoad( "../assets/Models/PRP_TREE_A.fbx", &textures, &normalMap );

	createPlane();
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	// Set vertex buffer
	g_pd3dDevice->IASetVertexBuffers( 0, 1, &characters[characterIndex].vertexBuffer, &stride, &offset );

	// Set index buffer
	//g_pd3dDevice->IASetIndexBuffer( characters[characterIndex].indexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	// Set primitive topology
	g_pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Set textures
	g_ptextureArrayPtr->SetResourceArray(&characters[characterIndex].texArray[0], 0, characters[characterIndex].texArray.size());

	vLightDirs.push_back( D3DXVECTOR4( 0.0f, 1.0f, 0.0f, 1.0f ) );
	vLightColor.push_back( D3DXVECTOR4( 1.0, 1.0, 1.0, 1.0 ) );

	vLightDirs.push_back( D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) );
	vLightColor.push_back( D3DXVECTOR4( 1.0, 1.0, 1.0, 1.0 ) );

	vLightDirs.push_back( D3DXVECTOR4( 0.0f, 0.0f, -1.0f, 1.0f ) );
	vLightColor.push_back( D3DXVECTOR4( 1.0, 1.0, 1.0, 1.0 ) );

	/*-----------------------------------------------------------------------------------------------------------*/
	
	if( FAILED( hr ) )
        return hr;

	// Initialize the world matrices
	D3DXMatrixIdentity( &g_World1 );


	// Initialize Camera Variables
	Cam.eye.x = 0.0f;
	Cam.eye.y = 1.0f;
	Cam.eye.z = -10.0f;

	Cam.at.x = 0.0f;
	Cam.at.y = 1.0f;
	Cam.at.z = 0.0f;

	Cam.up.x = 0.0f;
	Cam.up.y = 1.0f;
	Cam.up.z = 0.0f;

	camfunc();

	ZeroMemory( &raster, sizeof(D3D10_RASTERIZER_DESC));

	raster.FillMode = D3D10_FILL_SOLID;
	raster.CullMode = D3D10_CULL_NONE;
	raster.FrontCounterClockwise = FALSE;
	raster.DepthBias = 0;
	raster.DepthBiasClamp = 0.0f;
	raster.SlopeScaledDepthBias = 0.0f;
	raster.DepthClipEnable = TRUE; //set for testing otherwise true
	raster.ScissorEnable = FALSE;
	raster.MultisampleEnable = FALSE;
	raster.AntialiasedLineEnable = FALSE;

	hr = g_pd3dDevice->CreateRasterizerState (&raster, &pState);
	g_pd3dDevice->RSSetState( pState );

	return TRUE;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if( g_pd3dDevice ) g_pd3dDevice->ClearState();

	if( g_pVertexBuffer ) g_pVertexBuffer->Release();
	if( g_pIndexBuffer ) g_pIndexBuffer->Release();
	if( g_pVertexLayout ) g_pVertexLayout->Release();
	if( g_pEffect ) g_pEffect->Release();
	if( g_pRenderTargetView ) g_pRenderTargetView->Release();
	if( g_pDepthStencil ) g_pDepthStencil->Release();
	if( g_pDepthStencilView ) g_pDepthStencilView->Release();
	if( g_pSwapChain ) g_pSwapChain->Release();
	if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message )
	{
	case WM_KEYDOWN:
		{
			switch(wParam)
			{
			case 'W':
				Cam.eye.y = Cam.eye.y + moveUnit;
				Cam.at.y = Cam.at.y + moveUnit;
				camfunc();
				break;
			case 'S':
				Cam.eye.y = Cam.eye.y - moveUnit;
				Cam.at.y = Cam.at.y - moveUnit;
				camfunc();
				break;
			case 'A':
				Cam.eye.z = Cam.eye.z + moveUnit;
				Cam.at.z = Cam.at.z + moveUnit;
				camfunc();
				break;
			case 'D':
				Cam.eye.z = Cam.eye.z - moveUnit;
				Cam.at.z = Cam.at.z - moveUnit;
				camfunc();
				break;
			case 'L':
				raster.FillMode = D3D10_FILL_WIREFRAME;
				g_pd3dDevice->CreateRasterizerState (&raster, &pState);
				g_pd3dDevice->RSSetState( pState );
				break;	
			case 'K':
				raster.FillMode = D3D10_FILL_SOLID;
				g_pd3dDevice->CreateRasterizerState (&raster, &pState);
				g_pd3dDevice->RSSetState( pState );
				break;
			case VK_RIGHT:
				// Right arrow pressed
				// Set vertex buffer
				
				characterIndex++;
				if( characterIndex == characters.size() )
					characterIndex = 0;

				g_pd3dDevice->IASetVertexBuffers( 0, 1, &characters[characterIndex].vertexBuffer, &stride, &offset );
				// Set index buffer
//				g_pd3dDevice->IASetIndexBuffer( characters[characterIndex].indexBuffer, DXGI_FORMAT_R32_UINT, 0 );
				g_ptextureArrayPtr->SetResourceArray(&characters[characterIndex].texArray[0], 0, characters[characterIndex].texArray.size());
				g_pNormalArrayPtr->SetResourceArray(&characters[characterIndex].texArray[0], 0, characters[characterIndex].texArray.size());

				break;
			case VK_LEFT:
				// Right arrow pressed
				// Set vertex buffer
				characterIndex--;
				if( characterIndex < 0 )
					characterIndex = characters.size() - 1;
				g_pd3dDevice->IASetVertexBuffers( 0, 1, &characters[characterIndex].vertexBuffer, &stride, &offset );
				// Set index buffer
//				g_pd3dDevice->IASetIndexBuffer( characters[characterIndex].indexBuffer, DXGI_FORMAT_R32_UINT, 0 );
				g_ptextureArrayPtr->SetResourceArray(&characters[characterIndex].texArray[0], 0, characters[characterIndex].texArray.size());
				g_pNormalArrayPtr->SetResourceArray(&characters[characterIndex].texArray[0], 0, characters[characterIndex].texArray.size());
				
				break;
			case VK_UP:
				texdex++;
				if( texdex > characters[characterIndex].numMeshes)//characters[characterIndex].vertices[characters[characterIndex].vertices.size()-1].texNum )
					texdex = -2;
				break;
			case VK_DOWN:
				texdex--;
				if( texdex < - 2)
					texdex = characters[characterIndex].numMeshes;//characters[characterIndex].vertices[characters[characterIndex].vertices.size()-1].texNum;
				break;
			default:
				break;
			}
		}
	case WM_MOUSEMOVE:
		{

			// Retrieve mouse screen position
			int x=(short)LOWORD(lParam);
			int y=(short)HIWORD(lParam);

			// Check to see if the left button is held down:
			bool leftButtonDown=wParam & MK_LBUTTON;

			// Check if right button down:
			bool rightButtonDown=wParam & MK_RBUTTON;

			break;
		}


	case WM_PAINT:
		hdc = BeginPaint( hWnd, &ps );
		EndPaint( hWnd, &ps );
		break;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void DrawScene()
{
	// Update our time
	static float t = 0.0f;
	static DWORD dwTimeStart = 0;
	DWORD dwTimeCur = GetTickCount();
	if( dwTimeStart == 0 )
		dwTimeStart = dwTimeCur;
	t = ( dwTimeCur - dwTimeStart ) / 1000.0f;

	// 1st Cube: Rotate around the origin
	D3DXMatrixRotationY( &g_World1, t/2.0 );

	// Clear the back buffer
	g_pd3dDevice->ClearRenderTargetView( g_pRenderTargetView, COLOR );

	// Clear the depth buffer to 1.0 (max depth)
	g_pd3dDevice->ClearDepthStencilView( g_pDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	//
	// Update variables for the first cube
	//
	g_pWorldVariable->SetMatrix( ( float* )&g_World1 );
	g_pViewVariable->SetMatrix( ( float* )&g_View );
	g_pProjectionVariable->SetMatrix( ( float* )&g_Projection );

	//
	// Update lighting variables
	//
	g_pLightDirVariable->SetFloatVectorArray( (float*)&vLightDirs[0], 0, vLightDirs.size() );
	g_pLightColorVariable->SetFloatVectorArray( (float*)&vLightColor[0], 0, vLightColor.size() );


	// some more variables
	g_ptextureSelectVariable->AsScalar()->SetInt( texdex );
	g_ptextureAlphaVariable->SetInt( characters[characterIndex].alpha );

	g_pNormalArrayPtr->SetResourceArray(&characters[characterIndex].NormArray[0], 0, characters[characterIndex].texArray.size());

	g_pd3dDevice->IASetVertexBuffers( 0, 1, &characters[characterIndex].vertexBuffer, &stride, &offset );
	D3D10_TECHNIQUE_DESC techDesc;
	g_pTechnique->GetDesc( &techDesc );
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		g_pTechnique->GetPassByIndex( p )->Apply( 0 );
		g_pd3dDevice->Draw(	characters[characterIndex].vertices.size(), 0 );
	}
	
	g_pd3dDevice->IASetVertexBuffers( 0, 1, &terrain.vertexBuffer, &stride, &offset );
	//Set index buffer
	g_pTechnique->GetDesc( &techDesc );
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		g_pTechnique->GetPassByIndex( p )->Apply( 0 );
		g_pd3dDevice->Draw(	terrain.vertices.size(), 0 );
	}

	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present( 0, 0 );
}


void Render()
{
	DrawScene();
}
