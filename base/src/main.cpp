#include "main.h"

#include "Utility.h"

#include "SOIL.h"
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_projection.hpp>
#include <glm/gtc/matrix_operation.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <list>
#include <random>
#include <ctime>

#include "InstantRadiosity.h"

using namespace std;
using namespace glm;

const float PI = 3.14159f;
const int MAX_LIGHTS_PER_TILE = 64;

int width, height;
float inv_width, inv_height;
bool bloomEnabled = false, toonEnabled = false, DOFEnabled = false, DOFDebug = false;

int mouse_buttons = 0;
int mouse_old_x = 0, mouse_dof_x = 0;
int mouse_old_y = 0, mouse_dof_y = 0;

int nVPLs = 256;
int nLights = 0;
int nBounces = 2;

float FARP = 100.f;
float NEARP = 0.1f;

GLuint lightPosSBO = 0;
GLuint vplPosSBO = 0;
GLuint rayInfoSBO = 0;
GLuint bBoxSBO = 0;
GLuint lightListSBO = 0;
GLuint debugBufferSBO = 0;

std::list<LightData> lightList;
std::list<bBox>	boundingBoxes;
std::default_random_engine random_gen (time (NULL));

device_mesh_t uploadMesh(const mesh_t & mesh) 
{
    device_mesh_t out;
    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex 
    //attributes and the buffers they are bound to
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(out.vertex_array));
    glBindVertexArray(out.vertex_array);

    //Allocate vbos for data
    glGenBuffers(1, &(out.vbo_vertices));
    glGenBuffers(1, &(out.vbo_normals));
    glGenBuffers(1, &(out.vbo_indices));
    glGenBuffers(1, &(out.vbo_texcoords));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size()*sizeof(vec3), 
            &mesh.vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::POSITION);
    //cout << mesh.vertices.size() << " verts:" << endl;
    //for(int i = 0; i < mesh.vertices.size(); ++i)
    //    cout << "    " << mesh.vertices[i][0] << ", " << mesh.vertices[i][1] << ", " << mesh.vertices[i][2] << endl;

    //Upload normal data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, mesh.normals.size()*sizeof(vec3), 
            &mesh.normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::NORMAL, 3, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::NORMAL);
    //cout << mesh.normals.size() << " norms:" << endl;
    //for(int i = 0; i < mesh.normals.size(); ++i)
    //    cout << "    " << mesh.normals[i][0] << ", " << mesh.normals[i][1] << ", " << mesh.normals[i][2] << endl;

    //Upload texture coord data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_texcoords);
    glBufferData(GL_ARRAY_BUFFER, mesh.texcoords.size()*sizeof(vec2), 
            &mesh.texcoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::TEXCOORD);
    //cout << mesh.texcoords.size() << " texcos:" << endl;
    //for(int i = 0; i < mesh.texcoords.size(); ++i)
    //    cout << "    " << mesh.texcoords[i][0] << ", " << mesh.texcoords[i][1] << endl;

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(GLushort), 
            &mesh.indices[0], GL_STATIC_DRAW);
    out.num_indices = mesh.indices.size();
    //Unplug Vertex Array
    glBindVertexArray(0);

    out.texname = mesh.texname;
    out.color = mesh.color;
    return out;
}

int num_boxes = 3;
const int DUMP_SIZE = 1024;
mat4x4	get_mesh_world ();

vector<device_mesh_t> draw_meshes;
void initMesh() 
{
	mat4 modelMatrix = get_mesh_world ();
    for(vector<tinyobj::shape_t>::iterator it = shapes.begin();
            it != shapes.end(); ++it)
    {
		bBox BoundingBox;
		BoundingBox.min = glm::vec4 (1e6, 1e6, 1e6, 1.0);
		BoundingBox.max = glm::vec4 (-1e6, -1e6f, -1e6f, 1.0);

        tinyobj::shape_t shape = *it;
        int totalsize = shape.mesh.indices.size() / 3;
        int f = 0;
        while(f<totalsize)
		{
            mesh_t mesh;
            int process = std::min(10000, totalsize-f);
            int point = 0;
            for(int i=f; i<process+f; i++)
			{
                int idx0 = shape.mesh.indices[3*i];
                int idx1 = shape.mesh.indices[3*i+1];
                int idx2 = shape.mesh.indices[3*i+2];
                vec3 p0 = vec3(shape.mesh.positions[3*idx0],
                               shape.mesh.positions[3*idx0+1],
                               shape.mesh.positions[3*idx0+2]);
                vec3 p1 = vec3(shape.mesh.positions[3*idx1],
                               shape.mesh.positions[3*idx1+1],
                               shape.mesh.positions[3*idx1+2]);
                vec3 p2 = vec3(shape.mesh.positions[3*idx2],
                               shape.mesh.positions[3*idx2+1],
                               shape.mesh.positions[3*idx2+2]);

				vec4 p0_4 = modelMatrix*vec4 (p0.x, p0.y, p0.z, 1.0f);
				vec4 p1_4 = modelMatrix*vec4 (p1.x, p1.y, p1.z, 1.0f);
				vec4 p2_4 = modelMatrix*vec4 (p2.x, p2.y, p2.z, 1.0f);

				float minX = std::min (p0_4.x, std::min (p1_4.x, p2_4.x));
				float minY = std::min (p0_4.y, std::min (p1_4.y, p2_4.y));
				float minZ = std::min (p0_4.z, std::min (p1_4.z, p2_4.z));
				float maxX = std::max (p0_4.x, std::max (p1_4.x, p2_4.x));
				float maxY = std::max (p0_4.y, std::max (p1_4.y, p2_4.y));
				float maxZ = std::max (p0_4.z, std::max (p1_4.z, p2_4.z));
                
				if (minX < BoundingBox.min.x)
					BoundingBox.min.x = minX;
				if (minY < BoundingBox.min.y)
					BoundingBox.min.y = minY;
				if (minZ < BoundingBox.min.z)
					BoundingBox.min.z = minZ;

				if (maxX > BoundingBox.max.x)
					BoundingBox.max.x = maxX;
				if (maxY > BoundingBox.max.y)
					BoundingBox.max.y = maxY;
				if (maxZ > BoundingBox.max.z)
					BoundingBox.max.z = maxZ;

				mesh.vertices.push_back(p0);
                mesh.vertices.push_back(p1);
                mesh.vertices.push_back(p2);

                if(shape.mesh.normals.size() > 0)
                {
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx0],
                                                shape.mesh.normals[3*idx0+1],
                                                shape.mesh.normals[3*idx0+2]));
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx1],
                                                shape.mesh.normals[3*idx1+1],
                                                shape.mesh.normals[3*idx1+2]));
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx2],
                                                shape.mesh.normals[3*idx2+1],
                                                shape.mesh.normals[3*idx2+2]));
                }
                else
                {
                    vec3 norm = normalize(glm::cross(normalize(p1-p0), normalize(p2-p0)));
                    mesh.normals.push_back(norm);
                    mesh.normals.push_back(norm);
                    mesh.normals.push_back(norm);
                }

                if(shape.mesh.texcoords.size() > 0)
                {
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx0],
                                                  shape.mesh.positions[2*idx0+1]));
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx1],
                                                  shape.mesh.positions[2*idx1+1]));
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx2],
                                                  shape.mesh.positions[2*idx2+1]));
                }
                else
                {
                    vec2 tex(0.0);
                    mesh.texcoords.push_back(tex);
                    mesh.texcoords.push_back(tex);
                    mesh.texcoords.push_back(tex);
                }
                mesh.indices.push_back(point++);
                mesh.indices.push_back(point++);
                mesh.indices.push_back(point++);
            }

            mesh.color = vec3(shape.material.diffuse[0],
                              shape.material.diffuse[1],
                              shape.material.diffuse[2]);
            mesh.texname = shape.material.name;//diffuse_texname;
            draw_meshes.push_back(uploadMesh(mesh));
            f=f+process;
        }
		if (shape.material.name == "light")
		{
			LightData	new_light;
			new_light.position = vec4 (2.5, -2.5, 4.0, 1.0);//(mesh.vertices [0] + mesh.vertices [1] + mesh.vertices [2]) / 3.0f; 2.5, -2.5, 4.3) vec3 (3.5, -2.5, 2.0)
			new_light.intensity = vec4(1.0f);
			lightList.push_back (new_light);
		}
		boundingBoxes.push_back (BoundingBox);
    }
	nLights = lightList.size ();
	if (nLights == 0)
	{
		LightData	new_light;
		new_light.position = vec4 (3.5, -2.0, 4.0, 1.0);//(mesh.vertices [0] + mesh.vertices [1] + mesh.vertices [2]) / 3.0f; 2.5, -2.5, 4.3) vec3 (3.5, -2.5, 2.0)
		new_light.intensity = vec4(1.0f);
		lightList.push_back (new_light);
		++nLights;
	}
	
	glGenBuffers (1, &bBoxSBO);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, bBoxSBO);
	glBufferData (GL_SHADER_STORAGE_BUFFER, boundingBoxes.size ()*sizeof(bBox), NULL, GL_STATIC_DRAW);
	GLint bufferAccessMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	bBox * bbBuff = (bBox *) glMapBufferRange (GL_SHADER_STORAGE_BUFFER, 0, boundingBoxes.size ()*sizeof(bBox), bufferAccessMask);
	int count = 0;
	for (std::list<bBox>::iterator i = boundingBoxes.begin (); i != boundingBoxes.end (); ++i)
	{	
		bbBuff [count] = *i;
		++count;
	}
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
}


