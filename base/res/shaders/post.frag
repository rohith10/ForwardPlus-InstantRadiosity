#version 330

////////////////////////////
//       ENUMERATIONS
////////////////////////////

#define	DISPLAY_DEPTH 0
#define	DISPLAY_NORMAL 1
#define	DISPLAY_POSITION 2
#define	DISPLAY_COLOR 3
#define	DISPLAY_TOTAL 4
#define	DISPLAY_LIGHTS 5


/////////////////////////////////////
// Uniforms, Attributes, and Outputs
////////////////////////////////////
uniform sampler2D u_Posttex;
uniform sampler2D u_GlowMask;
uniform sampler2D u_RandomNormaltex;
uniform sampler2D u_RandomScalartex;
uniform sampler2D u_normalTex;
uniform sampler2D u_positionTex;
uniform sampler2D u_depthTex;
uniform sampler2D u_shadowTex;
uniform sampler2D u_lightCordTex;

uniform int u_ScreenWidth;
uniform int u_ScreenHeight;

uniform float u_InvScrHeight;
uniform float u_InvScrWidth;
uniform float u_mouseTexX;
uniform float u_mouseTexY;
uniform float u_lenQuant;
uniform float u_Near;
uniform float u_Far;

uniform bool u_BloomOn;
uniform bool u_toonOn;
uniform bool u_DOFOn;
uniform bool u_DOFDebug;

in vec2 fs_Texcoord;

out vec4 out_Color;
///////////////////////////////////////




uniform float zerothresh = 1.0f;
uniform float falloff = 0.1f;


/////////////////////////////////////
//				UTILITY FUNCTIONS
/////////////////////////////////////

//Depth used in the Z buffer is not linearly related to distance from camera
//This restores linear depth
float linearizeDepth(float exp_depth, float near, float far) {
    return	(2 * near) / (far + near -  exp_depth * (far - near)); 
}

//Helper function to automicatlly sample and unpack positions
vec3 sampleCol(vec2 texcoords) {
    return texture(u_Posttex,texcoords).xyz;
}

//Helper function to automicatlly sample and unpack light coord 
vec3 sampleLCoord(vec2 texcoords) {
    return texture(u_lightCordTex,texcoords).xyz;
}


float sampleGlowMask (vec2 texcoords)
{
	return	texture (u_GlowMask, texcoords).x;
}

//Get a random normal vector  given a screen-space texture coordinate
//Actually accesses a texture of random vectors
vec3 getRandomNormal(vec2 texcoords) {
    ivec2 sz = textureSize(u_RandomNormaltex,0);
    return texture(u_RandomNormaltex,vec2(texcoords.s* (u_ScreenWidth)/sz.x,
                (texcoords.t)*(u_ScreenHeight)/sz.y)).rgb;
}


//Get a random scalar given a screen-space texture coordinate
//Fetches from a random texture
float getRandomScalar(vec2 texcoords) {
    ivec2 sz = textureSize(u_RandomScalartex,0);
    return texture(u_RandomScalartex,vec2(texcoords.s*u_ScreenWidth/sz.x,
                texcoords.t*u_ScreenHeight/sz.y)).r;
}

///////////////////////////////////
// MAIN
//////////////////////////////////
const float occlusion_strength = 1.5f;
const mat3 GaussianMat3 = mat3 (vec3 (1,2,1), 
							   vec3 (2,4,2),
							   vec3 (1,2,1)) / 16.0;
const float GaussianMat5 [] = {	1/273.0, 4/273.0, 7/273.0, 4/273.0, 1/273.0,
								4/273.0, 16/273.0, 26/273.0, 16/273.0, 4/273.0,
							    7/273.0, 26/273.0, 41/273.0, 26/273.0, 7/273.0,
							    4/273.0, 16/273.0, 26/273.0, 16/273.0, 4/273.0,
							    1/273.0, 4/273.0, 7/273.0, 4/273.0, 1/273.0	};
								
