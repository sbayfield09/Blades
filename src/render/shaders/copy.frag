#version 450
layout(location=0) in vec2 v_uv;
layout(location=0) out vec4 o_color;
layout(set=0,binding=0) uniform sampler2D u_texture;
layout(push_constant) uniform Push {vec4 alpha;} pc;
void main() {
    vec2 uv=v_uv;
#ifdef BLADES_OPENGL
    uv.y=1.0-uv.y;
#endif
    o_color=vec4(texture(u_texture,uv).rgb,pc.alpha.x);
}