device_mesh2_t device_quad;
void initQuad()
{
    vertex2_t verts [] = {	{vec3(-1,1,0),vec2(0,1)},
							{vec3(-1,-1,0),vec2(0,0)},
							{vec3(1,-1,0),vec2(1,0)},
							{vec3(1,1,0),vec2(1,1)}		};

    unsigned short indices[] = { 0,1,2,0,2,3};

    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(device_quad.vertex_array));
    glBindVertexArray(device_quad.vertex_array);


    //Allocate vbos for data
    glGenBuffers(1,&(device_quad.vbo_data));
    glGenBuffers(1,&(device_quad.vbo_indices));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, device_quad.vbo_data);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    //Use of strided data, Array of Structures instead of Structures of Arrays
    glVertexAttribPointer(quad_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),0);
    glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(vec3));
    glEnableVertexAttribArray(quad_attributes::POSITION);
    glEnableVertexAttribArray(quad_attributes::TEXCOORD);

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);
    device_quad.num_indices = 6;
    //Unplug Vertex Array
    glBindVertexArray(0);
}


GLuint depthTexture = 0;
GLuint normalTexture = 0;
GLuint positionTexture = 0;
GLuint colorTexture = 0;
GLuint postTexture = 0;
GLuint glowmaskTexture = 0;
GLuint depthMapTexture = 0;

GLuint lightcordTexture = 0 ;
GLuint FBO[3] = {0, 0, 0};


GLuint vpl_prog;
GLuint pass_prog;
GLuint point_prog;
GLuint ambient_prog;
GLuint diagnostic_prog;
GLuint post_prog;

GLuint fplus_lightcull_prog = 0;
GLuint fplus_shading_prog = 0;

GLuint forward_shading_prog = 0;


