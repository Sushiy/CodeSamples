
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sceerror.h>
#include <math.h>

#include <gxt.h>
#include <gxm.h>
#include <kernel.h>
#include <ctrl.h>
#include <touch.h>
#include <display.h>
//Debug
#include <libdbg.h>
#include <libdbgfont.h>
//Geometry
#include <sce_geometry.h>
#include <vectormath.h>
//Profiling
#include <libsysmodule.h>
#include <razor_capture.h>
//CubeLogic
#include "logic.h"


using namespace sce::Vectormath::Simd::Aos;
using namespace sce::Geometry::Aos;


/*	Define the debug font pixel color format to render to. */
#define DBGFONT_PIXEL_FORMAT		SCE_DBGFONT_PIXELFORMAT_A8B8G8R8


/*	Define the width and height to render at the native resolution */
#define DISPLAY_WIDTH				960
#define DISPLAY_HEIGHT				544
#define DISPLAY_STRIDE_IN_PIXELS	1024

/*	Define the libgxm color format to render to.
	This should be kept in sync with the display format to use with the SceDisplay library.
*/
#define DISPLAY_COLOR_FORMAT		SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT		SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

/*	Define the number of back buffers to use with this sample.  Most applications
	should use a value of 2 (double buffering) or 3 (triple buffering).
*/
#define DISPLAY_BUFFER_COUNT		3

/*	Define the maximum number of queued swaps that the display queue will allow.
	This limits the number of frames that the CPU can get ahead of the GPU,
	and is independent of the actual number of back buffers.  The display
	queue will block during sceGxmDisplayQueueAddEntry if this number of swaps
	have already been queued.
*/
#define DISPLAY_MAX_PENDING_SWAPS	2


/*	Helper macro to align a value */
#define ALIGN(x, a)					(((x) + ((a) - 1)) & ~((a) - 1))




/*	The build process for the sample embeds the shader programs directly into the
	executable using the symbols below.  This is purely for convenience, it is
	equivalent to simply load the binary file into memory and cast the contents
	to type SceGxmProgram.
*/
extern const SceGxmProgram binaryClearVGxpStart;
extern const SceGxmProgram binaryClearFGxpStart;

/*	Data structure for clear geometry */
typedef struct ClearVertex
{
	float x;
	float y;
} ClearVertex;

// !! Data related to rendering vertex.
extern const SceGxmProgram binaryBasicVGxpStart;
extern const SceGxmProgram binaryBasicFGxpStart;

/*	Data structure for basic geometry */
typedef struct BasicVertex
{
    float position[3];
    float normal[3];
	  float tangent[3];
	  float uv[2];
    uint32_t color; // Data gets expanded to float 4 in vertex shader.
	int8_t faceTurnID;
} BasicVertex;

/*	Data structure to pass through the display queue.  This structure is
	serialized during sceGxmDisplayQueueAddEntry, and is used to pass
	arbitrary data to the display callback function, called from an internal
	thread once the back buffer is ready to be displayed.

	In this example, we only need to pass the base address of the buffer.
*/
typedef struct DisplayData
{
	void *address;
} DisplayData;

//************FACESTRUCT*****************
typedef struct FaceData
{
	float oVec[3];
	float nVec[3];
	float uVec[3];
	float vVec[3];
} FaceData;

typedef sce::Vectormath::Simd::Aos::Point3 Point3f;
typedef sce::Vectormath::Simd::Aos::Vector3 Vector3f;
typedef sce::Vectormath::Simd::Aos::Vector4 Vector4f;
typedef sce::Vectormath::Simd::Aos::Matrix4 Matrix4f;

const float PI = 3.14159265359f;

static SceGxmContextParams		s_contextParams;			/* libgxm context parameter */
static SceGxmRenderTargetParams s_renderTargetParams;		/* libgxm render target parameter */
static SceGxmContext			*s_context			= NULL;	/* libgxm context */
static SceGxmRenderTarget		*s_renderTarget		= NULL;	/* libgxm render target */
static SceGxmShaderPatcher		*s_shaderPatcher	= NULL;	/* libgxm shader patcher */

/*	display data */
static void							*s_displayBufferData[ DISPLAY_BUFFER_COUNT ];
static SceGxmSyncObject				*s_displayBufferSync[ DISPLAY_BUFFER_COUNT ];
static int32_t						s_displayBufferUId[ DISPLAY_BUFFER_COUNT ];
static SceGxmColorSurface			s_displaySurface[ DISPLAY_BUFFER_COUNT ];
static uint32_t						s_displayFrontBufferIndex = 0;
static uint32_t						s_displayBackBufferIndex = 0;
static SceGxmDepthStencilSurface	s_depthSurface;

/*	shader data */
static int32_t					s_clearVerticesUId;
static int32_t					s_clearIndicesUId;
static SceGxmShaderPatcherId	s_clearVertexProgramId;
static SceGxmShaderPatcherId	s_clearFragmentProgramId;
// !! Shader pactcher addded.
static SceGxmShaderPatcherId	s_basicVertexProgramId;
static SceGxmShaderPatcherId	s_basicFragmentProgramId;
static SceUID					s_patcherFragmentUsseUId;
static SceUID					s_patcherVertexUsseUId;
static SceUID					s_patcherBufferUId;
static SceUID					s_depthBufferUId;
static SceUID					s_vdmRingBufferUId;
static SceUID					s_vertexRingBufferUId;
static SceUID					s_fragmentRingBufferUId;
static SceUID					s_fragmentUsseRingBufferUId;

static ClearVertex				*s_clearVertices			= NULL;
static uint16_t					*s_clearIndices				= NULL;
static SceGxmVertexProgram		*s_clearVertexProgram		= NULL;
static SceGxmFragmentProgram	*s_clearFragmentProgram		= NULL;
// !! Data added.
static SceGxmVertexProgram		*s_basicVertexProgram		= NULL;
static SceGxmFragmentProgram	*s_basicFragmentProgram		= NULL;

static BasicVertex				*s_basicVertices			= NULL;
static uint16_t					*s_basicIndices				= NULL;
static int32_t					s_basicVerticesUId;
static int32_t					s_basicIndiceUId;



static uint16_t					*faceIndices				= NULL;
static int32_t					faceIndiceUId;

static BasicVertex				*faceVertices				= NULL;
static int32_t					faceVerticesUId;

static uint16_t					*greenIndices				= NULL;
static int32_t					greenIndiceUId;
static uint16_t					*redIndices					= NULL;
static int32_t					redIndiceUId;
static uint16_t					*blueIndices				= NULL;
static int32_t					blueIndiceUId;
static uint16_t					*orangeIndices				= NULL;
static int32_t					orangeIndiceUId;
static uint16_t					*whiteIndices				= NULL;
static int32_t					whiteIndiceUId;
static uint16_t					*yellowIndices				= NULL;
static int32_t					yellowIndiceUId;


//debug resultLine
static char *resultline = new char[256];

//!! The program parameter for the transformation of the cube

static FaceData faceData[6];

static Quat rotationQuat(0.0f, 1.0f, 0.0f, 0.0f);

static Matrix4f rotationM;
static Matrix4f resultM;
static Matrix4 viewProj;

static const float deltaTime = 1.0f/60.0f;
static const float rotationSpeed = 3.0f;
static float t = 0.0f; //lerpingcounter
static int turnDir;
static int turnMode = -1;


static const SceGxmProgramParameter *s_wvpParam = NULL;
static const SceGxmProgramParameter *iTWVPParam = NULL;
static const SceGxmProgramParameter *rotParam = NULL;


//Rotation Matrix for Vshader
static Matrix4f faceX;
static Matrix4f faceY;
static Matrix4f faceZ;
static const SceGxmProgramParameter *rotXParam  = NULL;
static const SceGxmProgramParameter *rotYParam  = NULL;
static const SceGxmProgramParameter *rotZParam  = NULL;

//BackTouch
static float lastA;
static float lastB;
static int lastID = -1;


//Raycast

static int touched_Face = -1; //which face is touched
static int touched_Tile = -1; //which tile of the face is touched

static float *firstHitPoint = new float[2];
static float *currentHitPoint = new float[2];

static float f_firstA, f_firstB, f_currentA, f_currentB;
static int f_firstID = -1;


//cube
static Logic cube;
static const float cubeSize = 1.0f;
static const float tileSize = cubeSize/3;

bool turning = false;

//textures
static void* textureData = NULL;
static SceUID gxt;
static SceGxmTexture textureMap;
static SceGxmTexture normalMap;

static char* texturePath = "app0:output3.gxt";



/* Callback function to allocate memory for the shader patcher */
static void *patcherHostAlloc( void *userData, uint32_t size );

/* Callback function to allocate memory for the shader patcher */
static void patcherHostFree( void *userData, void *mem );

/*	Callback function for displaying a buffer */
static void displayCallback( const void *callbackData );

/*	Helper function to allocate memory and map it for the GPU */
static void *graphicsAlloc( SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid );

/*	Helper function to free memory mapped to the GPU */
static void graphicsFree( SceUID uid );

/* Helper function to allocate memory and map it as vertex USSE code for the GPU */
static void *vertexUsseAlloc( uint32_t size, SceUID *uid, uint32_t *usseOffset );

/* Helper function to free memory mapped as vertex USSE code for the GPU */
static void vertexUsseFree( SceUID uid );

/* Helper function to allocate memory and map it as fragment USSE code for the GPU */
static void *fragmentUsseAlloc( uint32_t size, SceUID *uid, uint32_t *usseOffset );

/* Helper function to free memory mapped as fragment USSE code for the GPU */
static void fragmentUsseFree( SceUID uid );

/*	@brief Main entry point for the application
	@return Error code result of processing during execution: <c> SCE_OK </c> on success,
	or another code depending upon the error
*/
int main( void );


// !! Here we create the matrix.
void Update(void);



/*	@brief Initializes the graphics services and the libgxm graphics library
	@return Error code result of processing during execution: <c> SCE_OK </c> on success,
	or another code depending upon the error
*/
static int initGxm( void );

/*	 @brief Creates scenes with libgxm */
static void createGxmData( void );


/*	@brief Main rendering function to draw graphics to the display */
static void render( void );

/*	@brief render libgxm scenes */
static void renderGxm( void );



/*	@brief cycle display buffer */
static void cycleDisplayBuffers( void );

/*	@brief Destroy scenes with libgxm */
static void destroyGxmData( void );




/*	@brief Function to shut down libgxm and the graphics display services
	@return Error code result of processing during execution: <c> SCE_OK </c> on success,
	or another code depending upon the error
*/
static int shutdownGxm( void );



