//----------------------------------------------------------------------------------
// Hacked and updated version of FbxView found on conorgraphics 
// http://www.conorgraphics.com/?p=346
// Updated by Michael Gray
// No promises this works, not responsible if your machine goes up in a roaring
// ball of fire or you turn into a gypsie.
//----------------------------------------------------------------------------------
#include "importer.h"

using namespace std;

FbxScene*	lFbxScene;
FbxManager* lSdkManager;
string		lFileName;

int size = 0;

void LoadScene(char* filename);
void ProcessScene();
void ProcessNode( FbxNode* node, int attributeType);
void ProcessMesh( FbxNode* node );
FbxVector2 GetTexCoords( FbxMesh* mesh, int layerIndex, int polygonIndex, int polygonVertexIndex, int vertexIndex );


vector<Vertex>* Vertices;
int TexCount = 0;

int meshMatMax = 0;


// We just use the main function to get the party started. From here we will call everything in a straight line hopefully
int Import( char* filename, vector<Vertex>* vert )
{
	size = 0;
	TexCount = 0;

	Vertices = vert;

	LoadScene( filename );

	cout << "Finished" << endl;
	int a;
	cin >> a;

	return Vertices->size();
}


// Get the scene initialization running
void LoadScene(char* filename)
{
	// Create the FBX SDK manager
	lSdkManager = FbxManager::Create();

	// Create an IOSettings object.
	FbxIOSettings * ios = FbxIOSettings::Create(lSdkManager, IOSROOT );
	lSdkManager->SetIOSettings(ios);

	// Create the scene
	lFbxScene = FbxScene::Create( lSdkManager, "" );

	// Create an Importer
	FbxImporter* lImporter = FbxImporter::Create( lSdkManager, "" );

	lImporter->Initialize( filename );
	lImporter->Import( lFbxScene );

	lFileName = lImporter->GetFileName().Buffer();

	lImporter->Destroy();

	ProcessScene();
}

void ProcessScene()
{
	ProcessNode( lFbxScene->GetRootNode(), FbxNodeAttribute::eMesh );
}

void ProcessNode(FbxNode* node, int attributeType)
{
	if( !node )
		return;

	FbxNodeAttribute* nodeAttribute = node->GetNodeAttribute();

	if( nodeAttribute )
	{
		switch( nodeAttribute->GetAttributeType() )
		{
		case FbxNodeAttribute::eMesh:
			ProcessMesh( node );
		}
	}

	for( int i = 0; i < node->GetChildCount(); i++ )
		ProcessNode( node->GetChild( i ), attributeType );

}

