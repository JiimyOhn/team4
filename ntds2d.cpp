//---------------------------------------------------------------------------
#define _USE_MATH_DEFINES
#include <math.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glext.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "AircraftDB.h"
#include <stdio.h>
#include <png.h>

#pragma hdrstop

#include "ntds2d.h"
#include "hex_font.h"
#include "Aircraft.h"
#include "AircraftApi.h"
#include "DisplayGUI.h"
#define RADPERDEG (asin(1.0f)/90.0f)
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
//---------------------------------------------------------------------------

#pragma package(smart_init)

#define PI 3.1415926535898
#define RADIUS 20.0
#define NUM_SPRITES 81
#define SPRITE_WIDTH   72
#define SPRITE_HEIGHT  72
#define SPRITE_CHANNELS 4

static GLuint AIR_TRACK_FRIEND;
static GLuint AIR_TRACK_HOSTILE;
static GLuint AIR_TRACK_UNKNOWN;
static GLuint SURFACE_TRACK_FRIEND ;
static GLuint TRACKHOOK;

static GLuint TextureSpites[NUM_SPRITES];
static GLuint TextureSpriteArray = 0;
static int NumSprites=0;

// Airport marker atlas data
const int iconW = 48, iconH = 48;
const int atlasW = 144, atlasH = 48;
AtlasRect airportAtlasRects[3] = {
    {0,   0, iconW, iconH},
    {48,  0, iconW, iconH},
    {96,  0, iconW, iconH}
};
GLuint atlasTexId = 0;

static const size_t MAX_AIRPORT_INSTANCES = 40000;

//-----------------------------------------------------------------------------
AirportType GetAirportType(const AirportInfo& airport)
{
    return Civil;
}
//-----------------------------------------------------------------------------

struct AirportInstancingResources {
    bool initialized;
    GLuint vao;
    GLuint quadVBO;
    GLuint instanceVBO;
    GLuint program;
    std::vector<AirportInstance> instances;
};

static AirportInstancingResources gAirportInst = {false,0,0,0,0};

struct AirPlaneInstancingResources {
    bool initialized;
    GLuint vao;
    GLuint quadVBO;
    GLuint instanceVBO;
    GLuint program;
};

struct LineInstancingResources {
    bool initialized;
    GLuint vao;
    GLuint vertexVBO;
    GLuint instanceVBO;
    GLuint program;
};

static AirPlaneInstancingResources gInstancing = {false,0,0,0,0};
static LineInstancingResources gLineInstancing = {false,0,0,0,0};

struct HexTextResources {
    bool initialized;
    GLuint vao;
    GLuint quadVBO;
    GLuint instanceVBO;
    GLuint texture;
    GLuint program;
    float  scale;
    int    bold;
};

static HexTextResources gHexText = {false,0,0,0,0,0,1.0f,0};

// OpenGL extension function pointers for systems with only GL 1.1 headers
static bool gExtensionsLoaded = false;
static PFNGLCREATESHADERPROC           pglCreateShader           = nullptr;
static PFNGLSHADERSOURCEPROC           pglShaderSource           = nullptr;
static PFNGLCOMPILESHADERPROC          pglCompileShader          = nullptr;
static PFNGLCREATEPROGRAMPROC          pglCreateProgram          = nullptr;
static PFNGLATTACHSHADERPROC           pglAttachShader           = nullptr;
static PFNGLLINKPROGRAMPROC            pglLinkProgram            = nullptr;
static PFNGLDELETESHADERPROC           pglDeleteShader           = nullptr;
static PFNGLUSEPROGRAMPROC             pglUseProgram             = nullptr;
static PFNGLACTIVETEXTUREPROC          pglActiveTexture          = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC     pglGetUniformLocation     = nullptr;
static PFNGLUNIFORM1IPROC              pglUniform1i              = nullptr;
static PFNGLUNIFORM2FPROC              pglUniform2f              = nullptr;
static PFNGLUNIFORM1FPROC              pglUniform1f              = nullptr;
static PFNGLGENVERTEXARRAYSPROC        pglGenVertexArrays        = nullptr;
static PFNGLBINDVERTEXARRAYPROC        pglBindVertexArray        = nullptr;
static PFNGLGENBUFFERSPROC             pglGenBuffers             = nullptr;
static PFNGLBINDBUFFERPROC             pglBindBuffer             = nullptr;
static PFNGLBUFFERDATAPROC             pglBufferData             = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC    pglVertexAttribPointer    = nullptr;
static PFNGLVERTEXATTRIBDIVISORPROC    pglVertexAttribDivisor    = nullptr;
static PFNGLVERTEXATTRIBIPOINTERPROC   pglVertexAttribIPointer   = nullptr;
static PFNGLDRAWARRAYSINSTANCEDPROC    pglDrawArraysInstanced    = nullptr;
static PFNGLTEXIMAGE3DPROC             pglTexImage3D             = nullptr;
static PFNGLTEXSUBIMAGE3DPROC          pglTexSubImage3D          = nullptr;

extern ght_hash_table_t *AircraftDBHashTable;

static void *GetAnyGLFuncAddress(const char *name)
{
#ifdef _WIN32
    void *p = (void*)wglGetProcAddress(name);
    if(!p)
    {
        static HMODULE ogl = GetModuleHandleA("opengl32.dll");
        if(ogl) p = (void*)GetProcAddress(ogl, name);
    }
    return p;
#else
    return (void*)glXGetProcAddressARB((const GLubyte*)name);
#endif
}