//METHODS********************************************************
static float makeFloat(unsigned char input);

//create a cube
static void createCube();
//create a face
static void createFace(int dir);
//create a tile
static void createTile(Matrix4f m, int dir, int k);
//assign Indices to turning arrays
static void assignIndices();
//reset turningids
static void resetFaceIds();
//reset VertexColors
static void resetColors();

static void matchColorcode();

static void createFaceData();
static float lerp(float a, float b);

static float lerp2(float a, float b);

static void buttonCtrl(SceCtrlData ctrl);

static void backTouchCtrl(SceTouchData backTouch);

static void raycast(SceTouchReport rep);
static void raycast2(SceTouchReport rep);

static void turnX(bool left,	int direction);
static void turnY(bool top,		int direction);
static void turnZ(bool inner,	int direction);



/*	@brief User main thread parameters */
extern const char			sceUserMainThreadName[]		= "simple_main_thr";
extern const int			sceUserMainThreadPriority	= SCE_KERNEL_DEFAULT_PRIORITY_USER;
extern const unsigned int	sceUserMainThreadStackSize	= SCE_KERNEL_STACK_SIZE_DEFAULT_USER_MAIN;

/*	@brief libc parameters */
unsigned int	sceLibcHeapSize	= 1*1024*1024;



/* Main entry point of program */
int main( void )
{
	cube = Logic();
	int returnCode = SCE_OK;

	//sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_HUD);
	sceSysmoduleLoadModule(SCE_SYSMODULE_PERF);
	sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_CAPTURE);

	/* initialize libdbgfont and libgxm */
	returnCode =initGxm();
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

    SceDbgFontConfig config;
	memset( &config, 0, sizeof(SceDbgFontConfig) );
	config.fontSize = SCE_DBGFONT_FONTSIZE_LARGE;

	returnCode = sceDbgFontInit( &config );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* Message for SDK sample auto test */
	printf( "## simple: INIT SUCCEEDED ##\n" );

	/* create gxm graphics data */
	createGxmData();

	/*set Joystickmode*/
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITALANALOG);

	/*set Touchpadmode*/
	sceTouchSetSamplingState( SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState( SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);


	/* 6. main loop */
	while ( true)
	{
        Update();
		render();
		cycleDisplayBuffers();
	}


    /*
	// 10. wait until rendering is done 
	sceGxmFinish( s_context );

	// destroy gxm graphics data 
	destroyGxmData();

	// shutdown libdbgfont and libgxm 
	returnCode = shutdownGxm();
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	// Message for SDK sample auto test
	printf( "## api_libdbgfont/simple: FINISHED ##\n" );

	return returnCode;
    */
}

void Update (void)
{
    // We go from -1 to 1 in rendering.
    float aspectRatio = (float)DISPLAY_WIDTH / (float)DISPLAY_HEIGHT;


	//**********read Joystick***********************************
	SceCtrlData ctrl;
	sceCtrlReadBufferPositive(0, &ctrl, 1);
	
	//**********read Touchpad**************
	SceTouchData touchFront;
	sceTouchRead(SCE_TOUCH_PORT_FRONT, &touchFront, 1);	
	int touchFrontReportNum = touchFront.reportNum;
	SceTouchData touchBack;
	sceTouchRead(SCE_TOUCH_PORT_BACK, &touchBack, 1);

	//************CameraTURNING*********************************
	
	Matrix4 mProj, mView;
	
	/***************backTouchCtrl*****************************/
	backTouchCtrl(touchBack);
	/***************backTouchCtrl*****************************/

	rotationM = Matrix4f(rotationQuat, Vector3(0.0f, 0.0f, 0.0f));
	mView = Matrix4f::lookAt(Point3f(0.f,0.f, -3.5f), Point3f(0.f,0.f,0.f), Vector3f(0.0f, -1.0f,0.0f));

	mProj = Matrix4f::perspective(0.6f, aspectRatio, 0.1f, 10.f);

	viewProj = mProj * mView;
	resultM = viewProj * rotationM;

	//************CameraTURNING*********************************
	

	
	buttonCtrl(ctrl);
	float angleX, angleZ, angleY;
	int turnAlongU;
	//************RAYCAST****************************
	//Get InverseWorld/RotationMatrix
	float xOnFace, yOnFace;
	Vector2 z, h, k;


	if(touchFrontReportNum > 0)
	{

		SceTouchReport rep = touchFront.report[0];

		//is it a firstTime touch
		if(rep.id != f_firstID)
		{
			raycast2(rep);
		}
		//is it a moving touch?
		else
		{
			//calculate curent screenspace x,y in [-1,1]
			f_currentA = (rep.x/1919.f) * 2.0f - 1.f ;
			f_currentB = -((rep.y/1087.f) * 2.0f - 1.f);

			z = Vector2(f_currentA-f_firstA, f_currentB - f_firstB);


			if( sqrt(z.getX()*z.getX() + z.getY() * z.getY()) > 0.1f && !turning)
			{
				//calculate screenspace u and v
				Vector4 _h = resultM * Vector4(faceData[touched_Face].uVec[0], faceData[touched_Face].uVec[1], faceData[touched_Face].uVec[2], 0.0f);
				Vector4 _k = resultM * Vector4(faceData[touched_Face].vVec[0], faceData[touched_Face].vVec[1], faceData[touched_Face].vVec[2], 0.0f);

				h = normalize(Vector2(_h.getXY()));
				k = normalize(Vector2(_k.getXY()));
				
				/******************************SWIPE TO TURN**************************************/
				//movement along the u vector?
				
				if(abs(dot(h, z)) < abs(dot(k, z))&&!turning)
				{
					turnAlongU = 1;
					//turn in X direction
					if((touched_Face == 2 && (touched_Tile == 6 || touched_Tile == 8))
						||(touched_Face == 3 && (touched_Tile == 0 || touched_Tile == 2))
						)
					{
						turnX(true, dot(k, z)>0?1:-1);
					}
					if((touched_Face == 2 && (touched_Tile == 0 || touched_Tile == 2))
						||(touched_Face == 3 && (touched_Tile == 6 || touched_Tile == 8))
						)
					{
						turnX(false, dot(k, z)>0?1:-1);
					}

					//turn in Y direction
					if((touched_Face == 1 && (touched_Tile == 0 || touched_Tile == 2))
						||(touched_Face == 5 && (touched_Tile == 0 || touched_Tile == 2))
						)
					{
						turnY(true, dot(k, z)>0?1:-1);
					}
					if((touched_Face == 1 && (touched_Tile == 6 || touched_Tile == 8))
						||(touched_Face == 5 && (touched_Tile == 6 || touched_Tile == 8))
						)
					{
						turnY(false, dot(k, z)>0?1:-1);
					}

					//turn in Z direction
					if((touched_Face == 0 && (touched_Tile == 6 || touched_Tile == 8))
						||(touched_Face == 4 && (touched_Tile == 6 || touched_Tile == 8))
						)
					{
						if(touched_Face == 0)
							turnZ(true, dot(k, z)>0?-1:1);
						else
							turnZ(true, dot(k, z)>0?1:-1);
					}
					if((touched_Face == 0 && (touched_Tile == 0 || touched_Tile == 2))
						||(touched_Face == 4 && (touched_Tile == 0 || touched_Tile == 2))
						)
					{
						if(touched_Face == 0)
							turnZ(false, dot(k, z)>0?-1:1);
						else
							turnZ(false, dot(k, z)>0?1:-1);
					}
					/******************************REALTIMETURNING************************************/
					//t += dot(k, z);
				}
				//along the v vector?
				else if(abs(dot(h, z)) > abs(dot(k, z)) &&!turning)
				{
					turnAlongU = -1;
					//turn in X direction
					if((touched_Face == 1 && (touched_Tile == 0 || touched_Tile == 6))
						||(touched_Face == 0 && (touched_Tile == 0 || touched_Tile == 6))
						)
					{
						turnX(true, dot(h, z)>0?1:-1);
					}
					if((touched_Face == 1 && (touched_Tile == 2 || touched_Tile == 8))
						||(touched_Face == 0 && (touched_Tile == 2 || touched_Tile == 8))
						)
					{
						turnX(false, dot(h, z)>0?1:-1);
					}

					//turn in Y direction
					if((touched_Face == 4 && (touched_Tile == 0 || touched_Tile == 6))
						||(touched_Face == 3 && (touched_Tile == 0 || touched_Tile == 6))
						)
					{
						turnY(true, dot(h, z)>0?1:-1);
					}
					if((touched_Face == 4 && (touched_Tile == 2 || touched_Tile == 8))
						||(touched_Face == 3 && (touched_Tile == 2 || touched_Tile == 8))
						)
					{
						turnY(false, dot(h, z)>0?1:-1);
					}

					//turn in Z direction
					if((touched_Face == 2 && (touched_Tile == 0 || touched_Tile == 6))
						||(touched_Face == 5 && (touched_Tile == 2 || touched_Tile == 8))
						)
					{
						if(touched_Face == 2)
							turnZ(true, dot(h, z)>0?-1:1);
						else
							turnZ(true, dot(h, z)>0?1:-1);
					}
					if((touched_Face == 2 && (touched_Tile == 2 || touched_Tile == 8))
						||(touched_Face == 5 && (touched_Tile == 0 || touched_Tile == 6))
						)
					{
						if(touched_Face == 2)
							turnZ(false, dot(h, z)>0?-1:1);
						else
							turnZ(false, dot(h, z)>0?1:-1);
					}
					/******************************REALTIMETURNING************************************/
				}
				/******************************SWIPE TO TURN**************************************/
			}

		}
		
		sprintf(resultline, "touching Face: %i touched Tile %i", touched_Face, touched_Tile);
	}
	
	else if(touchFrontReportNum <= 0 && turning)
	{
		
		turnDir = t<0?1:-1;
		turnAlongU = 0;
		if(t >= 0.2 &&t <= 1.0f)
		{
			turnDir = -1;
			float l = lerp(0, 1.0f * PI/2);
			angleX = l;
			faceX = Matrix4f::rotationX(angleX);
					
			angleZ = l;
			faceZ = Matrix4f::rotationZ(angleZ);
				
			angleY = l;
			faceY = Matrix4f::rotationY(angleY);
		}

		if(t <= -0.2 &&t >= -1.0f)
		{
			turnDir = 1;
			float l = lerp2(0, -1.0f * PI/2);
			angleX = l;
			faceX = Matrix4f::rotationX(angleX);
					
			angleZ = l;
			faceZ = Matrix4f::rotationZ(angleZ);
				
			angleY = l;
			faceY = Matrix4f::rotationY(angleY);
		}

		if(t < 0.2 && t > -0.2)
		{
			turnAlongU = 0;
		
			turning = false;
			createCube();
		}
		
	}

	//************FaceTURN******************************	
	if(turning)
	{
			
		/***************SWIPETOTURN******************************************/
		/*if(t <= 1.0f)
		{
			float l = lerp(0, turnDir * PI/2);
			angleX = l;
			faceX = Matrix4f::rotationX(angleX);
					
			angleZ = l;
			faceZ = Matrix4f::rotationZ(angleZ);
				
			angleY = l;
			faceY = Matrix4f::rotationY(angleY);
		}
		/***************SWIPETOTURN******************************************/

		/***************REALTIMETURN******************************************/
		if(t > 1.0f || t < -1.0f)
		{
			turnDir = t<0?1:-1;
			f_firstA = f_currentA;
			f_firstB = f_currentB;
			turnAlongU = 0;
		
			turning = false;
			switch(turnMode)
			{
				case 0:
					turnDir<0?cube.turnY(false):cube.turnYr(false);
					break;
				case 1:
					turnDir>0?cube.turnZ(true):cube.turnZr(true);
					break;
				case 2:
					turnDir<0?cube.turnY(true):cube.turnYr(true);	
					break;
				case 3:
					turnDir>0?cube.turnZ(false):cube.turnZr(false);
					break;
				case 4:
					turnDir<0?cube.turnX(false):cube.turnXr(false);
					break;
				case 5:
					turnDir<0?cube.turnX(true):cube.turnXr(true);
					break;
			}
			turnDir = 0;

			createCube();

			//resetFaceIds();
			//resetColors();
		}
		else
		{
			Vector4 _h = resultM * Vector4(faceData[touched_Face].uVec[0], faceData[touched_Face].uVec[1], faceData[touched_Face].uVec[2], 0.0f);
			Vector4 _k = resultM * Vector4(faceData[touched_Face].vVec[0], faceData[touched_Face].vVec[1], faceData[touched_Face].vVec[2], 0.0f);

			h = normalize(Vector2(_h.getXY()));
			k = normalize(Vector2(_k.getXY()));
			z = Vector2(f_currentA-f_firstA, f_currentB - f_firstB);
					
			float l = 0 + (PI/2 - 0) * t;
			angleX = l;
			faceX = Matrix4f::rotationX(angleX);
					
			angleZ = l;
			faceZ = Matrix4f::rotationZ(angleZ);
				
			angleY = l;
			faceY = Matrix4f::rotationY(angleY);
			

			if(turnAlongU == 1 && touched_Face == 0)
				t = -dot(k, z);
			else if(turnAlongU == 1)
				t = dot(k, z);
			if(turnAlongU == -1 && touched_Face == 0)
				t = -dot(h, z);
			else if(turnAlongU == -1)
				t = dot(h, z);

			


			sprintf(resultline, "t: %f, turnDir: %i", t, turnDir);

		}
		/***************REALTIMETURN******************************************/
		
	}


	//************FaceTurns************************************




	const SceChar8 *c = (SceChar8*) resultline;
    sceDbgFontPrint( 20, 20, 0xffffffff, c);
};

