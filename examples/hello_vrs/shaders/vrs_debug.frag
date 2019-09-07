#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_NV_shading_rate_image : enable

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

//in ivec2 gl_FragmentSizeNV;
//in int gl_InvocationsPerPixelNV;

void main() {
	outColor = 0.1 *  vec4(0,0,0,0.1);

	//if (gl_FragCoord.x < 1600) {
	//	return;
	//}

	
	// *****************************
	// analytical based
	// *****************************
	//float shadingRate = gl_InvocationsPerPixelNV / 16.0;
	//float shadingRate = gl_FragmentSizeNV.x / 4.0;
	//float shadingRate = gl_FragmentSizeNV.y / 4.0;
	//float shadingRate = gl_FragmentSizeNV.x * gl_FragmentSizeNV.y / 16.0f;
	float shadingRate =  0.5 + (-gl_FragmentSizeNV.x * gl_FragmentSizeNV.y + 1.0)/30.0 + (gl_InvocationsPerPixelNV - 1.0) / 30.0f;

	float greenOrBlue = step(0.5, shadingRate);
	float redToGreen = smoothstep(0, 0.5, shadingRate);
	float greenToBlue = smoothstep(0.5, 1, shadingRate);
	//outColor = 0.1 * vec4(mix(mix(vec3(0,0,1), vec3(0,1,0), redToGreen), mix(vec3(0,1,0), vec3(1,0,0), greenToBlue), greenOrBlue), 0.5f);
	//outColor = vec4(shadingRate);

	
	// *****************************
	// color palette based
	// *****************************
	//shadingRate = gl_FragmentSizeNV.x + gl_FragmentSizeNV.y/2.0;
	shadingRate = int(gl_FragmentSizeNV.y/2) * 2 + int(gl_FragmentSizeNV.x/2);
	shadingRate = 1.0 - shadingRate/6.0;
	//outColor = vec4(shadingRate/10.0);

	greenOrBlue = step(0.5, shadingRate);
	redToGreen = smoothstep(0, 0.5, shadingRate);
	greenToBlue = smoothstep(0.5, 1, shadingRate);
	//outColor = vec4(mix(mix(vec3(0,0,1), vec3(0,1,0), redToGreen), mix(vec3(0,1,0), vec3(1,0,0), greenToBlue), greenOrBlue), 0.5f);
	//outColor = 0.1 * vec4(mix(vec3(0,0,1), vec3(0,1,0), shadingRate), 1.0);


	
	// *****************************
	// experiments
	// *****************************


	//outColor = vec4(vec3(gl_InvocationsPerPixelNV / 16.0f), 0.5f);
	//outColor = vec4(vec3(gl_FragmentSizeNV.x * gl_FragmentSizeNV.y / 16.0f), 0.5f);
	//outColor = vec4(vec3( 0.5 + (-gl_FragmentSizeNV.x * gl_FragmentSizeNV.y + 1.0)/30.0 + (gl_InvocationsPerPixelNV - 1.0) / 30.0f), 0.5f);
	//outColor = vec4(vec3(gl_FragmentSizeNV.x / 4.0f), 0.5f);
	//outColor =  vec4( 0.1 * texture(texSampler, fragTexCoord).xyz, 0.5);
	//outColor =  vec4( 0.1 * texelFetch(texSampler, ivec2(fragTexCoord * textureSize(texSampler, 0)), 0).xyz, 0.5);
	
	if (mod(gl_FragCoord.y/64, 2) <= 1 
	//&& mod(gl_FragCoord.x/32, 2) <= 1 ||
	//mod(gl_FragCoord.y/32, 2) >= 1 &&
	//mod(gl_FragCoord.x/32, 2) >= 1
	) {
		//outColor = 0.1 *  vec4(1,1,1,0.1);
	}
	//outColor = vec4(gl_FragCoord.x/1600);


}