static void LoadGLExtensions()
{
    if (gExtensionsLoaded) return;
#define LOAD_PROC(type, name) name = (type)GetAnyGLFuncAddress(#name + 1);

    LOAD_PROC(PFNGLTEXIMAGE3DPROC, pglTexImage3D);
    LOAD_PROC(PFNGLTEXSUBIMAGE3DPROC, pglTexSubImage3D);
    LOAD_PROC(PFNGLCREATESHADERPROC, pglCreateShader);
    LOAD_PROC(PFNGLSHADERSOURCEPROC, pglShaderSource);
    LOAD_PROC(PFNGLCOMPILESHADERPROC, pglCompileShader);
    LOAD_PROC(PFNGLCREATEPROGRAMPROC, pglCreateProgram);
    LOAD_PROC(PFNGLATTACHSHADERPROC, pglAttachShader);
    LOAD_PROC(PFNGLLINKPROGRAMPROC, pglLinkProgram);
    LOAD_PROC(PFNGLDELETESHADERPROC, pglDeleteShader);
    LOAD_PROC(PFNGLUSEPROGRAMPROC, pglUseProgram);
    LOAD_PROC(PFNGLACTIVETEXTUREPROC, pglActiveTexture);
    LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, pglGetUniformLocation);
    LOAD_PROC(PFNGLUNIFORM1IPROC, pglUniform1i);
    LOAD_PROC(PFNGLUNIFORM2FPROC, pglUniform2f);
    LOAD_PROC(PFNGLUNIFORM1FPROC, pglUniform1f);
    LOAD_PROC(PFNGLGENVERTEXARRAYSPROC, pglGenVertexArrays);
    LOAD_PROC(PFNGLBINDVERTEXARRAYPROC, pglBindVertexArray);
    LOAD_PROC(PFNGLGENBUFFERSPROC, pglGenBuffers);
    LOAD_PROC(PFNGLBINDBUFFERPROC, pglBindBuffer);
    LOAD_PROC(PFNGLBUFFERDATAPROC, pglBufferData);
    LOAD_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, pglEnableVertexAttribArray);
    LOAD_PROC(PFNGLVERTEXATTRIBPOINTERPROC, pglVertexAttribPointer);
    LOAD_PROC(PFNGLVERTEXATTRIBDIVISORPROC, pglVertexAttribDivisor);
    LOAD_PROC(PFNGLVERTEXATTRIBIPOINTERPROC, pglVertexAttribIPointer);
    LOAD_PROC(PFNGLDRAWARRAYSINSTANCEDPROC, pglDrawArraysInstanced);
    #undef LOAD_PROC
    gExtensionsLoaded = true;
}


//---------------------------------------------------------------------------
int MakeAirplaneImages(void)
{
    LoadGLExtensions();
	bool hasAlpha=false;
	const char filename[] = "..\\..\\Symbols\\sprites-RGBA.png";

	int width = 0;
	int height = 0;
	int nrChannels=0;
	GLubyte *SpriteImage;
	GLubyte SpriteTexture[SPRITE_WIDTH][SPRITE_HEIGHT][SPRITE_CHANNELS];
	int x,y;

	NumSprites=0;
    //stbi_set_flip_vertically_on_load(true);
	SpriteImage= stbi_load(filename, &width, &height,&nrChannels,0);
	if (SpriteImage == NULL)
	{
		ShowMessage("Unable to load png file");
		return(0);
	}

	//ShowMessage("Image loaded "+IntToStr(width)+" "+IntToStr(height) + " channnels " +IntToStr(nrChannels));
	if (nrChannels==4) {
     hasAlpha=true;
	}
        glGenTextures(NUM_SPRITES, TextureSpites);
        glGenTextures(1, &TextureSpriteArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, TextureSpriteArray);
        pglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, hasAlpha ? GL_RGBA : GL_RGB,
                     SPRITE_WIDTH, SPRITE_HEIGHT, NUM_SPRITES, 0,
                     hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	for (int row = 0; row < 11; row++)
	{
	 for (int col = 0; col < 8; col++)
      {
		for (int x = 0; x < SPRITE_WIDTH; ++x)
         {
		  for (int y = 0; y < SPRITE_HEIGHT; ++y)
           {
			int index = (((y+row*SPRITE_HEIGHT)* width + x+(col*SPRITE_WIDTH))) * SPRITE_CHANNELS;
            SpriteTexture[x][y][0]= SpriteImage[index];
            SpriteTexture[x][y][1]= SpriteImage[index+1];
            SpriteTexture[x][y][2]= SpriteImage[index+2];
            SpriteTexture[x][y][3]= SpriteImage[index+3];
           }
         }

        glBindTexture(GL_TEXTURE_2D, TextureSpites[NumSprites]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, hasAlpha ? 4 : 3, SPRITE_WIDTH,
                                   SPRITE_HEIGHT, 0, hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
                                   SpriteTexture);

        glBindTexture(GL_TEXTURE_2D_ARRAY, TextureSpriteArray);
        pglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, NumSprites,
                        SPRITE_WIDTH, SPRITE_HEIGHT, 1,
                        hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
                        SpriteTexture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBindTexture(GL_TEXTURE_2D, 0);
	  NumSprites++;
	  if (NumSprites==NUM_SPRITES)
		{
		  stbi_image_free(SpriteImage);
		  glEnable(GL_TEXTURE_2D);
		  glShadeModel(GL_FLAT);
		  return(NumSprites);
		}
    }
  }
  stbi_image_free(SpriteImage);
  glEnable(GL_TEXTURE_2D);
  glShadeModel(GL_FLAT);
  return(NumSprites);
}