/* Initialize libgxm */
int initGxm( void )
{
/* ---------------------------------------------------------------------
	2. Initialize libgxm

	First we must initialize the libgxm library by calling sceGxmInitialize.
	The single argument to this function is the size of the parameter buffer to
	allocate for the GPU.  We will use the default 16MiB here.

	Once initialized, we need to create a rendering context to allow to us
	to render scenes on the GPU.  We use the default initialization
	parameters here to set the sizes of the various context ring buffers.

	Finally we create a render target to describe the geometry of the back
	buffers we will render to.  This object is used purely to schedule
	rendering jobs for the given dimensions, the color surface and
	depth/stencil surface must be allocated separately.
	--------------------------------------------------------------------- */

	int returnCode = SCE_OK;

	/* set up parameters */
	SceGxmInitializeParams initializeParams;
	memset( &initializeParams, 0, sizeof(SceGxmInitializeParams) );
	initializeParams.flags = 0;
	initializeParams.displayQueueMaxPendingCount = DISPLAY_MAX_PENDING_SWAPS;
	initializeParams.displayQueueCallback = displayCallback;
	initializeParams.displayQueueCallbackDataSize = sizeof(DisplayData);
	initializeParams.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

	/* start libgxm */
	returnCode = sceGxmInitialize( &initializeParams );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* allocate ring buffer memory using default sizes */
	void *vdmRingBuffer = graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, 4, SCE_GXM_MEMORY_ATTRIB_READ, &s_vdmRingBufferUId );

	void *vertexRingBuffer = graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, 4, SCE_GXM_MEMORY_ATTRIB_READ, &s_vertexRingBufferUId );

	void *fragmentRingBuffer = graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, 4, SCE_GXM_MEMORY_ATTRIB_READ, &s_fragmentRingBufferUId );

	uint32_t fragmentUsseRingBufferOffset;
	void *fragmentUsseRingBuffer = fragmentUsseAlloc( SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE, &s_fragmentUsseRingBufferUId, &fragmentUsseRingBufferOffset );

	/* create a rendering context */
	memset( &s_contextParams, 0, sizeof(SceGxmContextParams) );
	s_contextParams.hostMem = malloc( SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE );
	s_contextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	s_contextParams.vdmRingBufferMem = vdmRingBuffer;
	s_contextParams.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
	s_contextParams.vertexRingBufferMem = vertexRingBuffer;
	s_contextParams.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
	s_contextParams.fragmentRingBufferMem = fragmentRingBuffer;
	s_contextParams.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
	s_contextParams.fragmentUsseRingBufferMem = fragmentUsseRingBuffer;
	s_contextParams.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
	s_contextParams.fragmentUsseRingBufferOffset = fragmentUsseRingBufferOffset;
	returnCode = sceGxmCreateContext( &s_contextParams, &s_context );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* set buffer sizes for this sample */
	const uint32_t patcherBufferSize = 64*1024;
	const uint32_t patcherVertexUsseSize = 64*1024;
	const uint32_t patcherFragmentUsseSize = 64*1024;

	/* allocate memory for buffers and USSE code */
	void *patcherBuffer = graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, patcherBufferSize, 4, SCE_GXM_MEMORY_ATTRIB_WRITE|SCE_GXM_MEMORY_ATTRIB_WRITE, &s_patcherBufferUId );

	uint32_t patcherVertexUsseOffset;
	void *patcherVertexUsse = vertexUsseAlloc( patcherVertexUsseSize, &s_patcherVertexUsseUId, &patcherVertexUsseOffset );

	uint32_t patcherFragmentUsseOffset;
	void *patcherFragmentUsse = fragmentUsseAlloc( patcherFragmentUsseSize, &s_patcherFragmentUsseUId, &patcherFragmentUsseOffset );

	/* create a shader patcher */
	SceGxmShaderPatcherParams patcherParams;
	memset( &patcherParams, 0, sizeof(SceGxmShaderPatcherParams) );
	patcherParams.userData = NULL;
	patcherParams.hostAllocCallback = &patcherHostAlloc;
	patcherParams.hostFreeCallback = &patcherHostFree;
	patcherParams.bufferAllocCallback = NULL;
	patcherParams.bufferFreeCallback = NULL;
	patcherParams.bufferMem = patcherBuffer;
	patcherParams.bufferMemSize = patcherBufferSize;
	patcherParams.vertexUsseAllocCallback = NULL;
	patcherParams.vertexUsseFreeCallback = NULL;
	patcherParams.vertexUsseMem = patcherVertexUsse;
	patcherParams.vertexUsseMemSize = patcherVertexUsseSize;
	patcherParams.vertexUsseOffset = patcherVertexUsseOffset;
	patcherParams.fragmentUsseAllocCallback = NULL;
	patcherParams.fragmentUsseFreeCallback = NULL;
	patcherParams.fragmentUsseMem = patcherFragmentUsse;
	patcherParams.fragmentUsseMemSize = patcherFragmentUsseSize;
	patcherParams.fragmentUsseOffset = patcherFragmentUsseOffset;
	returnCode = sceGxmShaderPatcherCreate( &patcherParams, &s_shaderPatcher );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* create a render target */
	memset( &s_renderTargetParams, 0, sizeof(SceGxmRenderTargetParams) );
	s_renderTargetParams.flags = 0;
	s_renderTargetParams.width = DISPLAY_WIDTH;
	s_renderTargetParams.height = DISPLAY_HEIGHT;
	s_renderTargetParams.scenesPerFrame = 1;
	s_renderTargetParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
	s_renderTargetParams.multisampleLocations	= 0;
	s_renderTargetParams.driverMemBlock = SCE_UID_INVALID_UID;

	returnCode = sceGxmCreateRenderTarget( &s_renderTargetParams, &s_renderTarget );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );


/* ---------------------------------------------------------------------
	3. Allocate display buffers, set up the display queue

	We will allocate our back buffers in CDRAM, and create a color
	surface for each of them.

	To allow display operations done by the CPU to be synchronized with
	rendering done by the GPU, we also create a SceGxmSyncObject for each
	display buffer.  This sync object will be used with each scene that
	renders to that buffer and when queueing display flips that involve
	that buffer (either flipping from or to).

	Finally we create a display queue object that points to our callback
	function.
	--------------------------------------------------------------------- */

	/* allocate memory and sync objects for display buffers */
	for ( unsigned int i = 0 ; i < DISPLAY_BUFFER_COUNT ; ++i )
	{
		/* allocate memory with large size to ensure physical contiguity */
		s_displayBufferData[i] = graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA, ALIGN(4*DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT, 1*1024*1024), SCE_GXM_COLOR_SURFACE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE, &s_displayBufferUId[i] );
		SCE_DBG_ALWAYS_ASSERT( s_displayBufferData[i] );

		/* memset the buffer to debug color */
		for ( unsigned int y = 0 ; y < DISPLAY_HEIGHT ; ++y )
		{
			unsigned int *row = (unsigned int *)s_displayBufferData[i] + y*DISPLAY_STRIDE_IN_PIXELS;

			for ( unsigned int x = 0 ; x < DISPLAY_WIDTH ; ++x )
			{
				row[x] = 0x0;
			}
		}

		/* initialize a color surface for this display buffer */
		returnCode = sceGxmColorSurfaceInit( &s_displaySurface[i], DISPLAY_COLOR_FORMAT, SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE,
											 SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS, s_displayBufferData[i] );
		SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

		/* create a sync object that we will associate with this buffer */
		returnCode = sceGxmSyncObjectCreate( &s_displayBufferSync[i] );
		SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );
	}

	/* compute the memory footprint of the depth buffer */
	const uint32_t alignedWidth = ALIGN( DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX );
	const uint32_t alignedHeight = ALIGN( DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY );
	uint32_t sampleCount = alignedWidth*alignedHeight;
	uint32_t depthStrideInSamples = alignedWidth;

	/* allocate it */
	void *depthBufferData = graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 4*sampleCount, SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE, &s_depthBufferUId );

	/* create the SceGxmDepthStencilSurface structure */
	returnCode = sceGxmDepthStencilSurfaceInit( &s_depthSurface, SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED, depthStrideInSamples, depthBufferData, NULL );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	return returnCode;
}