void ProcessMesh( FbxNode* node )
{
	FbxGeometryConverter GeometryConverter( node->GetFbxManager() );
	if( !GeometryConverter.TriangulateInPlace( node ) )
		return;

	FbxMesh* mesh = node->GetMesh();
	if( !mesh )
		return;

	int vertexCount = mesh->GetControlPointsCount();
	if( vertexCount <= 0 )
		return;

	cout << node->GetName() << "\t" << vertexCount << endl;

	FbxVector4* controlPoints = mesh->GetControlPoints();

	for( int i = 0; i < mesh->GetPolygonCount(); ++i )
	{
		meshMatMax = 0;
		FbxLayerElementMaterial* pLayerMaterial = mesh->GetLayer(0)->GetMaterials();
		FbxLayerElementArrayTemplate<int> *tmpArray = &pLayerMaterial->GetIndexArray();
		if( tmpArray->GetAt( i ) > meshMatMax )
			meshMatMax = tmpArray->GetAt( i );

		for( int j = 0; j < 3; ++j )
		{
			int vertexIndex = mesh->GetPolygonVertex( i, j );
			
			if( vertexIndex < 0 || vertexIndex >= vertexCount )
				continue;

			// Get vertices
			FbxVector4 position = controlPoints[vertexIndex];
			
			// Get normal
			FbxVector4 normal;
			mesh->GetPolygonVertexNormal( i, j, normal );

			// Get TexCoords
			FbxVector2 texcoord = GetTexCoords( mesh, 0, i, j, vertexIndex );

			// Get Binormals WIP USE AT OWN RISK THERE IS NO ERROR CHECKING HERE SO MUAHAHAHAHA
			FbxGeometryElementBinormal* bin = mesh->GetElementBinormal(0);
			FbxGeometryElementTangent* tan  = mesh->GetElementTangent(0);
			

			Vertex temp;
			temp.Pos = D3DXVECTOR3( position[0], position[1], position[2] );
			temp.Normal   = D3DXVECTOR3( normal[0], normal[1], normal[2] );
			temp.Tex = D3DXVECTOR2( texcoord[0], 1.0 - texcoord[1] );
			temp.texNum = tmpArray->GetAt(i) + TexCount;
			//temp.Tangent = temp.Pos;

			//temp.Tangent;
			//D3DXVec3Cross( &temp.Tangent, &temp.Normal, &temp.Pos );

			temp.Tangent = D3DXVECTOR3( tan->GetDirectArray().GetAt(i)[0],
										tan->GetDirectArray().GetAt(i)[1],
										tan->GetDirectArray().GetAt(i)[2] );
			
			temp.BiNormal = D3DXVECTOR3( bin->GetDirectArray().GetAt(i)[0],
										 bin->GetDirectArray().GetAt(i)[1],
										 bin->GetDirectArray().GetAt(i)[2] );

			

			// Add to the Vector
			Vertices->push_back( temp );
		}
		if( Vertices->size() % 3 == 0 && Vertices->size() > 0  )
		//	if( Vertices->size() > 3 )
			{
				int i0 = Vertices->size() - 3;
				int i1 = Vertices->size() - 2;
				int i2 = Vertices->size() - 1;

				D3DXVECTOR3 Q1 = Vertices[0][i1].Pos - Vertices[0][i0].Pos;
				D3DXVECTOR3 Q2 = Vertices[0][i2].Pos - Vertices[0][i0].Pos;

				D3DXVECTOR2 u0 = Vertices[0][i0].Tex;
				D3DXVECTOR2 u1 = Vertices[0][i1].Tex;
				D3DXVECTOR2 u2 = Vertices[0][i2].Tex;

				D3DXVECTOR2 s1 = u1 - u0;
				D3DXVECTOR2 s2 = u2 - u0;

				D3DXVECTOR3 T = Q1*s2.y  + Q2*-s1.y;
				D3DXVECTOR3 B = Q1*-s2.x + Q2*s1.x;

				float val = 1 / ( s1.x*s2.y - s2.x*s1.y ) / 2.;

				T = T * val;
				B = B * val;
				Vertices[0][i0].Tangent = T;
				Vertices[0][i1].Tangent = T;
				Vertices[0][i2].Tangent = T;
				Vertices[0][i0].BiNormal = B;
				Vertices[0][i1].BiNormal = B;
				Vertices[0][i2].BiNormal = B;
				
			//	temp.Tangent = T;
			//	temp.BiNormal = B;
								
			}
	}
	TexCount += meshMatMax+1;
}

FbxVector2 GetTexCoords( FbxMesh* mesh, int layerIndex, int polygonIndex, int polygonVertexIndex, int vertexIndex )
{
	int layerCount = mesh->GetLayerCount();

	if( layerIndex < layerCount )
	{
		FbxLayer* layer = mesh->GetLayer( layerIndex );

		if( layer )
		{
			FbxLayerElementUV* uv = layer->GetUVs( FbxLayerElement::eTextureDiffuse );

			if( uv )
			{
				FbxLayerElement::EMappingMode mappingMode = uv->GetMappingMode();
				FbxLayerElement::EReferenceMode referenceMode = uv->GetReferenceMode();

				const FbxLayerElementArrayTemplate<KFbxVector2>& pUVArray = uv->GetDirectArray();
				const FbxLayerElementArrayTemplate<int>& pUVIndexArray = uv->GetIndexArray();

				switch(mappingMode)
				{
				case FbxLayerElement::eByControlPoint:
					{
						int mappingIndex = vertexIndex;
						switch(referenceMode)
						{
						case KFbxLayerElement::eDirect:
							if( mappingIndex < pUVArray.GetCount() )
							{
								return pUVArray.GetAt( mappingIndex );
							}
							break;
						case KFbxLayerElement::eIndexToDirect:
							if( mappingIndex < pUVIndexArray.GetCount() )
							{
								int nIndex = pUVIndexArray.GetAt( mappingIndex );
								if( nIndex < pUVArray.GetCount() )
								{
									return pUVArray.GetAt( nIndex );
								}
							}
							break;
						};
					}
					break;

				case KFbxLayerElement::eByPolygonVertex:
					{
						int mappingIndex = mesh->GetTextureUVIndex( polygonIndex, polygonVertexIndex, KFbxLayerElement::eTextureDiffuse );
						switch(referenceMode)
						{
						case KFbxLayerElement::eDirect:
						case KFbxLayerElement::eIndexToDirect: //I have no idea why the index array is not used in this case.
							if( mappingIndex < pUVArray.GetCount() )
							{
								return pUVArray.GetAt( mappingIndex );
							}
							break;
						};
					}
					break;
				}
			}
		}
		return FbxVector2();
	}
}