//---------------------------------------------------------------------------
int MakeAirportImages(void)
{
    LoadGLExtensions();
    if(atlasTexId != 0) return 1;

    const char* filename = "../../Symbols/airport_atlas.png";
    FILE* fp = fopen(filename, "rb");
    if(!fp){
        printf("[ERROR] failed to open airport atlas: %s\n", filename);
        return 0;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if(setjmp(png_jmpbuf(png))){
        printf("[ERROR] png parse error\n");
        fclose(fp);
        return 0;
    }
    png_init_io(png, fp);
    png_read_info(png, info);

    int width  = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    int color  = png_get_color_type(png, info);
    int depth  = png_get_bit_depth(png, info);

    if(depth == 16) png_set_strip_16(png);
    if(color == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if(color == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand(png);
    if(png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if(color == PNG_COLOR_TYPE_RGB || color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if(color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    int rowbytes = png_get_rowbytes(png, info);
    std::vector<png_byte> img(rowbytes * height);
    std::vector<png_bytep> rows(height);
    for(int y=0;y<height;y++)
        rows[y] = img.data() + y*rowbytes;
    png_read_image(png, rows.data());
    fclose(fp);

    for(int y=0;y<height;++y){
        for(int x=0;x<width;++x){
            png_bytep p = &img[y*rowbytes + x*4];
            if(p[0]>240 && p[1]>240 && p[2]>240)
                p[3] = 0;
        }
    }

    glGenTextures(1, &atlasTexId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlasTexId);
    pglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
                  iconW, iconH, 3, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<unsigned char> layer(iconW * iconH * 4);
    for(int i=0;i<3;i++){
        for(int y=0;y<iconH;y++){
            memcpy(layer.data()+y*iconW*4,
                   &img[y*rowbytes + i*iconW*4],
                   iconW*4);
        }
        pglTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0,0,i,
                        iconW, iconH, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        layer.data());
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    png_destroy_read_struct(&png, &info, NULL);
    gAirportInst.instances.reserve(MAX_AIRPORT_INSTANCES);
    return 1;
}
//---------------------------------------------------------------------------
void MakeAirTrackFriend(void)
{
 GLuint i;
 GLfloat cosine, sine;
 AIR_TRACK_FRIEND=glGenLists(1);
 glNewList(AIR_TRACK_FRIEND, GL_COMPILE);
 glPointSize(3.0);
 glLineWidth(2.0);
#if 1
  glEnable( GL_LINE_SMOOTH );
  glEnable( GL_POINT_SMOOTH );
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

 glBegin(GL_LINE_STRIP);
 for(i = 0; i <= 50; i++)
  {
   cosine = cos(i *  PI / 50.0)*RADIUS;
   sine = sin(i * PI/50.0)*RADIUS;
   glVertex2f(cosine, sine);
  }
 glEnd();

 glBegin(GL_POINTS);
 glVertex2f(0.0, 0.0);
 glEnd();
 glEndList();
}
//---------------------------------------------------------------------------
void MakeAirTrackHostile(void)
{
 AIR_TRACK_HOSTILE=glGenLists(1);
 glNewList(AIR_TRACK_HOSTILE, GL_COMPILE);
 glPointSize(3.0);
 glLineWidth(2.0);

 glBegin(GL_LINE_STRIP);
 glVertex2f(-10.0, 0.0);
 glVertex2f(0.0, 10.0);
 glVertex2f(10.0, 0.0);
 glEnd();

 glBegin(GL_POINTS);
 glVertex2f(0.0, 0.0);
 glEnd();
 glEndList();
}
//---------------------------------------------------------------------------
void MakeAirTrackUnknown(void)
{
 AIR_TRACK_UNKNOWN=glGenLists(1);
 glNewList(AIR_TRACK_UNKNOWN, GL_COMPILE);
 glPointSize(3.0);
 glLineWidth(2.0);

 glBegin(GL_LINE_STRIP);
 glVertex2f(-10.0, 0.0);
 glVertex2f(-10.0,10.0);
 glVertex2f(10.0, 10.0);
 glVertex2f(10.0, 0.0);
 glEnd();

 glBegin(GL_POINTS);
 glVertex2f(0.0, 0.0);
 glEnd();
 glEndList();
}
//---------------------------------------------------------------------------
void MakePoint(void)
{
 GLuint i;
 GLfloat cosine, sine;
 SURFACE_TRACK_FRIEND=glGenLists(1);
 glNewList(SURFACE_TRACK_FRIEND, GL_COMPILE);
 glPointSize(3.0);
 glLineWidth(3.0);
#if 1
 glEnable( GL_LINE_SMOOTH );
 glEnable( GL_POINT_SMOOTH );
 glEnable (GL_BLEND);
 glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

 glBegin(GL_LINE_STRIP);
 for(i = 0; i < 100; i++)
  {
   cosine = cos(i *2* PI / 100.0)*RADIUS;
   sine = sin(i * 2* PI/100.0)*RADIUS;
   glVertex2f(cosine, sine);
  }
 glEnd();

 glBegin(GL_LINE_STRIP);
 for(i = 0; i < 100; i++)
  {
   cosine = cos(i *2* PI / 100.0)*2.0;
   sine = sin(i * 2* PI/100.0)*2.0;
   glVertex2f(cosine, sine);
  }
 glEnd();

 glEndList();

}
//---------------------------------------------------------------------------
void MakeTrackHook(void)
{
 GLuint i;
 GLfloat cosine, sine;
 TRACKHOOK=glGenLists(1);
 glNewList(TRACKHOOK, GL_COMPILE);
 glPointSize(8.0);
 glLineWidth(10.0);
#if 1
 glEnable( GL_LINE_SMOOTH );
 glEnable( GL_POINT_SMOOTH );
 glEnable (GL_BLEND);
 glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

 glBegin(GL_LINE_STRIP);
 for(i = 0; i < 100; i++)
  {
   cosine = cos(i *2* PI / 100.0)*RADIUS*3;
   sine = sin(i * 2* PI/100.0)*RADIUS*3;
   glVertex2f(cosine, sine);
  }
 glEnd();
 glEndList();
}
 //---------------------------------------------------------------------------
void DrawAirplaneImage(float x, float y,float scale,float heading,int imageNum)
{
   glPushMatrix();
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, TextureSpites[imageNum]);
   glShadeModel(GL_FLAT);
   glTranslated(x,y,0.0);
   glRotatef(-heading-90.0, 0,0,1);
   glBegin(GL_QUADS);

   glTexCoord2f(1.0, 1.0);
   glVertex2f(36.0*scale,36.0*scale);      // top right

   glTexCoord2f(0.0, 1.0);
   glVertex2f(-36.0*scale, 36.0*scale);    // top left

   glTexCoord2f(0.0, 0.0);
   glVertex2f(-36.0*scale, -36.0*scale);  // bottom left

   glTexCoord2f(1.0, 0.0);
   glVertex2f(36.0*scale, -36.0*scale);    // bottom right

   glEnd();
   glBindTexture(GL_TEXTURE_2D, 0);
   glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

static GLuint CompileShader(GLenum type, const char* src)
{
    GLuint sh = pglCreateShader(type);
    pglShaderSource(sh, 1, &src, NULL);
    pglCompileShader(sh);
    return sh;
}

static GLuint CreateProgram(const char* vs, const char* fs)
{
    GLuint v = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = pglCreateProgram();
    pglAttachShader(p, v);
    pglAttachShader(p, f);
    pglLinkProgram(p);
    pglDeleteShader(v);
    pglDeleteShader(f);
    return p;
}

void InitAirplaneInstancing()
{
    LoadGLExtensions();
    if(gInstancing.initialized) return;

    const char* vsSrc =
        "#version 330 core\n"
        "layout(location=0) in vec2 vert;\n"
        "layout(location=1) in vec2 uv;\n"
        "layout(location=2) in vec2 pos;\n"
        "layout(location=3) in float scale;\n"
        "layout(location=4) in float heading;\n"
        "layout(location=5) in int image;\n"
        "layout(location=6) in vec4 color;\n"
        "uniform vec2 Viewport;\n"
        "out vec2 Tex;\n"
        "out vec4 Color;\n"
        "flat out int Image;\n"
        "void main(){\n"
        "  float rad = radians(-heading - 90.0);\n"
        "  mat2 R = mat2(cos(rad), sin(rad), -sin(rad), cos(rad));\n"
        "  vec2 p = pos + R * (vert * scale * 36.0);\n"
        "  vec2 ndc = vec2((p.x / Viewport.x) * 2.0 - 1.0, (p.y / Viewport.y) * 2.0 - 1.0);\n"
        "  gl_Position = vec4(ndc, 0.0, 1.0);\n"
        "  Tex = uv;\n"
        "  Color = color;\n"
        "  Image = image;\n"
        "}\n";

    const char* fsSrc =
        "#version 330 core\n"
        "in vec2 Tex;\n"
        "in vec4 Color;\n"
        "flat in int Image;\n"
        "out vec4 Frag;\n"
        "uniform sampler2DArray spriteTex;\n"
        "void main(){\n"
        "  Frag = texture(spriteTex, vec3(Tex, Image)) * Color;\n"
        "}\n";

    gInstancing.program = CreateProgram(vsSrc, fsSrc);

    float quad[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
       -1.0f, 1.0f, 0.0f, 1.0f,
       -1.0f,-1.0f, 0.0f, 0.0f,
        1.0f,-1.0f, 1.0f, 0.0f
    };

    pglGenVertexArrays(1, &gInstancing.vao);
    pglBindVertexArray(gInstancing.vao);

    pglGenBuffers(1, &gInstancing.quadVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gInstancing.quadVBO);
    pglBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    pglEnableVertexAttribArray(1);
    pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));

    pglGenBuffers(1, &gInstancing.instanceVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gInstancing.instanceVBO);
    pglBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    size_t stride = sizeof(AirplaneInstance);
    pglEnableVertexAttribArray(2);
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(AirplaneInstance, x));
    pglVertexAttribDivisor(2,1);
    pglEnableVertexAttribArray(3);
    pglVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(AirplaneInstance, scale));
    pglVertexAttribDivisor(3,1);
    pglEnableVertexAttribArray(4);
    pglVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(AirplaneInstance, heading));
    pglVertexAttribDivisor(4,1);
    pglEnableVertexAttribArray(5);
    pglVertexAttribIPointer(5, 1, GL_INT, stride, (void*)offsetof(AirplaneInstance, imageNum));
    pglVertexAttribDivisor(5,1);
    pglEnableVertexAttribArray(6);
    pglVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(AirplaneInstance, color));
    pglVertexAttribDivisor(6,1);

    pglBindBuffer(GL_ARRAY_BUFFER, 0);
    pglBindVertexArray(0);

    gInstancing.initialized = true;
}