void initShader() 
{
#ifdef WIN32

	// Common shaders
	const char * vpl_init = "../../../res/shaders/vpl_compute.glsl";
	const char * pass_vert = "../../../res/shaders/pass.vert";
	
	// Deferred shaders
	const char * shade_vert = "../../../res/shaders/shade.vert";
	const char * post_vert = "../../../res/shaders/post.vert";
	const char * pass_frag = "../../../res/shaders/pass.frag";
	const char * diagnostic_frag = "../../../res/shaders/diagnostic.frag";
	const char * ambient_frag = "../../../res/shaders/ambient.frag";
	const char * point_frag = "../../../res/shaders/point.frag";
	const char * post_frag = "../../../res/shaders/post.frag";

	// Forward shaders
	const char * forward_frag = "../../../res/shaders/forward_frag.glsl";
	const char * fplus_frag = "../../../res/shaders/fplus_frag.glsl";
	const char * fplus_lightcull = "../../../res/shaders/fplus_lightcull.glsl";
#else
	const char * pass_vert = "../res/shaders/pass.vert";
	const char * shade_vert = "../res/shaders/shade.vert";
	const char * post_vert = "../res/shaders/post.vert";

	const char * pass_frag = "../res/shaders/pass.frag";
	const char * diagnostic_frag = "../res/shaders/diagnostic.frag";
	const char * ambient_frag = "../res/shaders/ambient.frag";
	const char * point_frag = "../res/shaders/point.frag";
	const char * post_frag = "../res/shaders/post.frag";
#endif
	GLuint cshader = Utility::loadComputeShader (vpl_init);
	vpl_prog = glCreateProgram ();
	Utility::attachAndLinkCSProgram (vpl_prog, cshader);

	GLuint fplcshader = Utility::loadComputeShader (fplus_lightcull);
	fplus_lightcull_prog = glCreateProgram ();
	Utility::attachAndLinkCSProgram (fplus_lightcull_prog, fplcshader);

	Utility::shaders_t shaders = Utility::loadShaders(pass_vert, forward_frag);
	forward_shading_prog = glCreateProgram();
	glBindAttribLocation(forward_shading_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::NORMAL, "Normal");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(forward_shading_prog, shaders);

	shaders = Utility::loadShaders(pass_vert, fplus_frag);
	fplus_shading_prog = glCreateProgram();
	glBindAttribLocation(forward_shading_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::NORMAL, "Normal");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(fplus_shading_prog, shaders);

	shaders = Utility::loadShaders(pass_vert, pass_frag);
	pass_prog = glCreateProgram();
	glBindAttribLocation(pass_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(pass_prog, mesh_attributes::NORMAL, "Normal");
    glBindAttribLocation(pass_prog, mesh_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(pass_prog,shaders);

	shaders = Utility::loadShaders(shade_vert, diagnostic_frag);
	diagnostic_prog = glCreateProgram();
	glBindAttribLocation(diagnostic_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(diagnostic_prog, quad_attributes::TEXCOORD, "Texcoord");
    Utility::attachAndLinkProgram(diagnostic_prog, shaders);

	shaders = Utility::loadShaders(shade_vert, ambient_frag);
    ambient_prog = glCreateProgram();
    glBindAttribLocation(ambient_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(ambient_prog, quad_attributes::TEXCOORD, "Texcoord");
    Utility::attachAndLinkProgram(ambient_prog, shaders);

	shaders = Utility::loadShaders(shade_vert, point_frag);
    point_prog = glCreateProgram();
    glBindAttribLocation(point_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(point_prog, quad_attributes::TEXCOORD, "Texcoord");
    Utility::attachAndLinkProgram(point_prog, shaders);

	shaders = Utility::loadShaders(post_vert, post_frag);
    post_prog = glCreateProgram();
    glBindAttribLocation(post_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(post_prog, quad_attributes::TEXCOORD, "Texcoord");
    Utility::attachAndLinkProgram(post_prog, shaders);
}

void freeFBO() 
{
    glDeleteTextures(1,&depthTexture);
    glDeleteTextures(1,&normalTexture);
    glDeleteTextures(1,&positionTexture);
    glDeleteTextures(1,&colorTexture);
    glDeleteTextures(1,&postTexture);
	glDeleteTextures(1,&depthMapTexture);
    glDeleteTextures(1,&lightcordTexture);
    glDeleteFramebuffers(1,&FBO[0]);
    glDeleteFramebuffers(1,&FBO[1]);
	glDeleteFramebuffers(1,&FBO[2]);
}

void checkFramebufferStatus(GLenum framebufferStatus) 
{
    switch (framebufferStatus) 
	{
        case GL_FRAMEBUFFER_COMPLETE_EXT: break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
                                          printf("Attachment Point Unconnected\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                                          printf("Missing Attachment\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                                          printf("Dimensions do not match\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
                                          printf("Formats\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
                                          printf("Draw Buffer\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
                                          printf("Read Buffer\n");
                                          break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                                          printf("Unsupported Framebuffer Configuration\n");
                                          break;
        default:
                                          printf("Unkown Framebuffer Object Failure\n");
                                          break;
    }
}

GLuint random_normal_tex;
GLuint random_scalar_tex;
void initNoise() 
{ 

#ifdef WIN32
	const char * rand_norm_png = "../../../res/random_normal.png";
	const char * rand_png = "../../../res/random.png";
#else
	const char * rand_norm_png = "../res/random_normal.png";
	const char * rand_png = "../res/random.png";
#endif
	random_normal_tex = (unsigned int)SOIL_load_OGL_texture(rand_norm_png,0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

	random_scalar_tex = (unsigned int)SOIL_load_OGL_texture(rand_png,0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void initFBO(int w, int h) 
{
    GLenum FBOstatus;

    glActiveTexture(GL_TEXTURE9);

    glGenTextures(1, &depthTexture);
    glGenTextures(1, &normalTexture);
    glGenTextures(1, &positionTexture);
    glGenTextures(1, &colorTexture);
	glGenTextures(1, &depthMapTexture);
	glGenTextures (1, &glowmaskTexture);
	glGenTextures (1, &lightcordTexture);

    //Set up depth FBO
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    //Set up normal FBO
    glBindTexture(GL_TEXTURE_2D, normalTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    //Set up position FBO
    glBindTexture(GL_TEXTURE_2D, positionTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	  //Set up Light coordinate FBO
    glBindTexture(GL_TEXTURE_2D, lightcordTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    //Set up color FBO
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	 //Set up shadow FBO
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    //Set up glowmap FBO
    glBindTexture(GL_TEXTURE_2D, glowmaskTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);
	glGenerateMipmap (GL_TEXTURE_2D);

	// creatwwe a framebuffer object
    glGenFramebuffers(1, &FBO[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_NONE);
    GLint normal_loc = glGetFragDataLocation(pass_prog,"out_Normal");
    GLint position_loc = glGetFragDataLocation(pass_prog,"out_Position");
    GLint color_loc = glGetFragDataLocation(pass_prog,"out_Color");
    GLint glowmask_loc = glGetFragDataLocation(pass_prog,"out_GlowMask");
	GLint lightcoord_loc = glGetFragDataLocation(pass_prog,"out_LightCoord");

	GLenum draws [5];
    draws[normal_loc] = GL_COLOR_ATTACHMENT0;
    draws[position_loc] = GL_COLOR_ATTACHMENT1;
    draws[color_loc] = GL_COLOR_ATTACHMENT2;
	draws[glowmask_loc] = GL_COLOR_ATTACHMENT3;
	draws[lightcoord_loc] = GL_COLOR_ATTACHMENT4;

	glDrawBuffers(5, draws);

    // attach the texture to FBO depth attachment point
    int test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[normal_loc], normalTexture, 0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[position_loc], positionTexture, 0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[color_loc], colorTexture, 0);
	glBindTexture(GL_TEXTURE_2D, glowmaskTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[glowmask_loc], glowmaskTexture, 0);
	glBindTexture(GL_TEXTURE_2D, lightcordTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[lightcoord_loc], lightcordTexture, 0);
	

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) 
	{
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[0]\n");
        checkFramebufferStatus(FBOstatus);
    }

    //Post Processing buffer!
    glActiveTexture(GL_TEXTURE9);

    glGenTextures(1, &postTexture);

    //Set up post FBO
    glBindTexture(GL_TEXTURE_2D, postTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    // creatwwe a framebuffer object
    glGenFramebuffers(1, &FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);

//	checkError (" in initFBO: Marker."); 
    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_BACK);
	glGetError ();
    color_loc = glGetFragDataLocation(ambient_prog,"out_Color");
    GLenum draw[1];
    draw[color_loc] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, draw);

    // attach the texture to FBO depth attachment point
    test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, postTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, draw[color_loc], postTexture, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) 
	{
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[1]\n");
        checkFramebufferStatus(FBOstatus);
    }

	glGenFramebuffers (1, &FBO[2]);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO[2]);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapTexture, 0);
	glDrawBuffer (GL_NONE);

    // switch back to window-system-provided framebuffer
    glClear(GL_DEPTH_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void bindFBO(int buf) 
{
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); //Bad mojo to unbind the framebuffer using the texture
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[buf]);
    glClear(GL_DEPTH_BUFFER_BIT);
    //glColorMask(false,false,false,false);
    glEnable(GL_DEPTH_TEST);
}

void setTextures() 
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glColorMask(true,true,true,true);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
}


Camera cam(vec3(2.5, 5, 2),
        normalize(vec3(0,-1,0)),
        normalize(vec3(0,0,1)));
void	Camera::adjust (float dx, // look left right
						float dy, //look up down
						float dz,
						float tx, //strafe left right
						float ty,
						float tz)//go forward) //strafe up down
{

    if (abs(dx) > 0) 
	{
        rx += dx;
        rx = fmod(rx,360.0f);
    }

    if (abs(dy) > 0) 
	{
        ry += dy;
        ry = clamp(ry,-70.0f, 70.0f);
    }

    if (abs(tx) > 0) 
	{
        vec3 dir = glm::gtx::rotate_vector::rotate(start_dir,rx + 90,up);
        vec2 dir2(dir.x,dir.y);
        vec2 mag = dir2 * tx;
        pos += mag;	
    }

    if (abs(ty) > 0) 
	{
        z += ty;
    }

    if (abs(tz) > 0) 
	{
        vec3 dir = glm::gtx::rotate_vector::rotate(start_dir,rx,up);
        vec2 dir2(dir.x,dir.y);
        vec2 mag = dir2 * tz;
        pos += mag;
    }
}

mat4x4 Camera::get_view() 
{
    vec3 inclin = glm::gtx::rotate_vector::rotate(start_dir,ry,start_left);
    vec3 spun = glm::gtx::rotate_vector::rotate(inclin,rx,up);
    vec3 cent(pos, z);
    return lookAt(cent, cent + spun, up);
}


Light lig(vec3(3.5, -4.0, 5.3),
        normalize(vec3(0,0,-1.0)),
        normalize(vec3(0,1,0)));

mat4x4 Light::get_light_view() 
{
    vec3 inclin = glm::gtx::rotate_vector::rotate(start_dir,ry,start_left);
    vec3 spun = glm::gtx::rotate_vector::rotate(inclin,rx,up);
    vec3 cent(pos, z);
    return lookAt(cent, cent + spun, up);
}



mat4x4 get_mesh_world() 
{
    vec3 tilt(1.0f,0.0f,0.0f);
    //mat4 translate_mat = glm::translate(glm::vec3(0.0f,.5f,0.0f));
    mat4 tilt_mat = glm::rotate(mat4(), 90.0f, tilt);
    mat4 scale_mat = glm::scale(mat4(), vec3(0.01));
    return tilt_mat * scale_mat; //translate_mat;
}

void draw_mesh(Render render_type) 
{
    glUseProgram(pass_prog);

	LightData first = lightList.front();
	lig = Light (vec3 (first.position.x, first.position.y, first.position.z), normalize(vec3(0,0,-1.0)), normalize(vec3(0,1,0)));

    mat4 model = get_mesh_world();
	mat4 view,lview, persp, lpersp;
	if(render_type == RENDER_CAMERA)
	{	
		view = cam.get_view(); // Camera view Matrix
		lview = lig.get_light_view();
		persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
		lpersp = perspective(120.0f,(float)width/(float)height,NEARP,FARP);
	}
	else if(render_type == RENDER_LIGHT)
	{
		view = lig.get_light_view(); // Light view Matrix
		persp = perspective(90.0f,(float)width/(float)height,NEARP,FARP);
	}
//    persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    mat4 inverse_transposed = transpose(inverse(view*model));

    glUniform1f(glGetUniformLocation(pass_prog, "u_Far"), FARP);
	glUniform1f(glGetUniformLocation(pass_prog, "u_Near"), NEARP);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_Model"),1,GL_FALSE,&model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_View"),1,GL_FALSE,&view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_lView"),1,GL_FALSE,&lview[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_Persp"),1,GL_FALSE,&persp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_LPersp"),1,GL_FALSE,&lpersp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_InvTrans") ,1,GL_FALSE,&inverse_transposed[0][0]);


    for(int i=0; i<draw_meshes.size(); i++)
	{
        glUniform3fv(glGetUniformLocation(pass_prog, "u_Color"), 1, &(draw_meshes[i].color[0]));
        glBindVertexArray(draw_meshes[i].vertex_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_meshes[i].vbo_indices);
        
		float glowmask = 0.0f;
		if (draw_meshes [i].texname == "light")
			glowmask = 1.0f;
		glUniform1f (glGetUniformLocation (pass_prog, "glowmask"), glowmask);
		glDrawElements(GL_TRIANGLES, draw_meshes[i].num_indices, GL_UNSIGNED_SHORT,0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void draw_mesh_forward () 
{
    glUseProgram(forward_shading_prog);

    mat4 model = get_mesh_world();
	mat4 view,lview, persp, lpersp;

	view = cam.get_view(); // Camera view Matrix
	lview = lig.get_light_view();
	persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
	lpersp = perspective(120.0f,(float)width/(float)height,NEARP,FARP);

	//    persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    mat4 inverse_transposed = transpose(inverse(view*model));
	mat4 view_inverse = inverse (view);

    glUniform1f(glGetUniformLocation(forward_shading_prog, "u_Far"), FARP);
	glUniform1f(glGetUniformLocation(forward_shading_prog, "u_Near"), NEARP);
    
	glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numLights"), nLights);
	glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), nVPLs);

	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_Model"),1,GL_FALSE,&model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_View"),1,GL_FALSE,&view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_lView"),1,GL_FALSE,&lview[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_Persp"),1,GL_FALSE,&persp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_LPersp"),1,GL_FALSE,&lpersp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_InvTrans") ,1,GL_FALSE,&inverse_transposed[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_ViewInverse") ,1,GL_FALSE,&view_inverse[0][0]);

    for(int i=0; i<draw_meshes.size(); i++)
	{
        glUniform3fv(glGetUniformLocation(forward_shading_prog, "u_Color"), 1, &(draw_meshes[i].color[0]));
        glBindVertexArray(draw_meshes[i].vertex_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_meshes[i].vbo_indices);
        
		glDrawElements(GL_TRIANGLES, draw_meshes[i].num_indices, GL_UNSIGNED_SHORT,0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void draw_mesh_fplus () 
{
    glUseProgram(fplus_shading_prog);

    mat4 model = get_mesh_world();
	mat4 view,lview, persp, lpersp;

	view = cam.get_view(); // Camera view Matrix
	lview = lig.get_light_view();
	persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
	lpersp = perspective(120.0f,(float)width/(float)height,NEARP,FARP);

    mat4 inverse_transposed = transpose(inverse(view*model));
	mat4 view_inverse = inverse (view);
	uvec2 resolution = uvec2 (width, height);

	glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);

	glUniform1i (glGetUniformLocation (fplus_shading_prog, "depthTex"),0);
	glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_Model"),1,GL_FALSE,&model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_View"),1,GL_FALSE,&view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_lView"),1,GL_FALSE,&lview[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_Persp"),1,GL_FALSE,&persp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_LPersp"),1,GL_FALSE,&lpersp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_InvTrans") ,1,GL_FALSE,&inverse_transposed[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(fplus_shading_prog,"u_ViewInverse") ,1,GL_FALSE,&view_inverse[0][0]);
 	glUniform2uiv (glGetUniformLocation(fplus_shading_prog, "resolution"), 1, &resolution[0]);

    for(int i=0; i<draw_meshes.size(); i++)
	{
        glUniform3fv(glGetUniformLocation(fplus_shading_prog, "u_Color"), 1, &(draw_meshes[i].color[0]));
        glBindVertexArray(draw_meshes[i].vertex_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_meshes[i].vbo_indices);
        
		glDrawElements(GL_TRIANGLES, draw_meshes[i].num_indices, GL_UNSIGNED_SHORT,0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}


enum Display display_type = DISPLAY_TOTAL;
void setup_quad(GLuint prog)
{
    glUseProgram(prog);
    glEnable(GL_TEXTURE_2D);

    mat4 persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    vec4 test(-2,0,10,1);
    vec4 testp = persp * test;
    vec4 testh = testp / testp.w;
    vec2 coords = vec2(testh.x, testh.y) / 2.0f + 0.5f;
    glUniform1i(glGetUniformLocation(prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(prog, "u_ScreenWidth"), width);
    glUniform1f(glGetUniformLocation(prog, "u_Far"), FARP);
    glUniform1f(glGetUniformLocation(prog, "u_Near"), NEARP);
    glUniform1i(glGetUniformLocation(prog, "u_DisplayType"), display_type);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_Persp"),1, GL_FALSE, &persp[0][0] );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Depthtex"),0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Normaltex"),1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Positiontex"),2);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Colortex"),3);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(prog, "u_RandomScalartex"),5);

	glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, glowmaskTexture);
    glUniform1i(glGetUniformLocation(prog, "u_GlowMask"),6);

	glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Shadowtex"),7);

	glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, lightcordTexture);
    glUniform1i(glGetUniformLocation(prog, "u_lightCordTex"),8);
}

void draw_quad() 
{
    glBindVertexArray(device_quad.vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);

    glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT,0);

    glBindVertexArray(0);
}

void draw_light(vec3 pos, float strength, mat4 sc, mat4 vp, float NEARP) 
{
    float radius = strength;
    vec4 light = cam.get_view() * vec4(pos, 1.0); 
    if( light.z > NEARP)
    {
        return;
    }
    light.w = radius;
    glUniform4fv(glGetUniformLocation(point_prog, "u_Light"), 1, &(light[0]));
    glUniform1f(glGetUniformLocation(point_prog, "u_LightIl"), strength);

    vec4 left = vp * vec4(pos + radius*cam.start_left, 1.0);
    vec4 up = vp * vec4(pos + radius*cam.up, 1.0);
    vec4 center = vp * vec4(pos, 1.0);

    left = sc * left;
    up = sc * up;
    center = sc * center;

    left /= left.w;
    up /= up.w;
    center /= center.w;

    float hw = glm::distance(left, center);
    float hh = glm::distance(up, center);

    float r = (hh > hw) ? hh : hw;

    float x = center.x-r;
    float y = center.y-r;

    glScissor(x, y, 2*r, 2*r);
    draw_quad();
}

void updateDisplayText(char * disp) 
{
    switch(display_type) 
	{
        case(DISPLAY_DEPTH):
            sprintf(disp, "Displaying Depth");
            break; 
        case(DISPLAY_NORMAL):
            sprintf(disp, "Displaying Normal");
            break; 
        case(DISPLAY_COLOR):
            sprintf(disp, "Displaying Color");
            break;
        case(DISPLAY_POSITION):
            sprintf(disp, "Displaying Position");
            break;
        case(DISPLAY_TOTAL):
            sprintf(disp, "Displaying Diffuse");
            break;
        case(DISPLAY_LIGHTS):
            sprintf(disp, "Displaying Lights");
            break;
		case DISPLAY_GLOWMASK:
			sprintf (disp, "Displaying Glow Mask");
			break;
		case(DISPLAY_SHADOW):
            sprintf(disp, "Displaying ShadowMap");
            break;
    }
}

int frame = 0;
int currenttime = 0;
int timebase = 0;
char title[1024];
char disp[1024];
char occl[1024];
void updateTitle() 
{
    updateDisplayText(disp);
    //calculate the frames per second
    frame++;

    //get the current time
    currenttime = glutGet(GLUT_ELAPSED_TIME);

    //check if a second has passed
    if (currenttime - timebase > 1000) 
    {
		if (bloomEnabled)
			strcat (disp, " Bloom ON");
		else
			strcat (disp, " Bloom OFF");

		if (toonEnabled)
			strcat (disp, " Toon Shaded");

		if (DOFEnabled)
			strcat (disp, " DOF On");

        sprintf(title, "CIS565 OpenGL Frame | %s FPS: %4.2f", disp, frame*1000.0/(currenttime-timebase));
        //sprintf(title, "CIS565 OpenGL Frame | %4.2f FPS", frame*1000.0/(currenttime-timebase));
        glutSetWindowTitle(title);
        timebase = currenttime;		
        frame = 0;
    }
}

void RenderDepthMap (Render render_type)
{
	bindFBO(2);
    glClear(GL_DEPTH_BUFFER_BIT);
    draw_mesh (render_type);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool doIScissor = true;
void RenderDeferred ()
{
	// Stage 1 -- RENDER TO G-BUFFER
	RenderDepthMap (RENDER_LIGHT);		// Shadow Map
    
    bindFBO(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_mesh(RENDER_CAMERA);
	glActiveTexture (GL_TEXTURE9);
	glBindTexture (GL_TEXTURE_2D, glowmaskTexture);
	glGenerateMipmap (GL_TEXTURE_2D);

    // Stage 2 -- RENDER TO P-BUFFER
    setTextures();
    bindFBO(1);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ONE, GL_ONE);
    glClear(GL_COLOR_BUFFER_BIT);
    if(display_type == DISPLAY_LIGHTS || display_type == DISPLAY_TOTAL)
    {
        setup_quad(point_prog);
        if(doIScissor) glEnable(GL_SCISSOR_TEST);
        mat4 vp = perspective(45.0f,(float)width/(float)height,NEARP,FARP) * 
                  cam.get_view();
        mat4 sc = mat4(width, 0.0,    0.0, 0.0,
                       0.0,   height, 0.0, 0.0,
                       0.0,   0.0,    1.0, 0.0,
                       0.0,   0.0,    0.0, 1.0) *
                  mat4(0.5, 0.0, 0.0, 0.0,
                       0.0, 0.5, 0.0, 0.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.5, 0.5, 0.0, 1.0);

		glm::vec3 yellow = glm::vec3 (1,1,0);
		glm::vec3 orange = glm::vec3 (0.89,0.44,0.1);
		glm::vec3 red = glm::vec3 (1,0,0);
		glm::vec3 white = glm::vec3 (1,1,1);

		glUniform1i (glGetUniformLocation (point_prog, "u_toonOn"), toonEnabled);
		glUniform3fv (glGetUniformLocation(point_prog, "u_LightCol"), 1, &(white[0]));

		for (std::list<LightData>::iterator i = lightList.begin (); i != lightList.end (); ++ i)
		{
			draw_light(vec3 (i->position.x, i->position.y, i->position.z), i->intensity.x, sc, vp, NEARP);
		}

		glBindBuffer (GL_SHADER_STORAGE_BUFFER, vplPosSBO);
		LightData * ldBuff = (LightData *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		for (int j = 0; j < nVPLs*nLights; ++j)
		{
			draw_light (vec3 (ldBuff [j].position.x, ldBuff [j].position.y, ldBuff [j].position.z), 
						ldBuff [j].intensity.x, sc, vp, NEARP);
		}
		glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
	
        glDisable(GL_SCISSOR_TEST);
        vec4 dir_light(0.1, 1.0, 1.0, 0.0);
        dir_light = cam.get_view() * dir_light; 
        dir_light = normalize(dir_light);
        dir_light.w = 0.3;
        float strength = 0.09;

        setup_quad(ambient_prog);
		glUniform1i (glGetUniformLocation (ambient_prog, "u_toonOn"), toonEnabled);
        glUniform4fv(glGetUniformLocation(ambient_prog, "u_Light"), 1, &(dir_light[0]));
        glUniform1f(glGetUniformLocation(ambient_prog, "u_LightIl"), strength);
        draw_quad();
    }
    else
    {
        setup_quad(diagnostic_prog);
        draw_quad();
    }
    glDisable(GL_BLEND);

    //Stage 3 -- RENDER TO SCREEN
    setTextures();
    glUseProgram(post_prog);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, postTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex"),0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, glowmaskTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_GlowMask"),1);

	glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_normalTex"),2);

	glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_positionTex"),3);

	glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomScalartex"),5);

	glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_depthTex"),6);

	glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_shadowTex"),7);

	glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, lightcordTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_lightCordTex"),8);

    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenWidth"), width);
	glUniform1f(glGetUniformLocation(post_prog, "u_InvScrHeight"), inv_height);
    glUniform1f(glGetUniformLocation(post_prog, "u_InvScrWidth"), inv_width);
	glUniform1f(glGetUniformLocation(post_prog, "u_mouseTexX"), mouse_dof_x*inv_width);
	glUniform1f(glGetUniformLocation(post_prog, "u_mouseTexY"), abs(height-mouse_dof_y)*inv_height);
	glUniform1f(glGetUniformLocation(post_prog, "u_lenQuant"), 0.0025);
	glUniform1f(glGetUniformLocation(post_prog, "u_Far"), FARP);
    glUniform1f(glGetUniformLocation(post_prog, "u_Near"), NEARP);
	glUniform1i(glGetUniformLocation(post_prog, "u_BloomOn"), bloomEnabled);
    glUniform1i(glGetUniformLocation(post_prog, "u_toonOn"), toonEnabled);
	glUniform1i(glGetUniformLocation(post_prog, "u_DOFOn"), DOFEnabled);
	glUniform1i(glGetUniformLocation(post_prog, "u_DOFDebug"), DOFDebug);
	draw_quad();

    glEnable(GL_DEPTH_TEST);
}

void PopulateLights ()
{
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, lightPosSBO);
	checkError (" in PopulateLights () while trying to bind lPos SSBO!");
	GLint bufferAccessMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	vec4 * ldBuff = (vec4 *) glMapBufferRange (GL_SHADER_STORAGE_BUFFER, 0, nLights*sizeof(vec4), bufferAccessMask);
	checkError (" in PopulateLights () while trying to init lPos SSBO!");
	int count = 0;
	for (std::list<LightData>::iterator j = lightList.begin (); j != lightList.end (); ++j)
	{
		ldBuff [count] = vec4 (j->position.x, j->position.y, j->position.z, j->intensity.x);
		++ count;
	}
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);

	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, vplPosSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, lightPosSBO);
}

