#version 450
layout(location=0) in vec2 v_uv;
layout(location=1) in vec4 v_color;
layout(location=0) out vec4 o_color;
layout(set=0,binding=0) uniform sampler2D u_y;
layout(set=0,binding=1) uniform sampler2D u_u;
layout(set=0,binding=2) uniform sampler2D u_v;
layout(push_constant) uniform Params {vec4 red;vec4 green;vec4 blue;vec4 levels;} constants;
void main() {
    vec4 yuv=vec4(texture(u_y,v_uv).r-constants.levels.x,
        texture(u_u,v_uv).r-constants.levels.y,texture(u_v,v_uv).r-constants.levels.y,1.0);
    vec3 rgb=vec3(dot(yuv,constants.red),dot(yuv,constants.green),dot(yuv,constants.blue));
    o_color=vec4(clamp(rgb,0.0,1.0),1.0)*v_color;
}