void InitAirplaneLinesInstancing()
{
    LoadGLExtensions();
    if(gLineInstancing.initialized) return;
    const char* vsSrc =
        "#version 330 core\n"
        "layout(location=0) in float t;\n"
        "layout(location=1) in vec2 start;\n"
        "layout(location=2) in vec2 end;\n"
        "uniform vec2 Viewport;\n"
        "void main(){\n"
        "  vec2 p = mix(start, end, t);\n"
        "  vec2 ndc = vec2((p.x/Viewport.x)*2.0 - 1.0, (p.y/Viewport.y)*2.0 - 1.0);\n"
        "  gl_Position = vec4(ndc,0.0,1.0);\n"
        "}\n";

    const char* fsSrc =
        "#version 330 core\n"
        "out vec4 Frag;\n"
        "void main(){ Frag = vec4(1.0,1.0,0.0,1.0); }\n";

    gLineInstancing.program = CreateProgram(vsSrc, fsSrc);

    float verts[] = {0.0f, 1.0f};
    pglGenVertexArrays(1, &gLineInstancing.vao);
    pglBindVertexArray(gLineInstancing.vao);

    pglGenBuffers(1, &gLineInstancing.vertexVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gLineInstancing.vertexVBO);
    pglBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);

    pglGenBuffers(1, &gLineInstancing.instanceVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gLineInstancing.instanceVBO);
    pglBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    size_t stride = sizeof(AirplaneLineInstance);
    pglEnableVertexAttribArray(1);
    pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(AirplaneLineInstance, x1));
    pglVertexAttribDivisor(1,1);
    pglEnableVertexAttribArray(2);
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(AirplaneLineInstance, x2));
    pglVertexAttribDivisor(2,1);

    pglBindBuffer(GL_ARRAY_BUFFER, 0);
    pglBindVertexArray(0);

    gLineInstancing.initialized = true;
}

void DrawAirplaneImagesInstanced(const std::vector<AirplaneInstance>& instances)
{
    if(instances.empty()) return;
    if(!gInstancing.initialized)
        InitAirplaneInstancing();

    pglBindVertexArray(gInstancing.vao);
    pglBindBuffer(GL_ARRAY_BUFFER, gInstancing.instanceVBO);
    // TODO : 재할당 제거?
    pglBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(AirplaneInstance), instances.data(), GL_STREAM_DRAW);

    pglUseProgram(gInstancing.program);
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    pglUniform2f(pglGetUniformLocation(gInstancing.program, "Viewport"), (float)vp[2], (float)vp[3]);
    pglActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, TextureSpriteArray);
    pglUniform1i(pglGetUniformLocation(gInstancing.program, "spriteTex"), 0);

    pglDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, instances.size());

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    pglBindVertexArray(0);
    pglUseProgram(0);
}