void RenderForward ()
{
//	glUseProgram (forward_shading_prog);
	PopulateLights ();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*glEnable (GL_DEPTH_TEST);
	glColorMask (GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	glDepthFunc (GL_LESS);
	glDepthMask (GL_TRUE);
	draw_mesh_forward ();

	glEnable (GL_DEPTH_TEST);
	glColorMask (GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glDepthFunc (GL_LEQUAL);
	glDepthMask (GL_FALSE);*/
	draw_mesh_forward ();
}

void RenderFPlus ()
{
//	glUseProgram (forward_shading_prog);
	PopulateLights ();
//	RenderDepthMap (RENDER_CAMERA);
	glUseProgram (fplus_lightcull_prog);
	glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
//    glBindImageTexture (0, depthMapTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glUniform1i (glGetUniformLocation (fplus_lightcull_prog, "depthTex"),0);
//	glUniform1i (glGetUniformLocation (fplus_lightcull_prog, "depthTex2"),0);
    glUniform1i (glGetUniformLocation (fplus_lightcull_prog, "u_numLights"), nLights);
	glUniform1i (glGetUniformLocation (fplus_lightcull_prog, "u_numVPLs"), nVPLs);

	mat4 inverse_projection = inverse (cam.get_perspective());
	glUniformMatrix4fv (glGetUniformLocation (fplus_lightcull_prog, "u_InvProj"), 1, GL_FALSE, &inverse_projection[0][0]);
	mat4 view = cam.get_view();
	glUniformMatrix4fv (glGetUniformLocation (fplus_lightcull_prog, "u_View"), 1, GL_FALSE, &view[0][0]);

	/*glBindBuffer (GL_SHADER_STORAGE_BUFFER, lightListSBO);
	GLint bufferAccessMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	vec4 * rBuff = (vec4 *) glMapBufferRange (GL_SHADER_STORAGE_BUFFER, 0, 30*sizeof(vec4), bufferAccessMask);
	int count = 0;
	for (int j = 0; j < 30; ++j)
		rBuff [j] = vec4 (-1, -1, -1, -1);
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);*/

	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, vplPosSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, lightPosSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, lightListSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 4, debugBufferSBO);

	glDispatchCompute (width / 8, height / 8, 1);
	glFinish ();
	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);

	// Debug code---------------------------
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, lightPosSBO);
	vec4* llist = (vec4 *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, debugBufferSBO);
	llist = (vec4 *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, vplPosSBO);
	LightData *llist4 = (LightData *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, lightListSBO);
	uvec4 *llist2 = (uvec4 *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	for (int i = 0; i < 90; ++i)
		for (int j = 0; j < 160; ++j)
			if (llist2 [64*((i*160)+j)].z > 0)
				if (llist2 [64*((i*160)+j)].y == 1)
					int breakHere = -1;
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
	//---------------------------Debug code

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, vplPosSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, lightPosSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, lightListSBO);

	draw_mesh_fplus ();
}

