//========= Copyright Valve Corporation ============//

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

//reading obj files
#include <iostream>
#include <fstream>

//string operations

#include <GL/glu.h>

#include <stdio.h>
#include <string>
#include <cstdlib>

#include <openvr.h>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#include "LoadObj.h"


#ifndef _WIN32
#define APIENTRY
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

void ThreadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
	::Sleep(nMilliseconds);
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}

//class CGLRenderModel
//{
//public:
//	~CGLRenderModel();
//
//	void Cleanup();
//	const std::string & GetName() const { return m_sModelName; }
//
//private:
//	GLuint m_glVertBuffer;
//	GLuint m_glIndexBuffer;
//	GLuint m_glVertArray;
//	GLuint m_glTexture;
//	GLsizei m_unVertexCount;
//	std::string m_sModelName;
//};

static bool g_bPrintf = true;

//-----------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
class CMainApplication
{
public:
	CMainApplication(int argc, char *argv[]);
	virtual ~CMainApplication();

	bool BInit();
	bool BInitGL();

	void Shutdown();

	void RunMainLoop();
	bool HandleInput();
	void RenderFrame();

	bool SetupTexturemaps(const char* path);

	void SetupScene(const char * path);
	void AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata);
	void AddVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata);

	void AddTriangleToScene(Matrix4 mat, std::vector<float>& vertdata);
	void AddObjToScene(Matrix4 mat, const char* path, std::vector<float> &vertdata);
	void AddMeshToScene(Matrix4 mat, std::vector<float> &vertdata, std::vector<Vector4> inputVertices, std::vector<Vector4> inputFaces);

	bool SetupStereoRenderTargets();
	void SetupCompanionWindow();
	void SetupCameras();

	void RenderStereoTargets();
	void RenderCompanionWindow();
	void RenderScene(vr::Hmd_Eye nEye);

	Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	void UpdateHMDMatrixPose();

	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);

	GLuint CompileGLShader(const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader);
	bool CreateAllShaders();

private:
	vr::IVRSystem *m_pHMD;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];

private: // SDL bookkeeping
	SDL_Window *m_pCompanionWindow;
	uint32_t m_nCompanionWindowWidth;
	uint32_t m_nCompanionWindowHeight;

	SDL_GLContext m_pContext;

private: // OpenGL bookkeeping
	bool m_bShowCubes;

	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;
	float m_fScaleSpacing;
	float m_fScale;

	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20

	float m_fNearClip;
	float m_fFarClip;

	GLuint m_iTexture;

	unsigned int m_uiVertcount;

	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;
	unsigned int m_uiCompanionWindowIndexSize;

	unsigned int m_uiControllerVertcount;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

	struct VertexDataScene
	{
		Vector3 position;
		Vector2 texCoord;
	};

	struct VertexDataWindow
	{
		Vector2 position;
		Vector2 texCoord;

		VertexDataWindow(const Vector2 & pos, const Vector2 tex) : position(pos), texCoord(tex) {	}
	};

	GLuint m_unSceneProgramID;
	GLuint m_unCompanionWindowProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;

	GLint m_nSceneMatrixLocation;
	GLint m_nControllerMatrixLocation;
	GLint m_nRenderModelMatrixLocation;

	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc);

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	//std::vector< CGLRenderModel * > m_vecRenderModels;
	//CGLRenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
};

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
void dprintf(const char *fmt, ...)
{
	va_list args;
	char buffer[2048];

	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);

	if (g_bPrintf)
		printf("%s", buffer);

	OutputDebugStringA(buffer);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainApplication::CMainApplication(int argc, char *argv[])
	: m_pCompanionWindow(NULL)
	, m_pContext(NULL)
	, m_nCompanionWindowWidth(640)
	, m_nCompanionWindowHeight(320)
	, m_unSceneProgramID(0)
	, m_unCompanionWindowProgramID(0)
	, m_unControllerTransformProgramID(0)
	, m_unRenderModelProgramID(0)
	, m_pHMD(NULL)
	, m_unSceneVAO(0)
	, m_nSceneMatrixLocation(-1)
	, m_nControllerMatrixLocation(-1)
	, m_nRenderModelMatrixLocation(-1)
	, m_iSceneVolumeInit(5)
	, m_bShowCubes(true) {};


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainApplication::~CMainApplication()
{
	// work is done in Shutdown
	dprintf("Shutdown");
}