void DrawAirplaneLinesInstanced(const std::vector<AirplaneLineInstance>& instances)
{
    if(instances.empty()) return;
    if(!gLineInstancing.initialized)
        InitAirplaneLinesInstancing();

    pglBindVertexArray(gLineInstancing.vao);
    pglBindBuffer(GL_ARRAY_BUFFER, gLineInstancing.instanceVBO);
    // TODO : 재할당 제거?
    pglBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(AirplaneLineInstance), instances.data(), GL_STREAM_DRAW);

    pglUseProgram(gLineInstancing.program);
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    pglUniform2f(pglGetUniformLocation(gLineInstancing.program, "Viewport"), (float)vp[2], (float)vp[3]);
    glLineWidth(3.0f);
    pglDrawArraysInstanced(GL_LINES, 0, 2, instances.size());

    pglBindVertexArray(0);
    pglUseProgram(0);
}
//---------------------------------------------------------------------------
void DrawAirTrackFriend(float x, float y)
 {
  glPushMatrix();
  glTranslated(x,y,0.0);
  glCallList(AIR_TRACK_FRIEND);
  glPopMatrix();
 }
 //---------------------------------------------------------------------------
void DrawAirTrackHostile(float x, float y)
 {
  glPushMatrix();
  glTranslated(x,y,0.0);
  glCallList(AIR_TRACK_HOSTILE);
  glPopMatrix();
 }
//---------------------------------------------------------------------------
void DrawAirTrackUnknown(float x, float y)
 {
  glPushMatrix();
  glTranslated(x,y,0.0);
  glCallList(AIR_TRACK_UNKNOWN);
  glPopMatrix();
 }
//---------------------------------------------------------------------------
 void DrawPoint(float x, float y)
 {
  glPushMatrix();
  glTranslated(x,y,0.0);
  glCallList(SURFACE_TRACK_FRIEND);
  glPopMatrix();
 }
//---------------------------------------------------------------------------
 void DrawTrackHook(float x, float y)
 {
  glPushMatrix();
  glTranslated(x,y,0.0);
  glCallList(TRACKHOOK);
  glPopMatrix();
 }
//---------------------------------------------------------------------------
 void  DrawRadarCoverage(float xc, float yc, float major, float minor)
 {
  glBegin(GL_TRIANGLE_FAN );
  glVertex2f( xc, yc); // Center.
  for( float a = 0;  a <= 360; a+=5 )
   {
	  float ang = (float) RADPERDEG*a;
	  float x = xc + (float) (major*cos( ang ));
	  float y = yc + (float) (minor*sin( ang ));
	  glVertex2f( x, y);
   }
  glEnd();
 }
//---------------------------------------------------------------------------
void DrawLeader(float x1, float y1, float x2, float y2)
{
   // 기본 선 그리기
   glBegin(GL_LINE_STRIP);
   glVertex2f(x1,y1);
   glVertex2f(x2,y2);
   glEnd();
   
   // 화살표 머리 그리기
   float dx = x2 - x1;
   float dy = y2 - y1;
   float length = sqrt(dx*dx + dy*dy);
   
   if (length > 0) {
       // 정규화
       dx /= length;
       dy /= length;
       
       // 화살표 머리 크기
       float arrowSize = 8.0f;
       
       // 화살표 머리의 두 점 계산
       float arrowX1 = x2 - arrowSize * dx + arrowSize * 0.5f * dy;
       float arrowY1 = y2 - arrowSize * dy - arrowSize * 0.5f * dx;
       float arrowX2 = x2 - arrowSize * dx - arrowSize * 0.5f * dy;
       float arrowY2 = y2 - arrowSize * dy + arrowSize * 0.5f * dx;
       
       // 화살표 머리 그리기
       glBegin(GL_TRIANGLES);
       glVertex2f(x2, y2);
       glVertex2f(arrowX1, arrowY1);
       glVertex2f(arrowX2, arrowY2);
       glEnd();
   }
}
//---------------------------------------------------------------------------
void ComputeTimeToGoPosition(float  TimeToGo,
							  float  xs, float  ys,
							  float  xv, float  yv,
							  float &xe, float &ye)
 {
  xe=xs+( xv/3600.00)*TimeToGo;
  ye=ys+( yv/3600.00)*TimeToGo;

 }
 //---------------------------------------------------------------------------
void DrawLines(DWORD resolution, double xpts[],double ypts[])
{
  DWORD i;
	glBegin(GL_LINES);
	for (i=0; i<resolution; i++)
	{
		glVertex3f(xpts[i], ypts[i], 0.1f);
		glVertex3f(xpts[(i+1)%resolution], ypts[(i+1)%resolution], 0.1f);
	}
        glEnd();
}