void display(void)
{
	/* Debug code
	checkError (" in display () after outer glFinish ().");
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, rayInfoSBO);
	rBuff = (Ray *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);	*/

//	RenderDeferred ();
	RenderForward ();
//	RenderFPlus ();

    updateTitle();

    glutPostRedisplay();
    glutSwapBuffers();
}



void reshape(int w, int h)
{
    width = w;
    height = h;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    if (FBO[0] != 0 || depthTexture != 0 || normalTexture != 0 ) {
        freeFBO();
    }
    initFBO(w,h);
}

void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) 
	{
        mouse_buttons |= 1<<button;
    } 
	else if (state == GLUT_UP) 
	{
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;

	if (button == GLUT_RIGHT_BUTTON)
	{
		mouse_dof_x = mouse_old_x;
		mouse_dof_y = mouse_old_y;
	}
}

void motion(int x, int y)
{
    float dx, dy;
    dx = (float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);

    if (mouse_buttons & 1<<GLUT_RIGHT_BUTTON) 
	{
        cam.adjust(0,0,dx,0,0,0);;
    }
    else 
	{
        cam.adjust(-dx*0.2f,-dy*0.2f,0,0,0,0);
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void keyboard(unsigned char key, int x, int y) 
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
    switch(key) 
	{
        case(27):
            exit(0.0);
            break;
        case('w'):
            tz = 0.1;
            break;
        case('s'):
            tz = -0.1;
            break;
        case('d'):
            tx = -0.1;
            break;
        case('a'):
            tx = 0.1;
            break;
        case('q'):
            ty = 0.1;
            break;
        case('z'):
            ty = -0.1;
            break;
        case('1'):
            display_type = DISPLAY_DEPTH;
            break;
        case('2'):
            display_type = DISPLAY_NORMAL;
            break;
        case('3'):
            display_type = DISPLAY_COLOR;
            break;
        case('4'):
            display_type = DISPLAY_POSITION;
            break;
        case('5'):
            display_type = DISPLAY_LIGHTS;
            break;
        case('6'):
            display_type = DISPLAY_GLOWMASK;
            break;
        case('0'):
            display_type = DISPLAY_TOTAL;
            break;
        case('x'):
            doIScissor ^= true;
            break;
        case('r'):
            initShader();
			break;
		case('7'):
            display_type = DISPLAY_SHADOW;
			break;
		case 'b':
		case 'B':
			bloomEnabled = !bloomEnabled;
            break;
		case 't':
		case 'T':
//			toonEnabled = !toonEnabled;
			break;
		case 'f':
		case 'F':
			DOFEnabled = !DOFEnabled;
            break;
		case 'G':
		case 'g':
			DOFDebug = !DOFDebug;
			break;
    }

    if (abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0) 
	{
        cam.adjust(0,0,0,tx,ty,tz);
    }
}

void init() 
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f,1.0f);
}