void main() 
{
    vec3 color = sampleCol(fs_Texcoord);
//	if(linearizeDepth (texture(u_lightCordTex,fs_Texcoord).z, u_Near, u_Far) > linearizeDepth (texture (u_shadowTex, fs_Texcoord).x, u_Near, u_Far))
// if ( texture( shadowMap, ShadowCoord.xy ).z  <  ShadowCoord.z)
//if ( texture (u_shadowTex , texture(u_lightCordTex,fs_Texcoord).xy).z < u_lightCordTex.z) 
	//if (texture (u_lightCordTex,fs_Texcoord).x > 1.0)
vec2 smTexcoord = texture(u_lightCordTex,fs_Texcoord).xy;
smTexcoord += vec2 (1.0);
smTexcoord /= 2.0;
	if(texture(u_lightCordTex,fs_Texcoord).z > texture (u_shadowTex, smTexcoord).x)
		color = color*vec3 (0.3);

//if (texture(shadowMapSampler, shadowCoord.xy).z < shadowCoord.z - 0.0001f)
//    shadowFactor = 0.0f;

	if (u_BloomOn)
	{
//		if (sampleGlowMask (fs_Texcoord))
//		{
			vec3 bloomColour = vec3(0);
			for (int i = -1; i < 2; ++i)
			{
				int j = -1;
				bloomColour += (texture (u_GlowMask, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight), 2).xyz * GaussianMat3 [i+1][j+1]);
				++ j;
				bloomColour += (texture (u_GlowMask, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight), 2).xyz * GaussianMat3 [i+1][j+1]);
				++ j;
				bloomColour += (texture (u_GlowMask, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight), 2).xyz * GaussianMat3 [i+1][j+1]);
				++ j;
			}  
			color += color*bloomColour;  
//		}
	}

	if (u_toonOn)
	{
		float dotPdt = dot (texture (u_normalTex, fs_Texcoord).xyz, -(texture (u_positionTex, fs_Texcoord).xyz));
		if (dotPdt < 0.1)
			color = vec3 (0.0, 0.0, 0.0);
	}

	if (u_DOFOn)
	{
		float depth = texture (u_depthTex, fs_Texcoord).x;
		depth = linearizeDepth (depth, u_Near, u_Far);

		float focalLen = texture (u_depthTex, vec2 (u_mouseTexX, u_mouseTexY)).x;
		focalLen = linearizeDepth (focalLen, u_Near, u_Far);

		float lenQuant = 0.01;
		depth = abs (focalLen - depth); 
		depth /= lenQuant;

		vec3 bloomColour = vec3(0);
		if (depth >= 3.0)
		{
			for (int i = -2; i < 3; ++i)
			{
				int j = -2;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat5 [(j+2)*5+ (i+2)]);
				++ j;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat5 [(j+2)*5+ (i+2)]);
				++ j;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat5 [(j+2)*5+ (i+2)]);
				++ j;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat5 [(j+2)*5+ (i+2)]);
				++ j;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat5 [(j+2)*5+ (i+2)]);
				++ j;
			}

			if (!u_DOFDebug)
				color = bloomColour;
			else
				color = vec3 (1.0, 0.0, 0.0);
		}

		else if (depth >= 2.0)
		{
			for (int i = -1; i < 2; ++i)
			{
				int j = -1;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat3 [i+1][j+1]);
				++ j;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat3 [i+1][j+1]);
				++ j;
				bloomColour += (texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz * GaussianMat3 [i+1][j+1]);
				++ j;
			}
			if (!u_DOFDebug)
				color = bloomColour;
			else
				color = vec3 (0.0, 1.0, 0.0);
		}
		else if (depth >= 1.0)
		{
			for (int i = 0; i < 2; ++ i)
			{			
				for (int j = 0; j < 2; ++ j)
				{
					bloomColour += texture (u_Posttex, vec2 (fs_Texcoord.x + i*u_InvScrWidth, fs_Texcoord.y + j*u_InvScrHeight)).xyz;
				}
			}
			bloomColour /= 4.0;
			
			if (!u_DOFDebug)
				color = bloomColour;
			else
				color = vec3 (0.0, 0.0, 1.0);
		}
	}

//    float gray = dot(color, vec3(0.2125, 0.7154, 0.0721));
//    float vin = min(2*distance(vec2(0.5), fs_Texcoord), 1.0);
    out_Color = vec4(color,1.0);//vec4(mix(pow(color,vec3(1.0/1.8)),vec3(gray),vin), 1.0);
    return;
}