void InitHexTextInstancing()
{
    LoadGLExtensions();
    if(gHexText.initialized) return;

    const char* vsSrc =
        "#version 330 core\n"
        "layout(location=0) in vec2 vert;\n"
        "layout(location=1) in vec2 uv;\n"
        "layout(location=2) in vec2 pos;\n"
        "layout(location=3) in int glyph;\n"
        "layout(location=4) in vec4 color;\n"
        "uniform vec2 Viewport;\n"
        "uniform float Scale;\n"
        "out vec2 Tex;\n"
        "out vec4 Col;\n"
        "void main(){\n"
        "  vec2 p = pos + vert * Scale;\n"
        "  vec2 ndc = vec2((p.x/Viewport.x)*2.0 - 1.0, (p.y/Viewport.y)*2.0 - 1.0);\n"
        "  gl_Position = vec4(ndc,0.0,1.0);\n"
        "  Tex = vec2((uv.x + glyph) / 16.0, uv.y);\n"
        "  Col = color;\n"
        "}\n";

    const char* fsSrc =
        "#version 330 core\n"
        "in vec2 Tex;\n"
        "in vec4 Col;\n"
        "uniform sampler2D fontTex;\n"
        "uniform vec2 Texel;\n"
        "uniform int Bold;\n"
        "out vec4 Frag;\n"
        "void main(){\n"
        "  float a = texture(fontTex, Tex).r;\n"
        "  if(Bold > 0){\n"
        "    a = max(a, texture(fontTex, Tex + vec2(Texel.x,0)).r);\n"
        "    a = max(a, texture(fontTex, Tex - vec2(Texel.x,0)).r);\n"
        "    a = max(a, texture(fontTex, Tex + vec2(0,Texel.y)).r);\n"
        "    a = max(a, texture(fontTex, Tex - vec2(0,Texel.y)).r);\n"
        "  }\n"
        "  if(a < 0.1) discard;\n"
        "  Frag = vec4(Col.rgb, a * Col.a);\n"
        "}\n";

    gHexText.program = CreateProgram(vsSrc, fsSrc);

    unsigned char atlas[HEX_FONT_HEIGHT][HEX_FONT_WIDTH * HEX_FONT_GLYPHS];

	BuildConsolasHexAtlas(atlas);

    glGenTextures(1, &gHexText.texture);
    glBindTexture(GL_TEXTURE_2D, gHexText.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, HEX_FONT_WIDTH*HEX_FONT_GLYPHS, HEX_FONT_HEIGHT, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    float quad[] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        (float)HEX_FONT_WIDTH, 0.0f, 1.0f, 1.0f,
        (float)HEX_FONT_WIDTH, (float)HEX_FONT_HEIGHT, 1.0f, 0.0f,
        0.0f, (float)HEX_FONT_HEIGHT, 0.0f, 0.0f
    };

    pglGenVertexArrays(1, &gHexText.vao);
    pglBindVertexArray(gHexText.vao);

    pglGenBuffers(1, &gHexText.quadVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gHexText.quadVBO);
    pglBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    pglEnableVertexAttribArray(1);
    pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));

    pglGenBuffers(1, &gHexText.instanceVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gHexText.instanceVBO);
    pglBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    size_t stride = sizeof(HexCharInstance);
    pglEnableVertexAttribArray(2);
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HexCharInstance, x));
    pglVertexAttribDivisor(2, 1);

    pglEnableVertexAttribArray(3);
    pglVertexAttribIPointer(3, 1, GL_INT, stride, (void*)offsetof(HexCharInstance, glyph));
    pglVertexAttribDivisor(3, 1);

    pglEnableVertexAttribArray(4);
    pglVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HexCharInstance, color));
    pglVertexAttribDivisor(4, 1);

    pglBindBuffer(GL_ARRAY_BUFFER, 0);
    pglBindVertexArray(0);

    gHexText.initialized = true;
}

void DrawHexTextInstanced(const std::vector<HexCharInstance>& instances)
{
    if(instances.empty()) return;
    if(!gHexText.initialized)
        InitHexTextInstancing();

    pglBindVertexArray(gHexText.vao);
    pglBindBuffer(GL_ARRAY_BUFFER, gHexText.instanceVBO);
    pglBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(HexCharInstance), instances.data(), GL_STREAM_DRAW);

    pglUseProgram(gHexText.program);
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    pglUniform2f(pglGetUniformLocation(gHexText.program, "Viewport"), (float)vp[2], (float)vp[3]);
    pglUniform1f(pglGetUniformLocation(gHexText.program, "Scale"), gHexText.scale);
    pglUniform1i(pglGetUniformLocation(gHexText.program, "Bold"), gHexText.bold);
    pglUniform2f(pglGetUniformLocation(gHexText.program, "Texel"),
                 1.0f/(float)(HEX_FONT_WIDTH*HEX_FONT_GLYPHS), 1.0f/(float)HEX_FONT_HEIGHT);
    pglActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gHexText.texture);
    pglUniform1i(pglGetUniformLocation(gHexText.program, "fontTex"), 0);

    pglDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, instances.size());

    glBindTexture(GL_TEXTURE_2D, 0);
    pglBindVertexArray(0);
    pglUseProgram(0);
}

void SetHexTextScale(float scale)
{
    gHexText.scale = scale;
}

void SetHexTextBold(bool bold)
{
    gHexText.bold = bold ? 1 : 0;
}

float GetHexTextScale()
{
    return gHexText.scale;
}