void initSSBO ()
{
	//GLint bufferAccessMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	//LightData * ldBuff = (LightData *) glMapBufferRange (GL_SHADER_STORAGE_BUFFER, 0, nLights*sizeof(LightData), bufferAccessMask);

	//int count = 0;
	//for (std::list<LightData>::iterator i = lightList.begin (); i != lightList.end (); ++i)
	//{
	//	ldBuff [count] = *i;
	//	++ count;
	//}
	//glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);

	glGenBuffers (1, &vplPosSBO);
	checkError (" in initVPL () while trying to generate VPL SSBO!");
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, vplPosSBO);
	checkError (" in initVPL () while trying to bind VPL SSBO!");
	glBufferData (GL_SHADER_STORAGE_BUFFER, nVPLs*nLights*sizeof(LightData), NULL, GL_STATIC_DRAW);
	checkError (" in initVPL () while trying to allocate VPL SSBO!");

	glGenBuffers (1, &rayInfoSBO);
	checkError (" in initVPL () while trying to generate Ray SSBO!");
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, rayInfoSBO);
	checkError (" in initVPL () while trying to bind Ray SSBO!");
	glBufferData (GL_SHADER_STORAGE_BUFFER, ((nVPLs/nBounces)*nLights)*sizeof(Ray), NULL, GL_STATIC_DRAW);
	checkError (" in initVPL () while trying to allocate Ray SSBO!");

	glGenBuffers (1, &lightPosSBO);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, lightPosSBO);
	glBufferData (GL_SHADER_STORAGE_BUFFER, nLights*sizeof(vec4), NULL, GL_STATIC_DRAW);

	uvec2 numTiles = uvec2 (width / 8, height / 8);
	glGenBuffers (1, &lightListSBO);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, lightListSBO);
	glBufferData (GL_SHADER_STORAGE_BUFFER, numTiles.x*numTiles.y*MAX_LIGHTS_PER_TILE*sizeof(uvec4), NULL, GL_STATIC_DRAW);

	glGenBuffers (1, &debugBufferSBO);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, debugBufferSBO);
	glBufferData (GL_SHADER_STORAGE_BUFFER, numTiles.x*numTiles.y*sizeof(vec4)*64, NULL, GL_STATIC_DRAW);
}