/* Create libgxm scenes */
void createGxmData( void )
{
	//*******************Texture********************************************

	
	//datei öffnen
	SceUID openedGxt = sceIoOpen(texturePath, SCE_O_RDONLY, 0);

	//Größe der Datei in Bytes
	int size = sceIoLseek(openedGxt, 0, SCE_SEEK_END);

	//speicher anfordern
	void *file = malloc(size);

	//seek auf anfang
	sceIoLseek(openedGxt, 0, SCE_SEEK_SET);
	
	//datei laden
	sceIoRead(openedGxt, file, size);

	//datei schließen
	sceIoClose(openedGxt);


	//copy to memory
	textureData = (void*)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ, &gxt);
	
	const uint32_t dataSize = sceGxtGetDataSize(file);
	const void* dataSrc = sceGxtGetDataAddress(file);
	memcpy(textureData, dataSrc, dataSize);
	sceGxtInitTexture(&textureMap, file, textureData, 0);
	sceGxtInitTexture(&normalMap, file, textureData, 1);

	//free memory
	free(file);

	sceGxmTextureSetMinFilter(&textureMap, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMagFilter(&textureMap, SCE_GXM_TEXTURE_FILTER_LINEAR);

	
	sceGxmTextureSetMinFilter(&normalMap, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMagFilter(&normalMap, SCE_GXM_TEXTURE_FILTER_LINEAR);




/* ---------------------------------------------------------------------
	4. Create a shader patcher and register programs

	A shader patcher object is required to produce vertex and fragment
	programs from the shader compiler output.  First we create a shader
	patcher instance, using callback functions to allow it to allocate
	and free host memory for internal state.

	In order to create vertex and fragment programs for a particular
	shader, the compiler output must first be registered to obtain an ID
	for that shader.  Within a single ID, vertex and fragment programs
	are reference counted and could be shared if created with identical
	parameters.  To maximise this sharing, programs should only be
	registered with the shader patcher once if possible, so we will do
	this now.
	--------------------------------------------------------------------- */

	/* register programs with the patcher */
	int returnCode = sceGxmShaderPatcherRegisterProgram( s_shaderPatcher, &binaryClearVGxpStart, &s_clearVertexProgramId );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );
	returnCode = sceGxmShaderPatcherRegisterProgram( s_shaderPatcher, &binaryClearFGxpStart, &s_clearFragmentProgramId );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );


    returnCode = sceGxmShaderPatcherRegisterProgram( s_shaderPatcher, &binaryBasicVGxpStart, &s_basicVertexProgramId );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );
	returnCode = sceGxmShaderPatcherRegisterProgram( s_shaderPatcher, &binaryBasicFGxpStart, &s_basicFragmentProgramId );
    SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );


/* ---------------------------------------------------------------------
	5. Create the programs and data for the clear

	On SGX hardware, vertex programs must perform the unpack operations
	on vertex data, so we must define our vertex formats in order to
	create the vertex program.  Similarly, fragment programs must be
	specialized based on how they output their pixels and MSAA mode
	(and texture format on ES1).

	We define the clear geometry vertex format here and create the vertex
	and fragment program.

	The clear vertex and index data is static, we allocate and write the
	data here.
	--------------------------------------------------------------------- */

	/* get attributes by name to create vertex format bindings */
	const SceGxmProgram *clearProgram = sceGxmShaderPatcherGetProgramFromId( s_clearVertexProgramId );
	SCE_DBG_ALWAYS_ASSERT( clearProgram );
	const SceGxmProgramParameter *paramClearPositionAttribute = sceGxmProgramFindParameterByName( clearProgram, "aPosition" );
	SCE_DBG_ALWAYS_ASSERT( paramClearPositionAttribute && ( sceGxmProgramParameterGetCategory(paramClearPositionAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	/* create clear vertex format */
	SceGxmVertexAttribute clearVertexAttributes[1];
	SceGxmVertexStream clearVertexStreams[1];
	clearVertexAttributes[0].streamIndex = 0;
	clearVertexAttributes[0].offset = 0;
	clearVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	clearVertexAttributes[0].componentCount = 2;
	clearVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex( paramClearPositionAttribute );
	clearVertexStreams[0].stride = sizeof(ClearVertex);
	clearVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	/* create clear programs */
	returnCode = sceGxmShaderPatcherCreateVertexProgram( s_shaderPatcher, s_clearVertexProgramId, clearVertexAttributes, 1, clearVertexStreams, 1, &s_clearVertexProgram );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	returnCode = sceGxmShaderPatcherCreateFragmentProgram( s_shaderPatcher, s_clearFragmentProgramId,
														   SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, NULL,
														   sceGxmShaderPatcherGetProgramFromId(s_clearVertexProgramId), &s_clearFragmentProgram );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* create the clear triangle vertex/index data */
	s_clearVertices = (ClearVertex *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 3*sizeof(ClearVertex), 4, SCE_GXM_MEMORY_ATTRIB_READ, &s_clearVerticesUId );
	s_clearIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 3*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &s_clearIndicesUId );

	s_clearVertices[0].x = -1.0f;
	s_clearVertices[0].y = -1.0f;
	s_clearVertices[1].x =  3.0f;
	s_clearVertices[1].y = -1.0f;
	s_clearVertices[2].x = -1.0f;
	s_clearVertices[2].y =  3.0f;

	s_clearIndices[0] = 0;
	s_clearIndices[1] = 1;
	s_clearIndices[2] = 2;

    // !! All related to triangle.

    /* get attributes by name to create vertex format bindings */
	/* first retrieve the underlying program to extract binding information */
	const SceGxmProgram *basicProgram = sceGxmShaderPatcherGetProgramFromId( s_basicVertexProgramId );
	SCE_DBG_ALWAYS_ASSERT( basicProgram );
	const SceGxmProgramParameter *paramBasicPositionAttribute = sceGxmProgramFindParameterByName( basicProgram, "aPosition" );
	SCE_DBG_ALWAYS_ASSERT( paramBasicPositionAttribute && ( sceGxmProgramParameterGetCategory(paramBasicPositionAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	const SceGxmProgramParameter *paramBasicNormalAttribute = sceGxmProgramFindParameterByName( basicProgram, "aNormal" );
	SCE_DBG_ALWAYS_ASSERT( paramBasicNormalAttribute && ( sceGxmProgramParameterGetCategory(paramBasicNormalAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	const SceGxmProgramParameter *paramBasicTangentAttribute = sceGxmProgramFindParameterByName( basicProgram, "aTangent" );
	SCE_DBG_ALWAYS_ASSERT( paramBasicTangentAttribute && ( sceGxmProgramParameterGetCategory(paramBasicTangentAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	const SceGxmProgramParameter *paramBasicTexCoordAttribute = sceGxmProgramFindParameterByName( basicProgram, "texIn" );
	SCE_DBG_ALWAYS_ASSERT( paramBasicTexCoordAttribute && ( sceGxmProgramParameterGetCategory(paramBasicTexCoordAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	const SceGxmProgramParameter *paramBasicColorAttribute = sceGxmProgramFindParameterByName( basicProgram, "aColor" );
	SCE_DBG_ALWAYS_ASSERT( paramBasicColorAttribute && ( sceGxmProgramParameterGetCategory(paramBasicColorAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	const SceGxmProgramParameter *paramBasicFaceIDAttribute = sceGxmProgramFindParameterByName( basicProgram, "turnFaceID" );
	SCE_DBG_ALWAYS_ASSERT( paramBasicFaceIDAttribute && ( sceGxmProgramParameterGetCategory(paramBasicFaceIDAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE ) );

	/* create shaded triangle vertex format */
	SceGxmVertexAttribute basicVertexAttributes[6];
	SceGxmVertexStream basicVertexStreams[1];

	basicVertexAttributes[0].streamIndex = 0;
	basicVertexAttributes[0].offset = 0;
	basicVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	basicVertexAttributes[0].componentCount = 3;
	basicVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex( paramBasicPositionAttribute );
	
	basicVertexAttributes[1].streamIndex = 0;
	basicVertexAttributes[1].offset = 12;
	basicVertexAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	basicVertexAttributes[1].componentCount = 3;
	basicVertexAttributes[1].regIndex = sceGxmProgramParameterGetResourceIndex( paramBasicNormalAttribute );
	
	basicVertexAttributes[2].streamIndex = 0;
	basicVertexAttributes[2].offset = 24;
	basicVertexAttributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	basicVertexAttributes[2].componentCount = 3;
	basicVertexAttributes[2].regIndex = sceGxmProgramParameterGetResourceIndex( paramBasicTangentAttribute );

	basicVertexAttributes[3].streamIndex = 0;
	basicVertexAttributes[3].offset = 36;
	basicVertexAttributes[3].format = SCE_GXM_ATTRIBUTE_FORMAT_F32; // Mapping relation clarified.
	basicVertexAttributes[3].componentCount = 2;
	basicVertexAttributes[3].regIndex = sceGxmProgramParameterGetResourceIndex( paramBasicTexCoordAttribute );

	basicVertexAttributes[4].streamIndex = 0;
	basicVertexAttributes[4].offset = 44;
	basicVertexAttributes[4].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N; // Mapping relation clarified.
	basicVertexAttributes[4].componentCount = 4;
	basicVertexAttributes[4].regIndex = sceGxmProgramParameterGetResourceIndex( paramBasicColorAttribute );

	basicVertexAttributes[5].streamIndex = 0;
	basicVertexAttributes[5].offset = 48;
	basicVertexAttributes[5].format = SCE_GXM_ATTRIBUTE_FORMAT_U8; // Mapping relation clarified.
	basicVertexAttributes[5].componentCount = 1;
	basicVertexAttributes[5].regIndex = sceGxmProgramParameterGetResourceIndex( paramBasicFaceIDAttribute );

	basicVertexStreams[0].stride = sizeof(BasicVertex);
	basicVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	/* create shaded triangle shaders */
	returnCode = sceGxmShaderPatcherCreateVertexProgram( s_shaderPatcher, s_basicVertexProgramId, basicVertexAttributes, 6,
														 basicVertexStreams, 1, &s_basicVertexProgram );

	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	returnCode = sceGxmShaderPatcherCreateFragmentProgram( s_shaderPatcher, s_basicFragmentProgramId,
														   SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, NULL,
														   sceGxmShaderPatcherGetProgramFromId(s_basicVertexProgramId), &s_basicFragmentProgram );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* find vertex uniforms by name and cache parameter information */
	s_wvpParam = sceGxmProgramFindParameterByName( basicProgram, "wvp" );
	rotParam = sceGxmProgramFindParameterByName( basicProgram, "rot" );
	rotXParam  = sceGxmProgramFindParameterByName( basicProgram, "faceX" );
	rotYParam  = sceGxmProgramFindParameterByName( basicProgram, "faceY" );
	rotZParam  = sceGxmProgramFindParameterByName( basicProgram, "faceZ" );

	//create cube

	faceVertices = (BasicVertex *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 6*9*4*sizeof(BasicVertex), 4, SCE_GXM_MEMORY_ATTRIB_READ, &faceVerticesUId );
	faceIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 6*9*6*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &faceIndiceUId );
	

	
	int count  = 0;
	int baseIndex = 0;
	for(int tiles = 0; tiles < 9*6; ++tiles)
	{
			
		faceIndices[count++] = baseIndex;
		faceIndices[count++] = baseIndex+1;
		faceIndices[count++] = baseIndex+2;
 		  
		faceIndices[count++] = baseIndex+1;
		faceIndices[count++] = baseIndex+2;
		faceIndices[count++] = baseIndex+3;

		baseIndex+=4;
	}

	
	createFaceData();
	createCube();
	assignIndices();

	
}

void assignIndices()
{
	greenIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 21*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &greenIndiceUId );
	redIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 21*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &redIndiceUId );
	blueIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 21*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &blueIndiceUId );
	orangeIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 21*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &orangeIndiceUId );
	whiteIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 21*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &whiteIndiceUId );
	yellowIndices = (uint16_t *)graphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 21*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &yellowIndiceUId );

	//green
	for(int i = 0; i < 9; i++)
		{
			greenIndices[i]	 = 0*36+i*4+0;

			redIndices[i]	 = 1*36+i*4+0;

			blueIndices[i]	 = 2*36+i*4+0;

			orangeIndices[i] = 3*36+i*4+0;

			whiteIndices[i]	 = 4*36+i*4+0;

			yellowIndices[i] = 5*36+i*4+0;

		}
		
		greenIndices[9 + 0]		= 1*36 + 2*4;
		greenIndices[9 + 1]		= 1*36 + 5*4;
		greenIndices[9 + 2]		= 1*36 + 8*4;
									    
		greenIndices[9 + 3]		= 3*36 + 2*4;
		greenIndices[9 + 4]		= 3*36 + 5*4;
		greenIndices[9 + 5]		= 3*36 + 8*4;
					 				 	 
		greenIndices[9 + 6]		= 4*36 + 2*4;
		greenIndices[9 + 7]		= 4*36 + 5*4;
		greenIndices[9 + 8]		= 4*36 + 8*4;
					 				    
		greenIndices[9 + 9]		= 5*36 + 2*4;
		greenIndices[9 + 10]	= 5*36 + 5*4;
		greenIndices[9 + 11]	= 5*36 + 8*4;
		

		redIndices[9 + 0]		= 0*36 + 0*4;
		redIndices[9 + 1]		= 0*36 + 3*4;
		redIndices[9 + 2]		= 0*36 + 6*4;
									    
		redIndices[9 + 3]		= 2*36 + 0*4;
		redIndices[9 + 4]		= 2*36 + 3*4;
		redIndices[9 + 5]		= 2*36 + 6*4;
					 				   
		redIndices[9 + 6]		= 4*36 + 6*4;
		redIndices[9 + 7]		= 4*36 + 7*4;
		redIndices[9 + 8]		= 4*36 + 8*4;
					 				   
		redIndices[9 + 9]		= 5*36 + 6*4;
		redIndices[9 + 10]		= 5*36 + 7*4;
		redIndices[9 + 11]		= 5*36 + 8*4;


		blueIndices[9 + 0]		= 1*36 + 0*4;
		blueIndices[9 + 1]		= 1*36 + 3*4;
		blueIndices[9 + 2]		= 1*36 + 6*4;
								    
		blueIndices[9 + 3]		= 3*36 + 0*4;
		blueIndices[9 + 4]		= 3*36 + 3*4;
		blueIndices[9 + 5]		= 3*36 + 6*4;
				 				    
		blueIndices[9 + 6]		= 4*36 + 0*4;
		blueIndices[9 + 7]		= 4*36 + 3*4;
		blueIndices[9 + 8]		= 4*36 + 6*4;
				 				    
		blueIndices[9 + 9]		= 5*36 + 0*4;
		blueIndices[9 + 10]		= 5*36 + 3*4;
		blueIndices[9 + 11]		= 5*36 + 6*4;
									
									
		orangeIndices[9 + 0]	= 0*36 + 2*4;
		orangeIndices[9 + 1]	= 0*36 + 5*4;
		orangeIndices[9 + 2]	= 0*36 + 8*4;
									   
		orangeIndices[9 + 3]	= 2*36 + 2*4;
		orangeIndices[9 + 4]	= 2*36 + 5*4;
		orangeIndices[9 + 5]	= 2*36 + 8*4;
					 				   
		orangeIndices[9 + 6]	= 4*36 + 0*4;
		orangeIndices[9 + 7]	= 4*36 + 1*4;
		orangeIndices[9 + 8]	= 4*36 + 2*4;
					 				   
		orangeIndices[9 + 9]	= 5*36 + 0*4;
		orangeIndices[9 + 10]	= 5*36 + 1*4;
		orangeIndices[9 + 11]	= 5*36 + 2*4;
		

		whiteIndices[9 + 0]		= 0*36 + 6*4;
		whiteIndices[9 + 1]		= 0*36 + 7*4;
		whiteIndices[9 + 2]		= 0*36 + 8*4;
								    
		whiteIndices[9 + 3]		= 1*36 + 6*4;
		whiteIndices[9 + 4]		= 1*36 + 7*4;
		whiteIndices[9 + 5]		= 1*36 + 8*4;
			 					    
		whiteIndices[9 + 6]		= 2*36 + 6*4;
		whiteIndices[9 + 7]		= 2*36 + 7*4;
		whiteIndices[9 + 8]		= 2*36 + 8*4;
			 					    
		whiteIndices[9 + 9]		= 3*36 + 6*4;
		whiteIndices[9 + 10]	= 3*36 + 7*4;
		whiteIndices[9 + 11]	= 3*36 + 8*4;
			

		yellowIndices[9 + 0]	= 0*36 + 0*4;
		yellowIndices[9 + 1]	= 0*36 + 1*4;
		yellowIndices[9 + 2]	= 0*36 + 2*4;
									
		yellowIndices[9 + 3]	= 1*36 + 0*4;
		yellowIndices[9 + 4]	= 1*36 + 1*4;
		yellowIndices[9 + 5]	= 1*36 + 2*4;
									
		yellowIndices[9 + 6]	= 2*36 + 0*4;
		yellowIndices[9 + 7]	= 2*36 + 1*4;
		yellowIndices[9 + 8]	= 2*36 + 2*4;
									
		yellowIndices[9 + 9]	= 3*36 + 0*4;
		yellowIndices[9 + 10]	= 3*36 + 1*4;
		yellowIndices[9 + 11]	= 3*36 + 2*4;
}

void resetFaceIds()
{
	for(int i = 0; i<54; i++)
	{
		faceVertices[i*4+0].faceTurnID = 7;
		faceVertices[i*4+1].faceTurnID = 7;
		faceVertices[i*4+2].faceTurnID = 7;
		faceVertices[i*4+3].faceTurnID = 7;
	}
}

int matchColorcode(char c)
{
	switch(c)
	{
		case 'g':
			return 0x0000ff00;
		case 'r':
			return 0x000000ff;
		case 'b':
			return 0x00ff0000;
		case 'o':
			return 0x0000a3ff;
		case 'w':
			return 0x00ffffff;
		case 'y':
			return 0x0000ffff;
		default:
			return 0;
	}
}

void resetColors()
{
	for(int i = 0; i < 9; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				faceVertices[greenIndices[i]+j].color	= matchColorcode(cube.getFace('g')[i]);
													 									
				faceVertices[redIndices[i]+j].color		= matchColorcode(cube.getFace('r')[i]);
													  									
				faceVertices[blueIndices[i]+j].color	= matchColorcode(cube.getFace('b')[i]);
																						
				faceVertices[orangeIndices[i]+j].color	= matchColorcode(cube.getFace('o')[i]);
													  									
				faceVertices[whiteIndices[i]+j].color	= matchColorcode(cube.getFace('w')[i]);
																						
				faceVertices[yellowIndices[i]+j].color	= matchColorcode(cube.getFace('y')[i]);
			}
		}
}

float makeFloat(unsigned char input)
{
    return (((float)(input)) / 128.0f) - 1.0f;
}

void createCube()
{
	for(int i = 0; i < 6; ++i)
	{
		
		createFace(i);
	}

	resetFaceIds();
}

void createFaceData()
{
	//green
	faceData[0].oVec[0] = 0.5f;
	faceData[0].oVec[1] = 0.0f;
	faceData[0].oVec[2] = 0.0f;

	faceData[0].nVec[0] = 1.0f;
	faceData[0].nVec[1] = 0.0f;
	faceData[0].nVec[2] = 0.0f;

	faceData[0].uVec[0] = 0.0f;
	faceData[0].uVec[1] = 0.0f;
	faceData[0].uVec[2] = -1.0f;

	faceData[0].vVec[0] = 0.0f;
	faceData[0].vVec[1] = 1.0f;
	faceData[0].vVec[2] = 0.0f;

	//red
	faceData[1].oVec[0] = 0.0f;
	faceData[1].oVec[1] = 0.0f;
	faceData[1].oVec[2] = 0.5f;

	faceData[1].nVec[0] = 0.0f;
	faceData[1].nVec[1] = 0.0f;
	faceData[1].nVec[2] = 1.0f;

	faceData[1].uVec[0] = -1.0f;
	faceData[1].uVec[1] = 0.0f;
	faceData[1].uVec[2] = 0.0f;

	faceData[1].vVec[0] = 0.0f;
	faceData[1].vVec[1] = 1.0f;
	faceData[1].vVec[2] = 0.0f;

	//blue
	faceData[2].oVec[0] = -0.5f;
	faceData[2].oVec[1] = 0.0f;
	faceData[2].oVec[2] = 0.0f;

	faceData[2].nVec[0] = -1.0f;
	faceData[2].nVec[1] = 0.0f;
	faceData[2].nVec[2] = 0.0f;

	faceData[2].uVec[0] = 0.0f;
	faceData[2].uVec[1] = 1.0f;
	faceData[2].uVec[2] = 0.0f;

	faceData[2].vVec[0] = 0.0f;
	faceData[2].vVec[1] = 0.0f;
	faceData[2].vVec[2] = -1.0f;

	//orange
	faceData[3].oVec[0] = 0.0f;
	faceData[3].oVec[1] = 0.0f;
	faceData[3].oVec[2] = -0.5f;

	faceData[3].nVec[0] = 0.0f;
	faceData[3].nVec[1] = 0.0f;
	faceData[3].nVec[2] = -1.0f;

	faceData[3].uVec[0] = 0.0f;
	faceData[3].uVec[1] = -1.0f;
	faceData[3].uVec[2] = 0.0f;

	faceData[3].vVec[0] = 1.0f;
	faceData[3].vVec[1] = 0.0f;
	faceData[3].vVec[2] = 0.0f;

	//white
	faceData[4].oVec[0] = 0.0f;
	faceData[4].oVec[1] = 0.5f;
	faceData[4].oVec[2] = 0.0f;
	
	faceData[4].nVec[0] = 0.0f;
	faceData[4].nVec[1] = 1.0f;
	faceData[4].nVec[2] = 0.0f;
	
	faceData[4].uVec[0] = 0.0f;
	faceData[4].uVec[1] = 0.0f;
	faceData[4].uVec[2] = -1.0f;
	
	faceData[4].vVec[0] = 1.0f;
	faceData[4].vVec[1] = 0.0f;
	faceData[4].vVec[2] = 0.0f;

	//yellow
	faceData[5].oVec[0] = 0.0f;
	faceData[5].oVec[1] = -0.5f;
	faceData[5].oVec[2] = 0.0f;
			 
	faceData[5].nVec[0] = 0.0f;
	faceData[5].nVec[1] = -1.0f;
	faceData[5].nVec[2] = 0.0f;
			 
	faceData[5].uVec[0] = -1.0f;
	faceData[5].uVec[1] = 0.0f;
	faceData[5].uVec[2] = 0.0f;
			 
	faceData[5].vVec[0] = 0.0f;
	faceData[5].vVec[1] = 0.0f;
	faceData[5].vVec[2] = 1.0f;
}

void createFace(int dir)
{

	Matrix4f face, tile;
	int color;
	face = Matrix4f::identity();
	switch(dir)
	{
		case 0: //green
			face  = Matrix4f::rotationY(PI/2);
			face.setTranslation(Vector3f(cubeSize/2, 0.0f, 0.0f));
			break;

		case 1: //red
			face  = Matrix4f::rotationY(0.f);
			face.setTranslation(Vector3f(0.0f, 0.0f, cubeSize/2));
			break;

		case 2: //blue
			face  = Matrix4f::rotationY(PI/2.0f);	
			face.setTranslation(Vector3f(-cubeSize/2, 0.0f, 0.0f));
			break;
		case 3: //orange
			face  = Matrix4f::rotationY(0.f);
			face.setTranslation(Vector3f(0.0f, 0.0f, -cubeSize/2));
			break;
		case 4: //white
			face  = Matrix4f::rotationX(PI/2.0f);
			face.setTranslation(Vector3f(0.0f, cubeSize/2, 0.0f));
			break;
		case 5: //yellow
			face  = Matrix4f::rotationX(PI/2.0f);
			face.setTranslation(Vector3f(0.0f, -cubeSize/2, 0.0f));
			break;
	}

	int k = 0;
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			tile = Matrix4f::identity();
			tile.setTranslation(Vector3f((j*tileSize) + tileSize/2 - cubeSize/2, (i*tileSize) + tileSize/2- cubeSize/2, 0.0f));
			createTile(face * tile, dir, k);
			++k;
		}
	}
}

void createTile(Matrix4f m, int dir, int k)
{
	int color;
	char logic;

	//initialize Edgepositions
	Vector4f* v = new Vector4f[4];
	v[0] = m * Vector4f(-(tileSize/2),-(tileSize/2), 0.f,1);
	v[1] = m * Vector4f((tileSize/2),-(tileSize/2), 0.f,1);
	v[2] = m * Vector4f(-(tileSize/2),(tileSize/2), 0.f,1);
	v[3] = m * Vector4f((tileSize/2),(tileSize/2), 0.f,1);

	switch(dir)
	{
		
		case 0: //green
			logic = cube.getFace('g')[k];
			break;
		case 1: //red
			logic = cube.getFace('r')[k];
			break;
		case 2: //blue
			logic = cube.getFace('b')[k];
			break;
		case 3: //orange
			logic = cube.getFace('o')[k];
			break;
		case 4: //white
			logic = cube.getFace('w')[k];
			break;
		case 5: //yellow
			logic = cube.getFace('y')[k];
			break;
			
	}

	switch(logic)
	{
		case 'g':
			color = 0x0000ff00;
			break;		
		case 'r':
			color = 0x000000ff;
			break;	
		case 'b':
			color = 0x00ff0000;
			break;	
		case 'o':
			color = 0x0000a3ff;
			break;	
		case 'w':
			color = 0x00ffffff;
			break;	
		case 'y':
			color = 0x0000ffff;
			break;	
	}
	
	for(int i = 0; i < 4; ++i)
	{
		faceVertices[dir*36+k*4+i].position[0] = v[i].getX();
		faceVertices[dir*36+k*4+i].position[1] = v[i].getY();
		faceVertices[dir*36+k*4+i].position[2] = v[i].getZ();
		faceVertices[dir*36+k*4+i].normal[0] = faceData[dir].nVec[0];
		faceVertices[dir*36+k*4+i].normal[1] = faceData[dir].nVec[1];
		faceVertices[dir*36+k*4+i].normal[2] = faceData[dir].nVec[2];
		faceVertices[dir*36+k*4+i].tangent[0] = -faceData[dir].uVec[0];
		faceVertices[dir*36+k*4+i].tangent[1] = -faceData[dir].uVec[1];
		faceVertices[dir*36+k*4+i].tangent[2] = -faceData[dir].uVec[2];
		faceVertices[dir*36+k*4+i].color = color;
		faceVertices[dir*36+k*4+i].faceTurnID = 7;
		/*if(k == 0 && i == 0)
			faceVertices[dir*36+k*4+i].color = 0x00000000;*/

		switch(i)
		{
			case 0:
				faceVertices[dir*36+k*4+i].uv[0] = 0.0f;
				faceVertices[dir*36+k*4+i].uv[1] = 0.0f;
				break;
			case 1:
				faceVertices[dir*36+k*4+i].uv[0] = 1.0f;
				faceVertices[dir*36+k*4+i].uv[1] = 0.0f;
				break;
			case 2:
				faceVertices[dir*36+k*4+i].uv[0] = 0.0f;
				faceVertices[dir*36+k*4+i].uv[1] = 1.0f;
				break;
			case 3:
				faceVertices[dir*36+k*4+i].uv[0] = 1.0f;
				faceVertices[dir*36+k*4+i].uv[1] = 1.0f;
				break;
		}

	}

}

float lerp(float a, float b)
{	
	t += deltaTime * rotationSpeed; 
	return a + (b-a) * t;
}

float lerp2(float a, float b)
{	
	t -= deltaTime * rotationSpeed; 
	return a + (b-a) * t;
}

void backTouchCtrl(SceTouchData touchBack)
{
		
	int touchBackReportNum = touchBack.reportNum;
	float lx, ly, rx;
	lx = ly = rx = 0.0f;
	if(touchBackReportNum > 0)
	{
		SceTouchReport rep = touchBack.report[0];
		SceTouchReport rep1;

		//calculate screencoordinates a,b in -1/1
		float currentA = (rep.x/1919.f) * 2.0f - 1.f ;
		float currentB = (rep.y/1087.f) * 2.0f - 1.f;

		int currentID = rep.id;

		if(touchBackReportNum > 1)
		{
			rep1 = touchBack.report[1];
			if(currentID != lastID)
			{
				lastA = currentA;
				lastB = currentB;
				lastID = currentID;
			}
			else if(lastA - currentA != 0 && lastB - currentB != 0)
			{
				lastB - currentB < 0? rx = abs(lastB - currentB) * 0.2f: rx = -abs(lastB - currentB) * 0.2f;
			
				lastA = currentA;
				lastB = currentB;
			}
		}
		else
		{
			if(currentID != lastID)
			{
				lastA = currentA;
				lastB = currentB;
				lastID = currentID;
			}
			else if(lastA - currentA != 0 && lastB - currentB != 0)
			{
				lastA - currentA < 0? lx = abs(lastA - currentA) * 0.2f: lx = -abs(lastA - currentA) * 0.2f;
				lastB - currentB < 0? ly = abs(lastB - currentB) * 0.2f: ly = -abs(lastB - currentB) * 0.2f;
			
				lastA = currentA;
				lastB = currentB;
			}
		}
	}
		
	Quat rotationVelocity(ly, lx, rx, 0.0f);
	rotationQuat += 2.5f * rotationVelocity * rotationQuat;
	rotationQuat = normalize(rotationQuat);
}

void buttonCtrl(SceCtrlData ctrl)
{
	if(!turning)
	{
		switch(ctrl.buttons)
		{
			case SCE_CTRL_SQUARE:
				turnY(true, 1);
				break;

			case SCE_CTRL_CROSS:
				turnY(false, 1);
				break;

			case SCE_CTRL_TRIANGLE:
				turnX(false, 1);
				break;

			case SCE_CTRL_CIRCLE:
				turnX(true, 1);
				break;

			case SCE_CTRL_L:
				turnZ(true, 1);
				break;

			case SCE_CTRL_R:
				turnZ(false, 1);
				break;
			default:
				break;
		}
	}
}

void raycast(SceTouchReport rep)
{
	
	Matrix4 invRes = inverse(resultM);
	//calculate screencoordinates a,b in -1/1
	float a = ((rep.x/1919.f) * 2.0f - 1.f) ;
	float b = -((rep.y/1087.f) * 2.0f - 1.f);


		//calculate two points that 
	Vector4 point0 = invRes * Vector4(a , b, 0.9f, 1.0f);
	Vector4 point1 = invRes * Vector4(a , b, 0.1f, 1.0f);
	
	Vector3 p0 = Vector3(point0.getX(), point0.getY(), point0.getZ())/point0.getW();
	Vector3 p1 = Vector3(point1.getX(), point1.getY(), point1.getZ())/point1.getW();

	Vector3 d = normalize(p1 - p0);
	Vector3 o, n, u, v;
	
	touched_Face = -1;

	for(int i = 0; i < 6; i++)
		{	
			currentHitPoint[0] = 0.0f;
			currentHitPoint[1] = 0.0f;
			//get local origin, normal, up(u) and right(v)
			o = Vector3(faceData[i].oVec[0], faceData[i].oVec[1], faceData[i].oVec[2]);
			n = Vector3(faceData[i].nVec[0], faceData[i].nVec[1], faceData[i].nVec[2]);
			u = Vector3(faceData[i].uVec[0], faceData[i].uVec[1], faceData[i].uVec[2]);
			v = Vector3(faceData[i].vVec[0], faceData[i].vVec[1], faceData[i].vVec[2]);

			if(dot(d,n) > 0)
			{	
				float alpha = dot(o - p0, n) / dot(d,n);
				Vector3 s = p0 + alpha * d;  

				float y = dot(u, (s - o));
				float x = dot(v, (s - o));

				currentHitPoint[0] = x;
				currentHitPoint[1] = y;

				if(rep.id != f_firstID)
				{
					f_firstID = rep.id;
					
					firstHitPoint[0] = x;
					firstHitPoint[1] = y;
				}
			}
	}
}

void raycast2(SceTouchReport rep)
{
	Matrix4 invRes = inverse(viewProj);
	//calculate screencoordinates a,b in -1/1
	float a = (rep.x/1919.f) * 2.0f - 1.f ;
	float b = -((rep.y/1087.f) * 2.0f - 1.f);
	
	
	touched_Face = -1;
	touched_Tile = -1;
	
	//calculate two points that 
	Vector4 point0 = invRes * Vector4(a , b, 0.9f, 1.0f);
	Vector4 point1 = invRes * Vector4(a , b, 0.1f, 1.0f);
	
	Vector3 p0 = Vector3(point0.getX(), point0.getY(), point0.getZ())/point0.getW();
	Vector3 p1 = Vector3(point1.getX(), point1.getY(), point1.getZ())/point1.getW();

	Vector3 d = normalize(p1 - p0);
	Vector3 o, n, u, v;

	for(int i = 0; i<6; i++)
	{
		
		//get local origin, normal, up(u) and right(v)
		Vector4 _o = Vector4(faceData[i].oVec[0], faceData[i].oVec[1], faceData[i].oVec[2], 1.0f);
		Vector4 _n = Vector4(faceData[i].nVec[0], faceData[i].nVec[1], faceData[i].nVec[2], 0.0f);
		Vector4 _u = Vector4(faceData[i].uVec[0], faceData[i].uVec[1], faceData[i].uVec[2], 0.0f);
		Vector4 _v = Vector4(faceData[i].vVec[0], faceData[i].vVec[1], faceData[i].vVec[2], 0.0f);

		//rotate into worldSpace
		_o = rotationM * _o;
		_n = rotationM * _n;
		_u = rotationM * _u;
		_v = rotationM * _v;

		o = _o.getXYZ() / _o.getW();
		n = normalize(_n.getXYZ() / _o.getW());
		u = normalize(_u.getXYZ() / _o.getW());
		v = normalize(_v.getXYZ() / _o.getW());

		if(dot(d,n) > 0)
		{	
			float alpha = dot(o - p0, n) / dot(d,n);
			Vector3 s = p0 + alpha * d;  

			float y = dot(u, (s - o));
			float x = dot(v, (s - o));

			if(x <= 0.5 && x >= -0.5 && y <= 0.5 && y >= -0.5)
			{	
				currentHitPoint[0] = x;
				currentHitPoint[1] = y;
			
				touched_Face = i;

				if(x < tileSize/2)
				{
					if(y > tileSize/2)
						touched_Tile = 0;
					else if(y < tileSize/2)
						touched_Tile = 6;
				}
				else if(x > tileSize/2)
				{
					if(y > tileSize/2)
						touched_Tile = 2;
					else if(y < tileSize/2)
						touched_Tile = 8;
				}
			}

			if(touched_Tile != -1)
			{
				f_firstID = rep.id;
				
				firstHitPoint[0] = x;
				firstHitPoint[1] = y;

				f_firstA = a;
				f_firstB = b;
			}
			//sprintf(resultline, "OnFaceX: %f, OnFaceY: %f", x, y);
		}
	}
}


void turnX(bool left, int direction)
{
	if(left)
	{
		for(int i = 0; i < 21; i++)
		{
			faceVertices[yellowIndices[i]+0].faceTurnID = 1;
			faceVertices[yellowIndices[i]+1].faceTurnID = 1;
			faceVertices[yellowIndices[i]+2].faceTurnID = 1;
			faceVertices[yellowIndices[i]+3].faceTurnID = 1;
		}
		turnMode = 5;
	}
	else
	{
		for(int i = 0; i < 21; i++)
		{
			faceVertices[whiteIndices[i]+0].faceTurnID = 1;
			faceVertices[whiteIndices[i]+1].faceTurnID = 1;
			faceVertices[whiteIndices[i]+2].faceTurnID = 1;
			faceVertices[whiteIndices[i]+3].faceTurnID = 1;
		}
		turnMode = 4;	
	}
	
	turning = true;
	t = 0.0f;
}

void turnY(bool top, int direction)
{
	if(top)
	{
		for(int i = 0; i < 21; i++)
		{
			faceVertices[blueIndices[i]+0].faceTurnID = 0;
			faceVertices[blueIndices[i]+1].faceTurnID = 0;
			faceVertices[blueIndices[i]+2].faceTurnID = 0;
			faceVertices[blueIndices[i]+3].faceTurnID = 0;
		}
		turnMode = 2;
	}

	else
	{
		for(int i = 0; i < 21; i++)
		{
			faceVertices[greenIndices[i]+0].faceTurnID = 0;
			faceVertices[greenIndices[i]+1].faceTurnID = 0;
			faceVertices[greenIndices[i]+2].faceTurnID = 0;
			faceVertices[greenIndices[i]+3].faceTurnID = 0;
		}
		turnMode = 0;	
	}
	
	turning = true;
	t = 0.0f;
}

void turnZ(bool inner, int direction)
{
	if(inner)
	{
		for(int i = 0; i < 21; i++)
		{
			faceVertices[redIndices[i]+0].faceTurnID = 2;
			faceVertices[redIndices[i]+1].faceTurnID = 2;
			faceVertices[redIndices[i]+2].faceTurnID = 2;
			faceVertices[redIndices[i]+3].faceTurnID = 2;
		}
		turnMode = 1;		
	}

	else
	{

		for(int i = 0; i < 21; i++)
		{
			faceVertices[orangeIndices[i]+0].faceTurnID = 2;
			faceVertices[orangeIndices[i]+1].faceTurnID = 2;
			faceVertices[orangeIndices[i]+2].faceTurnID = 2;
			faceVertices[orangeIndices[i]+3].faceTurnID = 2;
		}
		turnMode = 3;	
	}
	
	turning = true;
	t = 0.0f;
}



/* Main render function */
void render( void )
{
	/* render libgxm scenes */
	renderGxm();
}

/* render gxm scenes */
void renderGxm( void )
{
/* -----------------------------------------------------------------
	8. Rendering step

	This sample renders a single scene containing the clear triangle.
	Before any drawing can take place, a scene must be started.
	We render to the back buffer, so it is also important to use a
	sync object to ensure that these rendering operations are
	synchronized with display operations.

	The clear triangle shaders do not declare any uniform variables,
	so this may be rendered immediately after setting the vertex and
	fragment program.

	Once clear triangle have been drawn the scene can be ended, which
	submits it for rendering on the GPU.
	----------------------------------------------------------------- */

	/* start rendering to the render target */
	sceGxmBeginScene( s_context, 0, s_renderTarget, NULL, NULL, s_displayBufferSync[s_displayBackBufferIndex], &s_displaySurface[s_displayBackBufferIndex], &s_depthSurface );

	/* set clear shaders */
	sceGxmSetVertexProgram( s_context, s_clearVertexProgram );
	sceGxmSetFragmentProgram( s_context, s_clearFragmentProgram );

	/* draw ther clear triangle */
	sceGxmSetVertexStream( s_context, 0, s_clearVertices );
	sceGxmDraw( s_context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, s_clearIndices, 3 );

	//TEXTURES
	sceGxmSetFragmentTexture(s_context, 0, &textureMap);
	sceGxmSetFragmentTexture(s_context, 1, &normalMap);

    // rendering the cube.

	sceGxmSetVertexProgram( s_context, s_basicVertexProgram );
	sceGxmSetFragmentProgram( s_context, s_basicFragmentProgram );

	/* set the vertex program constants */
	void *vertexDefaultBuffer;
	sceGxmReserveVertexDefaultUniformBuffer( s_context, &vertexDefaultBuffer );
	sceGxmSetUniformDataF( vertexDefaultBuffer, rotParam, 0, 16, (float*)&rotationM);
	sceGxmSetUniformDataF( vertexDefaultBuffer, s_wvpParam, 0, 16, (float*)&resultM );
	sceGxmSetUniformDataF( vertexDefaultBuffer, rotXParam, 0, 16, (float*)&faceX );
	sceGxmSetUniformDataF( vertexDefaultBuffer, rotYParam, 0, 16, (float*)&faceY );
	sceGxmSetUniformDataF( vertexDefaultBuffer, rotZParam, 0, 16, (float*)&faceZ );;
	
	
	/*set the Vertexstream draw the Cube*/
	sceGxmSetVertexStream( s_context, 0, faceVertices );
	sceGxmDraw( s_context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, faceIndices, 6*9*6);

	/* stop rendering to the render target */
	sceGxmEndScene( s_context, NULL, NULL );
}



/* queue a display swap and cycle our buffers */
void cycleDisplayBuffers( void )
{
/* -----------------------------------------------------------------
	9-a. Flip operation

	Now we have finished submitting rendering work for this frame it
	is time to submit a flip operation.  As part of specifying this
	flip operation we must provide the sync objects for both the old
	buffer and the new buffer.  This is to allow synchronization both
	ways: to not flip until rendering is complete, but also to ensure
	that future rendering to these buffers does not start until the
	flip operation is complete.

	Once we have queued our flip, we manually cycle through our back
	buffers before starting the next frame.
	----------------------------------------------------------------- */

	/* PA heartbeat to notify end of frame */
	sceGxmPadHeartbeat( &s_displaySurface[s_displayBackBufferIndex], s_displayBufferSync[s_displayBackBufferIndex] );

	/* queue the display swap for this frame */
	DisplayData displayData;
	displayData.address = s_displayBufferData[s_displayBackBufferIndex];

	/* front buffer is OLD buffer, back buffer is NEW buffer */
	sceGxmDisplayQueueAddEntry( s_displayBufferSync[s_displayFrontBufferIndex], s_displayBufferSync[s_displayBackBufferIndex], &displayData );

	/* update buffer indices */
	s_displayFrontBufferIndex = s_displayBackBufferIndex;
	s_displayBackBufferIndex = (s_displayBackBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
}


/* Destroy Gxm Data */
void destroyGxmData( void )
{
/* ---------------------------------------------------------------------
	11. Destroy the programs and data for the clear and spinning triangle

	Once the GPU is finished, we release all our programs.
	--------------------------------------------------------------------- */

	/* clean up allocations */
	sceGxmShaderPatcherReleaseFragmentProgram( s_shaderPatcher, s_clearFragmentProgram );
	sceGxmShaderPatcherReleaseVertexProgram( s_shaderPatcher, s_clearVertexProgram );
	graphicsFree( s_clearIndicesUId );
	graphicsFree( s_clearVerticesUId );

	/* wait until display queue is finished before deallocating display buffers */
	sceGxmDisplayQueueFinish();

	/* unregister programs and destroy shader patcher */
	sceGxmShaderPatcherUnregisterProgram( s_shaderPatcher, s_clearFragmentProgramId );
	sceGxmShaderPatcherUnregisterProgram( s_shaderPatcher, s_clearVertexProgramId );
	sceGxmShaderPatcherDestroy( s_shaderPatcher );
	fragmentUsseFree( s_patcherFragmentUsseUId );
	vertexUsseFree( s_patcherVertexUsseUId );
	graphicsFree( s_patcherBufferUId );
}



/* ShutDown libgxm */
int shutdownGxm( void )
{
/* ---------------------------------------------------------------------
	12. Finalize libgxm

	Once the GPU is finished, we deallocate all our memory,
	destroy all object and finally terminate libgxm.
	--------------------------------------------------------------------- */

	int returnCode = SCE_OK;

	graphicsFree( s_depthBufferUId );

	for ( unsigned int i = 0 ; i < DISPLAY_BUFFER_COUNT; ++i )
	{
		memset( s_displayBufferData[i], 0, DISPLAY_HEIGHT*DISPLAY_STRIDE_IN_PIXELS*4 );
		graphicsFree( s_displayBufferUId[i] );
		sceGxmSyncObjectDestroy( s_displayBufferSync[i] );
	}

	/* destroy the render target */
	sceGxmDestroyRenderTarget( s_renderTarget );

	/* destroy the context */
	sceGxmDestroyContext( s_context );

	fragmentUsseFree( s_fragmentUsseRingBufferUId );
	graphicsFree( s_fragmentRingBufferUId );
	graphicsFree( s_vertexRingBufferUId );
	graphicsFree( s_vdmRingBufferUId );
	free( s_contextParams.hostMem );

	/* terminate libgxm */
	sceGxmTerminate();

	return returnCode;
}


/* Host alloc */
static void *patcherHostAlloc( void *userData, unsigned int size )
{
	(void)( userData );

	return malloc( size );
}

/* Host free */
static void patcherHostFree( void *userData, void *mem )
{
	(void)( userData );

	free( mem );
}

/* Display callback */
void displayCallback( const void *callbackData )
{
/* -----------------------------------------------------------------
	10-b. Flip operation

	The callback function will be called from an internal thread once
	queued GPU operations involving the sync objects is complete.
	Assuming we have not reached our maximum number of queued frames,
	this function returns immediately.
	----------------------------------------------------------------- */

	SceDisplayFrameBuf framebuf;

	/* cast the parameters back */
	const DisplayData *displayData = (const DisplayData *)callbackData;


    // Render debug text.
    /* set framebuffer info */
	SceDbgFontFrameBufInfo info;
	memset( &info, 0, sizeof(SceDbgFontFrameBufInfo) );
	info.frameBufAddr = (SceUChar8 *)displayData->address;
	info.frameBufPitch = DISPLAY_STRIDE_IN_PIXELS;
	info.frameBufWidth = DISPLAY_WIDTH;
	info.frameBufHeight = DISPLAY_HEIGHT;
	info.frameBufPixelformat = DBGFONT_PIXEL_FORMAT;

	/* flush font buffer */
	int returnCode = sceDbgFontFlush( &info );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );
	

	/* wwap to the new buffer on the next VSYNC */
	memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
	framebuf.size        = sizeof(SceDisplayFrameBuf);
	framebuf.base        = displayData->address;
	framebuf.pitch       = DISPLAY_STRIDE_IN_PIXELS;
	framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
	framebuf.width       = DISPLAY_WIDTH;
	framebuf.height      = DISPLAY_HEIGHT;
	returnCode = sceDisplaySetFrameBuf( &framebuf, SCE_DISPLAY_UPDATETIMING_NEXTVSYNC );
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );

	/* block this callback until the swap has occurred and the old buffer is no longer displayed */
	returnCode = sceDisplayWaitVblankStart();
	SCE_DBG_ALWAYS_ASSERT( returnCode == SCE_OK );
}

/* Alloc used by libgxm */
static void *graphicsAlloc( SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid )
{
/*	Since we are using sceKernelAllocMemBlock directly, we cannot directly
	use the alignment parameter.  Instead, we must allocate the size to the
	minimum for this memblock type, and just SCE_DBG_ALWAYS_ASSERT that this will cover
	our desired alignment.

	Developers using their own heaps should be able to use the alignment
	parameter directly for more minimal padding.
*/

	if( type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA )
	{
		/* CDRAM memblocks must be 256KiB aligned */
		SCE_DBG_ALWAYS_ASSERT( alignment <= 256*1024 );
		size = ALIGN( size, 256*1024 );
	}
	else
	{
		/* LPDDR memblocks must be 4KiB aligned */
		SCE_DBG_ALWAYS_ASSERT( alignment <= 4*1024 );
		size = ALIGN( size, 4*1024 );
	}

	/* allocate some memory */
	*uid = sceKernelAllocMemBlock( "simple", type, size, NULL );
	SCE_DBG_ALWAYS_ASSERT( *uid >= SCE_OK );

	/* grab the base address */
	void *mem = NULL;
	int err = sceKernelGetMemBlockBase( *uid, &mem );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	/* map for the GPU */
	err = sceGxmMapMemory( mem, size, attribs );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	/* done */
	return mem;
}

/* Free used by libgxm */
static void graphicsFree( SceUID uid )
{
	/* grab the base address */
	void *mem = NULL;
	int err = sceKernelGetMemBlockBase(uid, &mem);
	SCE_DBG_ALWAYS_ASSERT(err == SCE_OK);

	// unmap memory
	err = sceGxmUnmapMemory(mem);
	SCE_DBG_ALWAYS_ASSERT(err == SCE_OK);

	// free the memory block
	err = sceKernelFreeMemBlock(uid);
	SCE_DBG_ALWAYS_ASSERT(err == SCE_OK);
}

/* vertex alloc used by libgxm */
static void *vertexUsseAlloc( uint32_t size, SceUID *uid, uint32_t *usseOffset )
{
	/* align to memblock alignment for LPDDR */
	size = ALIGN( size, 4096 );

	/* allocate some memory */
	*uid = sceKernelAllocMemBlock( "simple", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, NULL );
	SCE_DBG_ALWAYS_ASSERT( *uid >= SCE_OK );

	/* grab the base address */
	void *mem = NULL;
	int err = sceKernelGetMemBlockBase( *uid, &mem );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	/* map as vertex USSE code for the GPU */
	err = sceGxmMapVertexUsseMemory( mem, size, usseOffset );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	return mem;
}

/* vertex free used by libgxm */
static void vertexUsseFree( SceUID uid )
{
	/* grab the base address */
	void *mem = NULL;
	int err = sceKernelGetMemBlockBase( uid, &mem );
	SCE_DBG_ALWAYS_ASSERT(err == SCE_OK);

	/* unmap memory */
	err = sceGxmUnmapVertexUsseMemory( mem );
	SCE_DBG_ALWAYS_ASSERT(err == SCE_OK);

	/* free the memory block */
	err = sceKernelFreeMemBlock( uid );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );
}

/* fragment alloc used by libgxm */
static void *fragmentUsseAlloc( uint32_t size, SceUID *uid, uint32_t *usseOffset )
{
	/* align to memblock alignment for LPDDR */
	size = ALIGN( size, 4096 );

	/* allocate some memory */
	*uid = sceKernelAllocMemBlock( "simple", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, NULL );
	SCE_DBG_ALWAYS_ASSERT( *uid >= SCE_OK );

	/* grab the base address */
	void *mem = NULL;
	int err = sceKernelGetMemBlockBase( *uid, &mem );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	/* map as fragment USSE code for the GPU */
	err = sceGxmMapFragmentUsseMemory( mem, size, usseOffset);
	SCE_DBG_ALWAYS_ASSERT(err == SCE_OK);

	// done
	return mem;
}

/* fragment free used by libgxm */
static void fragmentUsseFree( SceUID uid )
{
	/* grab the base address */
	void *mem = NULL;
	int err = sceKernelGetMemBlockBase( uid, &mem );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	/* unmap memory */
	err = sceGxmUnmapFragmentUsseMemory( mem );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );

	/* free the memory block */
	err = sceKernelFreeMemBlock( uid );
	SCE_DBG_ALWAYS_ASSERT( err == SCE_OK );
}