// 항공기 타입별 아이콘 선택 함수 추가
int SelectAircraftIcon(const TADS_B_Aircraft* aircraft)
{
    if (!aircraft) return 0;
    
    // CycleImages가 체크되어 있으면 항공기에 할당된 SpriteImage 사용
    // (이 경우 AircraftDataModel에서 랜덤하게 할당됨)
    if (aircraft->SpriteImage >= 0) {
        return aircraft->SpriteImage;
    }
    
    // DB에서 타입코드 조회
    const TAircraftData* dbinfo = (const TAircraftData*) ght_get(AircraftDBHashTable, sizeof(aircraft->ICAO), &aircraft->ICAO);
    if (dbinfo) {
        const char* icaoType = dbinfo->Fields[5].c_str();
        printf("ICAO:%06X TypeCode:%s\n", aircraft->ICAO, icaoType);
        if (strstr(icaoType, "F") && (
            strstr(icaoType, "B763") || strstr(icaoType, "B752") || strstr(icaoType, "B744") || strstr(icaoType, "MD11") || strstr(icaoType, "A306") || strstr(icaoType, "A332") )) {
            printf("화물기 분류됨!\n");
            return 76; // 화물기
        }
        if (strstr(icaoType, "B737") || strstr(icaoType, "A320") || strstr(icaoType, "A321") /* ... */) {
            return 77; // 대형 여객기
        }
    } else {
        printf("ICAO:%06X DB정보 없음\n", aircraft->ICAO);
    }
    
    // 헬리콥터인지 확인
    const char* heloType = NULL;
    bool isHelo = aircraft_is_helicopter(aircraft->ICAO, &heloType);
    if (isHelo) {
        if (heloType) {
            if (strcmp(heloType, "H1P") == 0) return 72;
            else if (strcmp(heloType, "H2P") == 0) return 73;
            else if (strcmp(heloType, "H1T") == 0) return 74;
            else if (strcmp(heloType, "H2T") == 0) return 75;
        }
        return 72;
    }
    
    // 군용기
    if (aircraft_is_military(aircraft->ICAO, NULL)) return 2;

    // ICAO 타입 코드로 화물기/대형 여객기 분류
    if (dbinfo && !dbinfo->Fields[5].IsEmpty()) {
        const char* icaoType = dbinfo->Fields[5].c_str();
        // 대형 여객기: B737, B738, B739, B777, B763, B764, A320, A321, A319, A330, A350 등
        if (strstr(icaoType, "B737") || strstr(icaoType, "B738") || strstr(icaoType, "B739") || strstr(icaoType, "B777") || strstr(icaoType, "B763") || strstr(icaoType, "B764") || strstr(icaoType, "A320") || strstr(icaoType, "A321") || strstr(icaoType, "A319") || strstr(icaoType, "A330") || strstr(icaoType, "A350") ) {
            return 77; // 대형 여객기
        }
    }
    // FlightNum으로 화물기 감지 (UPS, FedEx 등)
    if (aircraft->HaveFlightNum) {
        const char* flightNum = aircraft->FlightNum;
        if (flightNum && (strstr(flightNum, "5X") || strstr(flightNum, "FX") || strstr(flightNum, "1X") || strstr(flightNum, "9X"))) {
            return 76; // 화물기
        }
    }
    // FlightNum으로 대형 여객기 감지 (UA, AA, DL, WN 등)
    if (aircraft->HaveFlightNum) {
        const char* flightNum = aircraft->FlightNum;
        if (flightNum && (strstr(flightNum, "UA") || strstr(flightNum, "AA") || strstr(flightNum, "DL") || strstr(flightNum, "WN"))) {
            return 77; // 대형 여객기
        }
    }
    // 고도/속도 조건
    if (aircraft->HaveAltitude) {
        if (aircraft->Altitude > 30000) return 3;
        else if (aircraft->Altitude < 10000) return 4;
    }
    if (aircraft->HaveSpeedAndHeading) {
        if (aircraft->Speed > 500) return 5;
        else if (aircraft->Speed < 100) return 6;
    }
    return 0;
}

//---------------------------------------------------------------------------
void InitAirportInstancing()
{
    LoadGLExtensions();
    if(gAirportInst.initialized) return;

    const char* vsSrc =
        "#version 330 core\n"
        "layout(location=0) in vec2 vert;\n"
        "layout(location=1) in vec2 uv;\n"
        "layout(location=2) in vec2 pos;\n"
        "layout(location=3) in int  type;\n"
        "uniform vec2 Viewport;\n"
        "uniform float Size;\n"
        "out vec2 Tex;\n"
        "flat out int Icon;\n"
        "void main(){\n"
        "  vec2 p = pos + vert * Size * 0.5;\n"
        "  vec2 ndc = vec2((p.x/Viewport.x)*2.0 - 1.0, (p.y/Viewport.y)*2.0 - 1.0);\n"
        "  gl_Position = vec4(ndc,0.0,1.0);\n"
        "  Tex = uv;\n"
        "  Icon = type;\n"
        "}\n";

    const char* fsSrc =
        "#version 330 core\n"
        "in vec2 Tex;\n"
        "flat in int Icon;\n"
        "out vec4 Frag;\n"
        "uniform sampler2DArray atlasTex;\n"
        "void main(){\n"
        "  Frag = texture(atlasTex, vec3(Tex, Icon));\n"
        "}\n";

    gAirportInst.program = CreateProgram(vsSrc, fsSrc);

    float quad[] = {
        -1.0f,-1.0f, 0.0f,0.0f,
         1.0f,-1.0f, 1.0f,0.0f,
         1.0f, 1.0f, 1.0f,1.0f,
        -1.0f, 1.0f, 0.0f,1.0f
    };

    pglGenVertexArrays(1, &gAirportInst.vao);
    pglBindVertexArray(gAirportInst.vao);

    pglGenBuffers(1, &gAirportInst.quadVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gAirportInst.quadVBO);
    pglBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    pglEnableVertexAttribArray(1);
    pglVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));

    pglGenBuffers(1, &gAirportInst.instanceVBO);
    pglBindBuffer(GL_ARRAY_BUFFER, gAirportInst.instanceVBO);
    pglBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    size_t stride = sizeof(AirportInstance);
    pglEnableVertexAttribArray(2);
    pglVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(AirportInstance,x));
    pglVertexAttribDivisor(2,1);
    pglEnableVertexAttribArray(3);
    pglVertexAttribIPointer(3,1,GL_INT,stride,(void*)offsetof(AirportInstance,type));
    pglVertexAttribDivisor(3,1);

    pglBindBuffer(GL_ARRAY_BUFFER,0);
    pglBindVertexArray(0);

    gAirportInst.initialized = true;
}

//---------------------------------------------------------------------------
void DrawAtlasIcon(double x, double y, const AtlasRect& rect, GLuint texId,
                   int iconDrawSize)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texId);

    float u0 = rect.x / (float)atlasW;
    float v0 = rect.y / (float)atlasH;
    float u1 = (rect.x + rect.w) / (float)atlasW;
    float v1 = (rect.y + rect.h) / (float)atlasH;
    float sz = (float)iconDrawSize;

    glColor4f(1,1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x - sz/2, y - sz/2);
    glTexCoord2f(u1, v0); glVertex2f(x + sz/2, y - sz/2);
    glTexCoord2f(u1, v1); glVertex2f(x + sz/2, y + sz/2);
    glTexCoord2f(u0, v1); glVertex2f(x - sz/2, y + sz/2);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