void initVPL ()
{
	std::uniform_real_distribution<float>	xi1 (0.0f, 1.0f);
	std::uniform_real_distribution<float>	xi2 (0.0f, 1.0f);
	
	// Create the VPLs in the scene
	glUseProgram (vpl_prog);
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, rayInfoSBO);
	checkError (" in initVPL () while trying to bind Ray SSBO!");
	GLint bufferAccessMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	Ray * rBuff = (Ray *) glMapBufferRange (GL_SHADER_STORAGE_BUFFER, 0, ((nVPLs/nBounces)*nLights)*sizeof(Ray), bufferAccessMask);
	checkError (" in initVPL () while trying to init Ray SSBO!");
	int count = 0;
	for (std::list<LightData>::iterator j = lightList.begin (); j != lightList.end (); ++j)
	{
		int currentIndex = count*(nVPLs/nBounces);
		int divs = std::sqrt ((float)nVPLs/(float)nBounces);	int totalvertdivs = divs + 2;	// To avoid poles! 
		float vertStep = 1.0f / totalvertdivs, horizStep = 1.0f / divs;

		float u = 0.f, v = 0.f;
		for (int i = 0; i < (nVPLs/nBounces); ++i)
		{
			if ((i % divs) == 0)
			{	u = 0.f;	v += vertStep;	}

			glm::vec4 position = j->position;
			glm::vec4 direction = normalize (vec4 (cos(2.f*PI*u)*sin(PI*v),
												   cos(PI*v),
												   sin(2.f*PI*u)*sin(PI*v),
												   0.0));
			position += 0.01f*direction;

			rBuff [currentIndex + i].origin = position;
			rBuff [currentIndex + i].direction = direction;
			rBuff [currentIndex + i].intensity = j->intensity;
			u += horizStep;
		}
		++ count;
	}
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);

	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 1, vplPosSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 2, rayInfoSBO);
	glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 3, bBoxSBO);

	glUniform1i (glGetUniformLocation (vpl_prog, "u_numLights"), nLights);
	glUniform1i (glGetUniformLocation (vpl_prog, "u_numBounces"), nBounces);
	glUniform1i (glGetUniformLocation (vpl_prog, "u_numVPLs"), nVPLs);
	glUniform1i (glGetUniformLocation (vpl_prog, "u_numGeometry"), boundingBoxes.size ());

	/* Debug code
	checkError (" Mark 1337.");
	glBindBuffer (GL_SHADER_STORAGE_BUFFER, rayInfoSBO);
	rBuff = (Ray *) glMapBuffer (GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
	checkError (" Mark 1338."); */
	for (int i = 0; i < nBounces; ++ i)
	{
		glUniform1i (glGetUniformLocation (vpl_prog, "u_bounceNo"), i);
		//	checkError (" Mark 1339.");		// Debug code.
		glDispatchCompute (ceil ((nVPLs/nBounces)/128.0f), 1, 1);
		checkError (" in display () after outer glDispatchCompute ()/glFinish ().");
	}
	glFinish ();
	glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
}