//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::BInit()
{
	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		return false;
	}

	int nWindowPosX = 700;
	int nWindowPosY = 100;
	Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	m_pCompanionWindow = SDL_CreateWindow("hellovr", nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags);

	m_pContext = SDL_GL_CreateContext(m_pCompanionWindow);

	glewExperimental = GL_TRUE;
	GLenum nGlewError = glewInit(); // FUNKTIONSAUFRUF --------------------------------------------------------------
	if (nGlewError != GLEW_OK) {
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW

	std::string strWindowTitle = "Unser IT-Projekt";
	SDL_SetWindowTitle(m_pCompanionWindow, strWindowTitle.c_str());

	// cube array
	m_iSceneVolumeWidth = m_iSceneVolumeInit;
	m_iSceneVolumeHeight = m_iSceneVolumeInit;
	m_iSceneVolumeDepth = m_iSceneVolumeInit;

	m_fScale = 0.3f;
	m_fScaleSpacing = 4.0f;

	m_fNearClip = 0.1f;
	m_fFarClip = 30.0f;

	m_iTexture = 0;
	m_uiVertcount = 0;

	if (!BInitGL())
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.
//-----------------------------------------------------------------------------
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	dprintf("GL Error: %s\n", message);
}


//-----------------------------------------------------------------------------
// Purpose: Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 
//-----------------------------------------------------------------------------
bool CMainApplication::BInitGL()
{
	if (!CreateAllShaders())
		return false;

	std::vector<const char *> textureVector = std::vector<const char *>();
	std::vector<const char *> objectVector = std::vector<const char *>();

	textureVector.push_back("../3d-models/alduin_.png");
	objectVector.push_back("../3d-models/alduin.obj");

	for (unsigned i = 0; i < textureVector.size(); i++){
		SetupTexturemaps(textureVector[i]); // loads texture
		SetupScene(objectVector[i]); // loads cubes and puts texture on them
	}


	SetupCameras(); // gets projection matrix and pose of left and right camera
	SetupStereoRenderTargets(); // creates frame buffer for render objects
	SetupCompanionWindow(); // sets up windows for right and left eye

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::Shutdown()
{
	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

	if (m_pCompanionWindow)
	{
		SDL_DestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}

	SDL_Quit();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::HandleInput()
{
	SDL_Event sdlEvent;
	bool bRet = false;

	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		if (sdlEvent.type == SDL_QUIT)
		{
			bRet = true;
		}
		else if (sdlEvent.type == SDL_KEYDOWN)
		{
			if (sdlEvent.key.keysym.sym == SDLK_ESCAPE
				|| sdlEvent.key.keysym.sym == SDLK_q)
			{
				bRet = true;
			}
			if (sdlEvent.key.keysym.sym == SDLK_c)
			{
				m_bShowCubes = !m_bShowCubes;
			}
		}
	}

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RunMainLoop()
{
	bool bQuit = false;

	while (!bQuit)
	{
		bQuit = HandleInput(); // handles input from mouse/computer

		RenderFrame();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame()
{
	// for now as fast as possible
	if (m_pHMD)
	{
		RenderStereoTargets(); // Renders everything
		RenderCompanionWindow(); // puts rendering on left and right eye

		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	// SwapWindow
	{
		SDL_GL_SwapWindow(m_pCompanionWindow); // displays scene on pc
	}

	UpdateHMDMatrixPose();
}


//-----------------------------------------------------------------------------
// Purpose: Compiles a GL shader program and returns the handle. Returns 0 if
//			the shader couldn't be compiled for some reason.
//-----------------------------------------------------------------------------
GLuint CMainApplication::CompileGLShader(const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader)
{
	GLuint unProgramID = glCreateProgram();

	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(nSceneVertexShader, 1, &pchVertexShader, NULL);
	glCompileShader(nSceneVertexShader);

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);


	glAttachShader(unProgramID, nSceneVertexShader);
	glDeleteShader(nSceneVertexShader); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(nSceneFragmentShader, 1, &pchFragmentShader, NULL);
	glCompileShader(nSceneFragmentShader);

	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);


	glAttachShader(unProgramID, nSceneFragmentShader);
	glDeleteShader(nSceneFragmentShader); // the program hangs onto this once it's attached

	glLinkProgram(unProgramID);

	GLint programSuccess = GL_TRUE;
	glGetProgramiv(unProgramID, GL_LINK_STATUS, &programSuccess);


	glUseProgram(unProgramID);
	glUseProgram(0);

	return unProgramID;
}


//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//-----------------------------------------------------------------------------
bool CMainApplication::CreateAllShaders()
{
	m_unSceneProgramID = CompileGLShader(
		"Scene",

		// Vertex Shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVcoordsIn;\n"
		"layout(location = 2) in vec3 v3NormalIn;\n"
		"out vec2 v2UVcoords;\n"
        "out Data {\n"
        "    vec3 normal;\n"
        "    vec4 eye;\n"
        "} DataOut;\n"
		"void main()\n"
		"{\n"
        "   DataOut.normal = normalize(m_normal * normal);\n"
        "   DataOut.eye = -(m_viewModel * position);\n"
		"	v2UVcoords = v2UVcoordsIn;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// Fragment Shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"in vec2 v2UVcoords;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture(mytexture, v2UVcoords);\n"
		"}\n"
	);
	m_nSceneMatrixLocation = glGetUniformLocation(m_unSceneProgramID, "matrix");

	m_unControllerTransformProgramID = CompileGLShader(
		"Controller",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3ColorIn;\n"
		"out vec4 v4Color;\n"
		"void main()\n"
		"{\n"
		"	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// fragment shader
		"#version 410\n"
		"in vec4 v4Color;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = v4Color;\n"
		"}\n"
	);
	m_nControllerMatrixLocation = glGetUniformLocation(m_unControllerTransformProgramID, "matrix");

	m_unRenderModelProgramID = CompileGLShader(
		"render model",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3NormalIn;\n"
		"layout(location = 2) in vec2 v2TexCoordsIn;\n"
		"out vec2 v2TexCoord;\n"
		"void main()\n"
		"{\n"
		"	v2TexCoord = v2TexCoordsIn;\n"
		"	gl_Position = matrix * vec4(position.xyz, 1);\n"
		"}\n",

		//fragment shader
		"#version 410 core\n"
		"uniform sampler2D diffuse;\n"
		"in vec2 v2TexCoord;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture( diffuse, v2TexCoord);\n"
		"}\n"

	);
	m_nRenderModelMatrixLocation = glGetUniformLocation(m_unRenderModelProgramID, "matrix");

	m_unCompanionWindowProgramID = CompileGLShader(
		"CompanionWindow",

		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVIn;\n"
		"noperspective out vec2 v2UV;\n"
		"void main()\n"
		"{\n"
		"	v2UV = v2UVIn;\n"
		"	gl_Position = position;\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"noperspective in vec2 v2UV;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"		outputColor = texture(mytexture, v2UV);\n"
		"}\n"
	);

	return m_unSceneProgramID != 0
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unCompanionWindowProgramID != 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupTexturemaps(const char* path)
{
	std::string sExecutableDirectory = Path_StripFilename(Path_GetExecutablePath());
	//std::string strFullPath = Path_MakeAbsolute("../cube_texture.png", sExecutableDirectory);
	//std::string strFullPath = Path_MakeAbsolute("../3d-models/Mewtwo_with_stones_.png", sExecutableDirectory);
	std::string strFullPath = Path_MakeAbsolute(path, sExecutableDirectory);

	std::vector<unsigned char> imageRGBA;
	unsigned nImageWidth, nImageHeight;
	unsigned nError = lodepng::decode(imageRGBA, nImageWidth, nImageHeight, strFullPath.c_str());

	if (nError != 0)
		return false;

	glGenTextures(1, &m_iTexture);
	glBindTexture(GL_TEXTURE_2D, m_iTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nImageWidth, nImageHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, &imageRGBA[0]);

	glGenerateMipmap(GL_TEXTURE_2D);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (m_iTexture != 0);
}


//-----------------------------------------------------------------------------
// Purpose: Create Items
//-----------------------------------------------------------------------------
void CMainApplication::SetupScene(const char * path)
{
	if (!m_pHMD)
		return;

	std::vector<float> vertdataarray;

	Matrix4 matScale;
	double a = m_fScale;

	matScale.scale(m_fScale, m_fScale, m_fScale);

	Matrix4 matTransform;
	matTransform.translate(5,1,0);

	matTransform.translate(
		-((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f);

	Matrix4 mat = matScale * matTransform;

	//for (int z = 0; z< m_iSceneVolumeDepth; z++)
	//{
	//	for (int y = 0; y< m_iSceneVolumeHeight; y++)
	//	{
	//		for (int x = 0; x< m_iSceneVolumeWidth; x++)
	//		{
	//			AddCubeToScene(mat, vertdataarray);
	//			mat = mat * Matrix4().translate(m_fScaleSpacing, 0, 0);

	//			//AddMeshToScene(mat, vertdataarray);

	//			//AddTriangleToScene(mat, vertdataarray);
	//			//mat = mat * Matrix4().translate(m_fScaleSpacing, 0, 0);

	//		}
	//	//	mat = mat * Matrix4().translate(-((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0);
	//	//}
	//	//mat = mat * Matrix4().translate(0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing);

	//	//mat = mat * Matrix4().translate(-5,5,0);
	//}
	//mat = mat * Matrix4().translate(0 ,5,-5);
	//}

	//load obj
	//const char* objPath = "..\\bin\\3d-models\\Mewtwo_with_stones.obj";
	//const char* objPath = "..\\bin\\3d-models\\alduin.obj";
	//const char* objPath = "..\\bin\\3d-models\\simple_object.obj";
	AddObjToScene(mat, path, vertdataarray);

	m_uiVertcount = vertdataarray.size() / 5;

	glGenVertexArrays(1, &m_unSceneVAO);
	glBindVertexArray(m_unSceneVAO);

	glGenBuffers(1, &m_glSceneVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW);

	GLsizei stride = sizeof(VertexDataScene);
	uintptr_t offset = 0;

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

	offset += sizeof(Vector3);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void *)offset);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMainApplication::AddVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata)
{
	vertdata.push_back(fl0);
	vertdata.push_back(fl1);
	vertdata.push_back(fl2);
	vertdata.push_back(fl3);
	vertdata.push_back(fl4);
}

void CMainApplication::AddObjToScene(Matrix4 mat, const char* path, std::vector<float> &vertdata) {
	std::vector<Vector4> vertices;
	std::vector<Vector2> textCoords;
	std::vector<Vector3> triangles;
	std::vector<Vector3> trianglesTxtrIndices;

	LoadObj(path, &vertices, &textCoords, &triangles, &trianglesTxtrIndices);

	// Still unclear, 4th and 5th parameter that gets pushed into vertdata (0's and 1's)
	for (unsigned i = 0; i < triangles.size(); i++) {
		unsigned int vi1 = (unsigned int)(triangles[i].x),
			vi2 = (unsigned int)(triangles[i].y),
			vi3 = (unsigned int)(triangles[i].z);
		unsigned int vti1 = (unsigned int)(trianglesTxtrIndices[i].x),
			vti2 = (unsigned int)(trianglesTxtrIndices[i].y),
			vti3 = (unsigned int)(trianglesTxtrIndices[i].z);

		Vector4 v1 = mat * vertices[vi1],
			v2 = mat * vertices[vi2],
			v3 = mat * vertices[vi3];
		Vector2 vt1 = textCoords[vti1],
			vt2 = textCoords[vti2],
			vt3 = textCoords[vti3];

		AddVertex(v1.x/120, v1.y/120, v1.z/120, vt1.x, vt1.y, vertdata);
		AddVertex(v2.x/120, v2.y/120, v2.z/120, vt2.x, vt2.y, vertdata);
		AddVertex(v3.x/120, v3.y/120, v3.z/120, vt3.x, vt3.y, vertdata);
	}
}

//CGLRenderModel CMainApplication::AddMeshToScene(Matrix4 mat, std::vector<float> &vertdata, std::vector<Vector4> inputVertices, std::vector<Vector4> inputFaces) {
//	//foreach f in inputFaces:
//	//	var v1, v2, v3 = f[0, 1, 2]
//
//}

void CMainApplication::AddTriangleToScene(Matrix4 mat, std::vector<float> &vertdata) {
	Vector4 A = mat * Vector4(0, 0, 0, 0);
	Vector4 B = mat * Vector4(1, 0, 1, 0);
	Vector4 C = mat * Vector4(1, 0, 0, 0);
	Vector4 D = mat * Vector4(0.5, 1, 0.5, 0);

	//Boden
	AddVertex(A.x, A.y, A.z, 0, 1, vertdata);
	AddVertex(C.x, C.y, C.z, 1, 1, vertdata);
	AddVertex(B.x, B.y, B.z, 1, 0, vertdata);

	//Seite1
	AddVertex(A.x, A.y, A.z, 0, 1, vertdata);
	AddVertex(D.x, D.y, D.z, 1, 1, vertdata);
	AddVertex(B.x, B.y, B.z, 1, 0, vertdata);

	//Seite2
	AddVertex(A.x, A.y, A.z, 1, 0, vertdata);
	AddVertex(D.x, D.y, D.z, 1, 1, vertdata);
	AddVertex(C.x, C.y, C.z, 1,0, vertdata);

	//Seite3
	AddVertex(C.x, C.y, C.z, 0, 1, vertdata);
	AddVertex(D.x, D.y, D.z, 0, 1, vertdata);
	AddVertex(B.x, B.y, B.z, 0, 1, vertdata);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata)
{
	// Matrix4 mat( outermat.data() );

	Vector4 A = mat * Vector4(-1, 0, 0, 1);
	Vector4 B = mat * Vector4(1, 0, 0, 1);
	Vector4 C = mat * Vector4(1, 1, 0, 1);
	Vector4 D = mat * Vector4(-1, 1, 0, 1);
	Vector4 E = mat * Vector4(-1, 0, 1, 1);
	Vector4 F = mat * Vector4(1, 0, 1, 1);
	Vector4 G = mat * Vector4(1, 1, 1, 1);
	Vector4 H = mat * Vector4(-1, 1, 1, 1);


	// triangles instead of quads
	AddVertex(E.x, E.y, E.z, 0, 1, vertdata); //Front
	AddVertex(F.x, F.y, F.z, 1, 1, vertdata);
	AddVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddVertex(H.x, H.y, H.z, 0, 0, vertdata);
	AddVertex(E.x, E.y, E.z, 0, 1, vertdata);

	AddVertex(B.x, B.y, B.z, 0, 1, vertdata); //Back
	AddVertex(A.x, A.y, A.z, 1, 1, vertdata);
	AddVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddVertex(C.x, C.y, C.z, 0, 0, vertdata);
	AddVertex(B.x, B.y, B.z, 0, 1, vertdata);

	AddVertex(H.x, H.y, H.z, 0, 1, vertdata); //Top
	AddVertex(G.x, G.y, G.z, 1, 1, vertdata);
	AddVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddVertex(H.x, H.y, H.z, 0, 1, vertdata);

	AddVertex(A.x, A.y, A.z, 0, 1, vertdata); //Bottom
	AddVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddVertex(E.x, E.y, E.z, 0, 0, vertdata);
	AddVertex(A.x, A.y, A.z, 0, 1, vertdata);

	AddVertex(A.x, A.y, A.z, 0, 1, vertdata); //Left
	AddVertex(E.x, E.y, E.z, 1, 1, vertdata);
	AddVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddVertex(A.x, A.y, A.z, 0, 1, vertdata);

	AddVertex(F.x, F.y, F.z, 0, 1, vertdata); //Right
	AddVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddVertex(G.x, G.y, G.z, 0, 0, vertdata);
	AddVertex(F.x, F.y, F.z, 0, 1, vertdata);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CMainApplication::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc)
{
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupStereoRenderTargets()
{
	if (!m_pHMD)
		return false;

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCompanionWindow()
{
	if (!m_pHMD)
		return;

	std::vector<VertexDataWindow> vVerts;

	// left eye verts
	vVerts.push_back(VertexDataWindow(Vector2(-1, -1), Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(1, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(-1, 1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(1, 0)));

	// right eye verts
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(1, -1), Vector2(1, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(1, 1), Vector2(1, 0)));

	GLushort vIndices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6 };
	m_uiCompanionWindowIndexSize = _countof(vIndices);

	glGenVertexArrays(1, &m_unCompanionWindowVAO);
	glBindVertexArray(m_unCompanionWindowVAO);

	glGenBuffers(1, &m_glCompanionWindowIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glCompanionWindowIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, texCoord));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderStereoTargets()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// --------------

	glEnable(GL_MULTISAMPLE);

	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Right);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}


//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void CMainApplication::RenderScene(vr::Hmd_Eye nEye)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (m_bShowCubes)
	{
		glUseProgram(m_unSceneProgramID);
		glUniformMatrix4fv(m_nSceneMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glBindVertexArray(m_unSceneVAO);
		glBindTexture(GL_TEXTURE_2D, m_iTexture);
		glDrawArrays(GL_TRIANGLES, 0, m_uiVertcount);
		glBindVertexArray(0);
	}

	bool bIsInputCapturedByAnotherProcess = m_pHMD->IsInputFocusCapturedByAnotherProcess();

	if (!bIsInputCapturedByAnotherProcess)
	{
		// draw the controller axis lines
		glUseProgram(m_unControllerTransformProgramID);
		glUniformMatrix4fv(m_nControllerMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glDrawArrays(GL_LINES, 0, m_uiControllerVertcount);
		glBindVertexArray(0);
	}

	// ----- Render Model rendering -----
	glUseProgram(m_unRenderModelProgramID);

	glUseProgram(0);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderCompanionWindow()
{
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);

	glBindVertexArray(m_unCompanionWindowVAO);
	glUseProgram(m_unCompanionWindowProgramID);

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(uintptr_t)(m_uiCompanionWindowIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}


//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return matrixObj.invert();
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matMVP;
	if (nEye == vr::Eye_Left)
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight *  m_mat4HMDPose;
	}

	return matMVP;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::UpdateHMDMatrixPose()
{
	if (!m_pHMD)
		return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}


//CGLRenderModel::~CGLRenderModel()
//{
//	Cleanup();
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Frees the GL resources for a render model
////-----------------------------------------------------------------------------
//void CGLRenderModel::Cleanup()
//{
//	if (m_glVertBuffer)
//	{
//		glDeleteBuffers(1, &m_glIndexBuffer);
//		glDeleteVertexArrays(1, &m_glVertArray);
//		glDeleteBuffers(1, &m_glVertBuffer);
//		m_glIndexBuffer = 0;
//		m_glVertArray = 0;
//		m_glVertBuffer = 0;
//	}
//}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	CMainApplication *pMainApplication = new CMainApplication(argc, argv);

	if (!pMainApplication->BInit())
	{
		pMainApplication->Shutdown();
		return 1;
	}

	pMainApplication->RunMainLoop();

	pMainApplication->Shutdown();

	return 0;
}
