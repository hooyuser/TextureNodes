#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
    float shift_x;
    float shift_y;
    float rotation;
    float scale_x;
    float scale_y;
    bool clamp;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D nodeTextures[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

mat2 buildScale(float sx, float sy)
{
    return mat2(sx, 0.0, 0.0, sy);
}
// rot is in degrees
mat2 buildRot(float rot)
{
    float r = radians(rot);
    return mat2(cos(r), -sin(r), sin(r), cos(r));
}
        
mat3 transMat(vec2 t)
{
    return mat3(vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(t, 1.0));
}
mat3 scaleMat(vec2 s)
{
    return mat3(vec3(s.x,0.0,0.0), vec3(0.0,s.y,0.0), vec3(0.0, 0.0, 1.0));
}
mat3 rotMat(float rot)
{
    float r = radians(rot);
    return mat3(vec3(cos(r), -sin(r),0.0), vec3(sin(r), cos(r),0.0), vec3(0.0, 0.0, 1.0));
}
void main()
{
    // transform by (-0.5, -0.5)
    // scale
    // rotate
    // transform
    // transform by (0.5, 0.5)  
    mat3 trans = transMat(vec2(0.5, 0.5)) *
        transMat(vec2(ubo.shift_x, ubo.shift_y)) *
        rotMat(ubo.rotation) *
        scaleMat(vec2(ubo.scale_x, ubo.scale_y)) *
        transMat(vec2(-0.5, -0.5));
    vec2 uv = (inverse(trans) * vec3(fragUV, 1.0)).xy;
    if (ubo.clamp){
        outColor = texture(image, clamp(uv,vec2(0.0), vec2(1.0)));
    }
    else {
        outColor = texture(image, uv);
    }
    
}