int main (int argc, char* argv[])
{
    bool loadedScene = false;
    for(int i=1; i<argc; i++)
	{
        string header; string data;
        istringstream liness(argv[i]);
        getline(liness, header, '='); getline(liness, data, '=');
        if(strcmp(header.c_str(), "mesh")==0)
		{
            int found = data.find_last_of("/\\");
            string path = data.substr(0,found+1);
            cout << "Loading: " << data << endl;
            string err = tinyobj::LoadObj(shapes, data.c_str(), path.c_str());
            if(!err.empty())
            {
                cerr << err << endl;
                return -1;
            }
            loadedScene = true;
        }
    }

    if(!loadedScene)
	{
        cout << "Usage: mesh=[obj file]" << endl; 
        std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
        return 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    width = 1280;	inv_width = 1.0/(width-1);
    height = 720;	inv_height = 1.0/(height-1);
	cam.set_perspective (perspective(45.0f,(float)width/(float)height,NEARP,FARP));
	glutInitWindowSize(width,height);
    glutCreateWindow("CIS565 OpenGL Frame");
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        cout << "glewInit failed, aborting." << endl;
        exit (1);
    }

	// Make sure only OpenGL 4.3 or Direct3D 11 cards are allowed to run the program.
	if (!GLEW_VERSION_4_3)
		if (!GLEW_ARB_compute_shader)
		{
			cout << "This program requires either a Direct3D 11 or OpenGL 4.3 class graphics card." << endl
				 << "Press any key to terminate...";
			cin.get ();
			exit (1);
		}
    cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;
    cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

    initNoise();
    initShader();
    initFBO(width,height);
    init();
    initMesh();
    initQuad();
	initSSBO ();
	initVPL ();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);	
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}

void	checkError (char * printString)
{
	GLenum glError = glGetError ();
	switch (glError)
	{
	case GL_INVALID_VALUE: 
		cout << "\nInvalid value" << printString;
		cin.get ();
		exit (1);
	case GL_INVALID_OPERATION:
		cout << "\nInvalid operation" << printString;
		cin.get ();
		exit (1);
	case GL_INVALID_ENUM:
		cout << "\nInvalid enum" << printString;
		cin.get ();
		exit (1);
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		cout << "\nThe framebuffer object is not complete" << printString;
		cin.get ();
		exit (1);
	case GL_OUT_OF_MEMORY:
		cout << "\nThere is not enough memory left to execute the command" << printString;
		cin.get ();
		exit (1);
	case GL_STACK_UNDERFLOW:
		cout << "\nAn attempt has been made to perform an operation that would cause an internal stack to underflow" << printString;
		cin.get ();
		exit (1);
	case GL_STACK_OVERFLOW:
		cout << "\nAn attempt has been made to perform an operation that would cause an internal stack to overflow" << printString;
		cin.get ();
		exit (1);
	case GL_NO_ERROR:
		break;
	default:
		cout << "\nError No.: " << glError << printString;
		cin.get ();
		exit (1);
	}
}