//---------------------------------------------------------------------------
void DrawAirportMarkers()
{
    if(apiAirportList.empty()) return;
    if(!gAirportInst.initialized)
        InitAirportInstancing();

    auto& instances = gAirportInst.instances;
    instances.clear();

    for(const auto& airport : apiAirportList){
        if(!airport.iata.empty() && fabs(airport.latitude) > 0.01 && fabs(airport.longitude) > 0.01){
            double x, y;
            Form1->LatLon2XY(airport.latitude, airport.longitude, x, y);

            if(x < 0 || x > Form1->ObjectDisplay->Width ||
               y < 0 || y > Form1->ObjectDisplay->Height)
                continue;

            if(instances.size() >= MAX_AIRPORT_INSTANCES) break;
            AirportInstance a;
            a.x = (float)x;
            a.y = (float)y;
            a.type = (int)GetAirportType(airport);
            instances.push_back(a);
        }
    }

    if(instances.empty()) return;

    pglBindVertexArray(gAirportInst.vao);
    pglBindBuffer(GL_ARRAY_BUFFER, gAirportInst.instanceVBO);
    pglBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(AirportInstance), instances.data(), GL_STREAM_DRAW);

    pglUseProgram(gAirportInst.program);
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    pglUniform2f(pglGetUniformLocation(gAirportInst.program, "Viewport"), (float)vp[2], (float)vp[3]);
    pglUniform1f(pglGetUniformLocation(gAirportInst.program, "Size"), 28.0f);

    pglActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlasTexId);
    pglUniform1i(pglGetUniformLocation(gAirportInst.program, "atlasTex"), 0);

    pglDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, instances.size());

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    pglBindVertexArray(0);
    pglUseProgram(0);
}

// 다양한 리더 스타일 함수들
void DrawLeaderArrow(float x1, float y1, float x2, float y2, float arrowSize)
{
   // 기본 선 그리기
   glBegin(GL_LINE_STRIP);
   glVertex2f(x1,y1);
   glVertex2f(x2,y2);
   glEnd();
   
   // 화살표 머리 그리기
   float dx = x2 - x1;
   float dy = y2 - y1;
   float length = sqrt(dx*dx + dy*dy);
   
   if (length > 0) {
       // 정규화
       dx /= length;
       dy /= length;
       
       // 화살표 머리의 두 점 계산
       float arrowX1 = x2 - arrowSize * dx + arrowSize * 0.5f * dy;
       float arrowY1 = y2 - arrowSize * dy - arrowSize * 0.5f * dx;
       float arrowX2 = x2 - arrowSize * dx - arrowSize * 0.5f * dy;
       float arrowY2 = y2 - arrowSize * dy + arrowSize * 0.5f * dx;
       
       // 화살표 머리 그리기
       glBegin(GL_TRIANGLES);
       glVertex2f(x2, y2);
       glVertex2f(arrowX1, arrowY1);
       glVertex2f(arrowX2, arrowY2);
       glEnd();
   }
}

void DrawLeaderDashed(float x1, float y1, float x2, float y2)
{
   // 점선 스타일 설정
   glEnable(GL_LINE_STIPPLE);
   glLineStipple(2, 0xAAAA); // 점선 패턴
   
   glBegin(GL_LINE_STRIP);
   glVertex2f(x1,y1);
   glVertex2f(x2,y2);
   glEnd();
   
   glDisable(GL_LINE_STIPPLE);
}

void DrawLeaderThick(float x1, float y1, float x2, float y2, float thickness)
{
   // 두꺼운 선 그리기
   glLineWidth(thickness);
   
   glBegin(GL_LINE_STRIP);
   glVertex2f(x1,y1);
   glVertex2f(x2,y2);
   glEnd();
   
   glLineWidth(1.0f); // 기본 두께로 복원
}

#if 0
//---------------------------------------------------------------------------
// SAVE ORIGINAL CODE   ** DO NOT DELETE
//
  void DrawCovarianceEllipse(float c[][2], float cx,float cy, float sf)
  {
   float a,b,term1,term2,term3,term4;
   float major, minor, theta;
   //find direction of maximum variance
   a= 2.0*c[1][0];
   b= (c[0][0]-c[1][1]);
   if ((a==0.0)&& (b==0.0)) return;

   theta=0.5*atan2(a,b);
   //% find major and minor axis amplitudes
   term1=(c[0][0]+c[1][1]);
   term2=(c[0][0]-c[1][1]) *
			  (c[0][0]-c[1][1]) +
			  4.0*c[1][0]*c[1][0];
   if (term2<0.0) return;
   term2=sqrt(term2);
   term3=0.5*(term1+term2);
   term4=0.5*(term1-term2);
   if ((term3<0.0)|| (term4<0.0)) return;
   major=sqrt(term3);
   minor=sqrt(term4);
   theta*=DEGPERRAD;
   _DrawEllipse(theta, major*3, minor*3,cx,cy,sf );
 }
//---------------------------------------------------------------------------
// SAVE ORIGINAL CODE   ** DO NOT DELETE
//
 //---------------------------------------------------------------------------
 void _DrawEllipse(float major_deg, float major_len, float minor_len, float cx,
				   float cy, float sf)
{
const int resolution = 100;
float xpts[resolution], ypts[resolution], newy[resolution], newx[resolution];
float theta, theta_step, major_rad;
int i;

	major_rad = major_deg * 3.1416f / 180.0f;
	theta_step = 2.0f * 3.1416f / (float)resolution;
	for (theta=0.0, i=0; i<resolution; i++, theta += theta_step)
	{
		xpts[i] = major_len * (float)cos(theta);
		ypts[i] = minor_len * (float)sin(theta);
	}
	for (i=0; i<resolution; i++)
	{
		newx[i] = (float)(cx + (xpts[i] * (float)cos(major_rad) - ypts[i] * (float)sin(major_rad)))*sf;
		newy[i] = (float)(cy + (xpts[i] * (float)sin(major_rad) + ypts[i] * (float)cos(major_rad)))*sf;
	}

	glBegin(GL_LINES);
	for (i=0; i<resolution; i++)
	{
		glVertex3f(newx[i], newy[i], 0.1f);
		glVertex3f(newx[(i+1)%resolution], newy[(i+1)%resolution], 0.1f);
	}
	//glVertex3f(newx[0], newy[0], 0.1f);
	//glVertex3f(newx[resolution/2], newy[resolution/2], 0.1f);
	//glVertex3f(newx[resolution/4], newy[resolution/4], 0.1f);
	//glVertex3f(newx[3*resolution/4], newy[3*resolution/4], 0.1f);
	glEnd();
}
 //---------------------------------------------------------------------------
#endif







