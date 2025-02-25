///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;


}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
		std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
*LoadSceneTextures()
*
* This method is used for preparing the 3D scene by loading
* the shapes, textures in memory to support the 3D scene
* rendering
* **********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/glasscup.jpg",
		"glasscup");
	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood");
	
	bReturn = CreateGLTexture(
		"textures/vinous-liquid-with-foam-blobs.jpg",
		"coffee");

	bReturn = CreateGLTexture(
		"textures/lamp.jpg",
		"lamp");

	bReturn = CreateGLTexture(
		"textures/gold.jpg",
		"gold");

	bReturn = CreateGLTexture(
		"textures/keyboard.png",
		"keyboard");

	bReturn = CreateGLTexture(
		"textures/aluminum.png",
		"aluminum");

	bReturn = CreateGLTexture(
		"textures/login.jpg",
		"login");

	bReturn = CreateGLTexture(
		"textures/leather.jpg",
		"leather");

	bReturn = CreateGLTexture(
		"textures/pen.jpg",
		"pen");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.4f);
	woodMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodMaterial.shininess = 64.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.8f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 128.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	goldMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	goldMaterial.shininess = 52.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL leatherMaterial;
	leatherMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f);
	leatherMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	leatherMaterial.shininess = 0.001;
	leatherMaterial.tag = "leather";

	m_objectMaterials.push_back(leatherMaterial);

	OBJECT_MATERIAL canvasMaterial;
	canvasMaterial.diffuseColor = glm::vec3(0.7f, 0.6f, 0.5f);
	canvasMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	canvasMaterial.shininess = 0.001;
	canvasMaterial.tag = "canvas";

	m_objectMaterials.push_back(canvasMaterial);
}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene. 
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//Directional Light
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.2f));
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.4f, 0.4f, 0.4f));
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.6f, 0.6f, 0.6f));
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.3f, 0.3f, 0.3f));
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	//Secondary Light
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(2.0f, 3.0f, 2.0f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(1.0f, 0.8f, 0.7f));
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.9f, 0.8f, 0.7f));
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
	
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
		glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	//set texture
	SetShaderTexture("wood");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	
	//cylinder for coffee cup
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 1);
	//set texture
	SetShaderTexture("glasscup");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//torus for coffee cup handle
	scaleXYZ = glm::vec3(.8f, 0.8f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 1.0f, 3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 1);
	//set texture
	SetShaderTexture("glasscup");

	//set scale
	SetTextureUVScale(5.0, 1.0);

	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	// Coffee surface (thin cylinder on top of the cup)
	scaleXYZ = glm::vec3(0.95f, 0.05f, 0.95f);  // Slightly smaller than the cup and very thin

	// No rotation needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position it slightly above the cup
	positionXYZ = glm::vec3(5.0f, 2.0f, 3.0f); // Adjust Y position just inside the cup

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply coffee texture
	SetShaderTexture("coffee");

	// Adjust texture scale
	SetTextureUVScale(1.0, 1.0);

	// Draw the thin cylinder as the coffee surface
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	//Laptop Screen
	scaleXYZ = glm::vec3(4.0f, 0.0f, 2.5f); 

	// No rotation needed
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	
	positionXYZ = glm::vec3(-1.0f, 2.0f, -5.5f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("login");

	//set scale
	SetTextureUVScale(1.0, 1.0);



	// Draw plane as laptop screen
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	// Laptop Keyboard
	scaleXYZ = glm::vec3(8.1f, 0.5f, 6.0f);

	// No rotation needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	positionXYZ = glm::vec3(-1.0f, 0.0f, -2.5f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("keyboard");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	// Draw box for keyboard
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// Laptop base
	scaleXYZ = glm::vec3(8.1f, 0.49f, 6.1f);

	// No rotation needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	positionXYZ = glm::vec3(-1.0f, 0.0f, -2.5f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("gold");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("metal");

	// Draw box for keyboard
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	// Box for lamp
	scaleXYZ = glm::vec3(3.0f, 1.0f, 2.0f);

	// No rotation needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position to the left
	positionXYZ = glm::vec3(-10.0f, 0.0f, -3.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("gold");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	// Draw box as lamp stand
	m_basicMeshes->DrawBoxMesh();

	SetShaderMaterial("metal");

	/****************************************************************/
	// stand for lamp
	scaleXYZ = glm::vec3(0.5f, 7.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Position to the left
	positionXYZ = glm::vec3(-10.0f, 0.0f, -3.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetShaderTexture("gold");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("metal");

	// Draw cylinder as lamp stand
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	// shade for lamp
	scaleXYZ = glm::vec3(3.0f, 3.0f, 1.0f);

	// No rotation needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// to the left
	positionXYZ = glm::vec3(-10.0f, 6.0f, -3.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//set texture
	SetShaderTexture("lamp");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("canvas");

	// Draw cone as lampshade 
	m_basicMeshes->DrawConeMesh();

	/****************************************************************/
	// Pen
	scaleXYZ = glm::vec3(0.2f, 1.0f, 0.2f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 130.0f;
	ZrotationDegrees = 0.0f;

	// Position to the left
	positionXYZ = glm::vec3(-6.0f, 0.5f, 4.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("pen");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("metal");

	// Draw the thin cylinder for pen
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	// Pen
	scaleXYZ = glm::vec3(0.16f, 0.2f, 0.16f);

	XrotationDegrees = 270.0f;
	YrotationDegrees = 130.0f;
	ZrotationDegrees = 0.0f;

	// Position to the left
	positionXYZ = glm::vec3(-6.0f, 0.5f, 4.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("aluminum");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("metal");

	// Draw the taperd cylinder as pen tip
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	//Book
	scaleXYZ = glm::vec3(4.0f, 2.0f, 0.3f);

	XrotationDegrees = 270.0f;
	YrotationDegrees = 130.0f;
	ZrotationDegrees = 0.0f;

	// Position to the left
	positionXYZ = glm::vec3(-8.0f, 0.5f, 4.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set texture
	SetShaderTexture("leather");

	//set scale
	SetTextureUVScale(1.0, 1.0);

	SetShaderMaterial("leather");

	// Draw the box as cover
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//Book
	scaleXYZ = glm::vec3(4.0f, 1.98f, 0.2f);

	XrotationDegrees = 270.0f;
	YrotationDegrees = 130.0f;
	ZrotationDegrees = 0.0f;

	// Position to the left
	positionXYZ = glm::vec3(-7.93f, 0.5f, 4.0f);

	// Apply transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1);
	
	// Draw smaller box for pages
	m_basicMeshes->DrawBoxMesh();
}
